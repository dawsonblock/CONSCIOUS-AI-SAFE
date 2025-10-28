#include "brain_ai/config.hpp"
#include "brain_ai/core/qw_simple.hpp"
#include "brain_ai/core/memory_index.hpp"
#include "brain_ai/core/graph.hpp"
#include "brain_ai/metrics.hpp"
#include "brain_ai/filters.hpp"
#include "brain_ai/cache.hpp"
#include "brain_ai/http_server.hpp"
#include "brain.pb.h"
#include "brain.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <fstream>
#include <csignal>
#include <cstdlib>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace brain_ai {

class BrainServiceImpl final : public brain::Brain::Service {
public:
    explicit BrainServiceImpl(const SystemConfig& config) 
        : config_(config),
          qw_(config.quantum.dimension),
          memory_(create_memory_index(config.memory.backend, config.embeddings.dim)),
          graph_(config.sqlite.db_path),
          filters_(config.sqlite.db_path),
          cache_(config.cache.size, config.cache.ttl_s),
          start_time_(std::chrono::steady_clock::now()) {
        
        std::cout << "✅ Brain service initialized\n";
        std::cout << "   - Quantum dimension: " << config.quantum.dimension << "\n";
        std::cout << "   - Memory backend: " << config.memory.backend << "\n";
        std::cout << "   - Graph enabled: " << config.graph.enable << "\n";
        
        // Load existing index if available
        if (std::ifstream(config.memory.path).good()) {
            try {
                memory_->load(config.memory.path);
                std::cout << "   - Loaded " << memory_->size() << " vectors\n";
            } catch (const std::exception& e) {
                std::cerr << "   ⚠️  Failed to load index: " << e.what() << "\n";
            }
        }
    }
    
