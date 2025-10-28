#include "brain_ai/core/graph.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace brain_ai {

class ConnectionGraph::Impl {
public:
    explicit Impl(const std::string& db_path) {
        int rc = sqlite3_open(db_path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::string err = sqlite3_errmsg(db_);
            sqlite3_close(db_);
            throw std::runtime_error("Failed to open graph database: " + db_path + " - " + err);
        }
        
        // Create tables
        const char* schema = R"(
            CREATE TABLE IF NOT EXISTS node (
                id INTEGER PRIMARY KEY,
                kind TEXT NOT NULL,
                created_at INTEGER NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS edge (
                src INTEGER NOT NULL,
                dst INTEGER NOT NULL,
                weight REAL NOT NULL DEFAULT 1.0,
                last_seen INTEGER NOT NULL,
                PRIMARY KEY (src, dst)
            );
            
            CREATE INDEX IF NOT EXISTS idx_edge_src ON edge(src);
            CREATE INDEX IF NOT EXISTS idx_edge_dst ON edge(dst);
        )";
        
        char* err = nullptr;
        if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "Unknown error";
            sqlite3_free(err);
            throw std::runtime_error("Schema creation failed: " + msg);
        }
    }
    
    ~Impl() {
        if (db_) {
            sqlite3_close(db_);
        }
    }
    
    void ensure_node(int64_t id, const std::string& kind) {
        const char* sql = "INSERT OR IGNORE INTO node (id, kind, created_at) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }
        
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, kind.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, std::time(nullptr));
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    void update_from_recall(int64_t query_id, const std::vector<int64_t>& hit_ids,
                           const std::vector<float>& similarities, float threshold) {
        ensure_node(query_id, "query");
        
        for (size_t i = 0; i < hit_ids.size(); ++i) {
            if (similarities[i] >= threshold) {
                ensure_node(hit_ids[i], "memory");
                reinforce_edge(query_id, hit_ids[i], 0.05);
                
                // Co-occurrence edges between top hits
                for (size_t j = i + 1; j < std::min(i + 10, hit_ids.size()); ++j) {
                    reinforce_edge(hit_ids[i], hit_ids[j], 0.02);
                }
            }
        }
    }
    
    void reinforce_edge(int64_t src, int64_t dst, float step) {
        const char* sql = R"(
            INSERT INTO edge (src, dst, weight, last_seen) 
            VALUES (?, ?, ?, ?)
            ON CONFLICT(src, dst) DO UPDATE SET 
                weight = weight + ?,
                last_seen = ?
        )";
        
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        
        auto now = std::time(nullptr);
        sqlite3_bind_int64(stmt, 1, src);
        sqlite3_bind_int64(stmt, 2, dst);
        sqlite3_bind_double(stmt, 3, step);
        sqlite3_bind_int64(stmt, 4, now);
        sqlite3_bind_double(stmt, 5, step);
        sqlite3_bind_int64(stmt, 6, now);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    std::vector<Edge> get_neighbors(int64_t id, int limit) {
        const char* sql = "SELECT src, dst, weight, last_seen FROM edge WHERE src = ? ORDER BY weight DESC LIMIT ?";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_int(stmt, 2, limit);
        
        std::vector<Edge> edges;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            edges.push_back({
                sqlite3_column_int64(stmt, 0),
                sqlite3_column_int64(stmt, 1),
                static_cast<float>(sqlite3_column_double(stmt, 2)),
                sqlite3_column_int64(stmt, 3)
            });
        }
        
        sqlite3_finalize(stmt);
        return edges;
    }
    
    std::vector<int64_t> rerank_with_graph(int64_t query_id,
                                          const std::vector<int64_t>& candidates,
                                          const std::vector<float>& similarities,
                                          float alpha, int fanout) {
        // Get graph weights for candidates
        std::unordered_map<int64_t, float> graph_weights;
        auto neighbors = get_neighbors(query_id, fanout);
        
        for (const auto& edge : neighbors) {
            graph_weights[edge.dst] = edge.weight;
        }
        
        // Compute combined scores
        std::vector<std::pair<float, int64_t>> scored;
        for (size_t i = 0; i < candidates.size(); ++i) {
            float graph_w = graph_weights.count(candidates[i]) ? graph_weights[candidates[i]] : 0.0f;
            float score = alpha * similarities[i] + (1.0f - alpha) * graph_w;
            scored.push_back({score, candidates[i]});
        }
        
        std::sort(scored.begin(), scored.end(), std::greater<>());
        
        std::vector<int64_t> reranked;
        for (const auto& [score, id] : scored) {
            reranked.push_back(id);
        }
        
        return reranked;
    }
    
    void apply_decay(float factor) {
        const char* sql = "UPDATE edge SET weight = weight * ? WHERE weight > 0.01";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_double(stmt, 1, factor);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    size_t node_count() const {
        const char* sql = "SELECT COUNT(*) FROM node";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_step(stmt);
        size_t count = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return count;
    }
    
    size_t edge_count() const {
        const char* sql = "SELECT COUNT(*) FROM edge";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_step(stmt);
        size_t count = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return count;
    }
    
private:
    sqlite3* db_ = nullptr;
};

// ConnectionGraph implementation
ConnectionGraph::ConnectionGraph(const std::string& db_path) 
    : impl_(std::make_unique<Impl>(db_path)) {}

ConnectionGraph::~ConnectionGraph() = default;

void ConnectionGraph::ensure_node(int64_t id, const std::string& kind) {
    impl_->ensure_node(id, kind);
}

void ConnectionGraph::update_from_recall(int64_t query_id, 
                                        const std::vector<int64_t>& hit_ids,
                                        const std::vector<float>& similarities, 
                                        float threshold) {
    impl_->update_from_recall(query_id, hit_ids, similarities, threshold);
}

std::vector<Edge> ConnectionGraph::get_neighbors(int64_t id, int limit) {
    return impl_->get_neighbors(id, limit);
}

void ConnectionGraph::reinforce_edge(int64_t src, int64_t dst, float step) {
    impl_->reinforce_edge(src, dst, step);
}

void ConnectionGraph::apply_decay(float factor) {
    impl_->apply_decay(factor);
}

std::vector<int64_t> ConnectionGraph::rerank_with_graph(
    int64_t query_id,
    const std::vector<int64_t>& candidates,
    const std::vector<float>& similarities,
    float alpha, int fanout) {
    return impl_->rerank_with_graph(query_id, candidates, similarities, alpha, fanout);
}

size_t ConnectionGraph::node_count() const {
    return impl_->node_count();
}

size_t ConnectionGraph::edge_count() const {
    return impl_->edge_count();
}

} // namespace brain_ai
