#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <ctime>

namespace brain_ai {

struct Edge {
    int64_t src;
    int64_t dst;
    float weight;
    time_t last_seen;
};

struct Node {
    int64_t id;
    std::string kind;
    time_t created_at;
};

class ConnectionGraph {
public:
    explicit ConnectionGraph(const std::string& db_path);
    ~ConnectionGraph();
    
    // Automatic graph building
    void ensure_node(int64_t id, const std::string& kind = "memory");
    void update_from_recall(int64_t query_id, const std::vector<int64_t>& hit_ids, 
                           const std::vector<float>& similarities, float threshold);
    
    // Graph operations
    std::vector<Edge> get_neighbors(int64_t id, int limit = 32);
    void reinforce_edge(int64_t src, int64_t dst, float step);
    void apply_decay(float factor);
    void cap_degree(int64_t id, int max_edges);
    
    // Reranking
    std::vector<int64_t> rerank_with_graph(int64_t query_id, 
                                          const std::vector<int64_t>& candidates,
                                          const std::vector<float>& similarities,
                                          float alpha, int fanout);
    
    // Stats
    size_t node_count() const;
    size_t edge_count() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace brain_ai