    Status HealthCheck(ServerContext* context, const brain::HealthReq* request,
                      brain::HealthResp* response) override {
        response->set_status("SERVING");
        response->set_version("3.6.0");
        auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_).count();
        response->set_uptime_ms(uptime);
        return Status::OK;
    }
    
    Status Step(ServerContext* context, const brain::StepReq* request,
               brain::StepResp* response) override {
        Timer timer("step_ms");
        
        // Process quantum step
        qw_.step(config_.quantum.dt);
        total_steps_++;
        
        response->set_entropy(qw_.entropy());
        response->set_total_collapses(total_steps_.load());
        
        // Set memory stats
        auto* mem_stats = response->mutable_memory_stats();
        mem_stats->set_total_items(memory_->size());
        
        Metrics::instance().inc_counter("step_total");
        Metrics::instance().set_gauge("entropy", qw_.entropy());
        
        return Status::OK;
    }
    
    Status Upsert(ServerContext* context, const brain::UpsertReq* request,
                 brain::UpsertResp* response) override {
        Timer timer("upsert_ms");
        
        std::vector<std::vector<float>> vectors;
        std::vector<int64_t> ids;
        
        for (int i = 0; i < request->ids_size(); ++i) {
            ids.push_back(request->ids(i));
            
            const auto& vec = request->vectors(i);
            std::vector<float> v(vec.data().begin(), vec.data().end());
            vectors.push_back(v);
            
            // Ensure node in graph
            if (config_.graph.enable) {
                graph_.ensure_node(ids[i], "memory");
            }
        }
        
        memory_->add(vectors, ids);
        response->set_count(ids.size());
        
        Metrics::instance().inc_counter("upsert_total", ids.size());
        
        return Status::OK;
    }
    
    Status Recall(ServerContext* context, const brain::RecallReq* request,
                 brain::RecallResp* response) override {
        Timer timer("recall_ms");
        
        std::vector<float> query(request->query().begin(), request->query().end());
        int k = request->topk() > 0 ? request->topk() : config_.memory.topk_default;
        
        // Check cache
        std::string cache_key = compute_cache_key(query);
        auto cached = cache_.get(cache_key);
        if (cached.has_value()) {
            Metrics::instance().inc_counter("cache_hit");
            auto [ids, dists] = *cached;
            for (auto id : ids) response->add_ids(id);
            for (auto d : dists) response->add_distances(d);
            return Status::OK;
        }
        
        Metrics::instance().inc_counter("cache_miss");
        
        // Apply filters if requested
        std::unordered_set<int64_t> allowed_ids;
        if (request->filters_size() > 0 && config_.filters.enable) {
            std::vector<FilterPredicate> preds;
            for (const auto& f : request->filters()) {
                preds.push_back({f.key(), f.op(), f.value_text(), f.value_num()});
            }
            allowed_ids = filters_.filter_ids(preds);
        }
        
        // Perform ANN search
        auto result = memory_->search(query, k * 2); // Over-fetch for filtering/reranking
        
        // Apply filter intersection
        std::vector<int64_t> filtered_ids;
        std::vector<float> filtered_dists;
        if (!allowed_ids.empty()) {
            for (size_t i = 0; i < result.ids.size(); ++i) {
                if (allowed_ids.count(result.ids[i])) {
                    filtered_ids.push_back(result.ids[i]);
                    filtered_dists.push_back(result.distances[i]);
                    if (filtered_ids.size() >= static_cast<size_t>(k)) break;
                }
            }
        } else {
            filtered_ids = result.ids;
            filtered_dists = result.distances;
        }
        
        // Rerank with graph if enabled
        if (request->use_graph() && config_.graph.enable && !filtered_ids.empty()) {
            int64_t query_id = std::hash<std::string>{}(cache_key);
            
            // Convert distances to similarities
            std::vector<float> sims;
            for (auto d : filtered_dists) {
                sims.push_back(1.0f / (1.0f + d));
            }
            
            // Update graph automatically
            graph_.update_from_recall(query_id, filtered_ids, sims, config_.graph.sim_threshold);
            
            // Rerank
            filtered_ids = graph_.rerank_with_graph(query_id, filtered_ids, sims,
                                                   config_.graph.alpha_rerank, 
                                                   config_.graph.fanout);
        }
        
        // Truncate to k
        if (filtered_ids.size() > static_cast<size_t>(k)) {
            filtered_ids.resize(k);
            filtered_dists.resize(k);
        }
        
        // Cache result
        cache_.put(cache_key, std::make_pair(filtered_ids, filtered_dists));
        
        for (auto id : filtered_ids) response->add_ids(id);
        for (auto d : filtered_dists) response->add_distances(d);
        
        Metrics::instance().inc_counter("recall_total");
        
        return Status::OK;
    }
    
    Status GetStats(ServerContext* context, const brain::StatsReq* request,
                   brain::StatsResp* response) override {
        response->set_entropy(qw_.entropy());
        response->set_total_collapses(total_steps_.load());
        response->set_dimension(config_.quantum.dimension);
        
        auto* mem_stats = response->mutable_memory_stats();
        mem_stats->set_total_items(memory_->size());
        
        auto* graph_stats = response->mutable_graph_stats();
        graph_stats->set_nodes(graph_.node_count());
        graph_stats->set_edges(graph_.edge_count());
        
        // Add metrics
        (*response->mutable_metrics())["step_total"] = 
            Metrics::instance().get_counter("step_total");
        (*response->mutable_metrics())["recall_total"] = 
            Metrics::instance().get_counter("recall_total");
        (*response->mutable_metrics())["cache_hit_rate"] = 
            Metrics::instance().get_counter("cache_hit") / 
            (double)(Metrics::instance().get_counter("cache_hit") + 
                    Metrics::instance().get_counter("cache_miss") + 1e-9);
        
        return Status::OK;
    }
    
private:
    std::string compute_cache_key(const std::vector<float>& query) {
        // Simple hash of first few elements
        std::hash<float> hasher;
        size_t h = 0;
        for (size_t i = 0; i < std::min(query.size(), size_t(8)); ++i) {
            h ^= hasher(query[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return std::to_string(h);
    }
    
    SystemConfig config_;
    brain::QuantumWorkspace qw_;
    std::unique_ptr<MemoryIndex> memory_;
    ConnectionGraph graph_;
    MetadataFilter filters_;
    LRUCache<std::string, std::pair<std::vector<int64_t>, std::vector<float>>> cache_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<int64_t> total_steps_{0};
};

} // namespace brain_ai

int main(int argc, char** argv) {
    std::string config_path = "configs/system.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    std::cout << "🧠 Brain-AI v3.6.0 Server\\n";
    std::cout << "Loading config: " << config_path << "\\n";
    
    try {
        auto config = brain_ai::SystemConfig::load(config_path);
        config.validate();
        
        brain_ai::BrainServiceImpl service(config);
        
        std::string server_address = "0.0.0.0:" + std::to_string(config.server.grpc_port);
        
        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        
        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "✅ Server listening on " << server_address << "\\n";
        std::cout << "   HTTP metrics on port " << config.server.http_port << "\\n";
        
        server->Wait();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\\n";
        return 1;
    }
    
    return 0;
}
