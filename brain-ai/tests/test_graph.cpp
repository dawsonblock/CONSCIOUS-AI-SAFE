#include <gtest/gtest.h>
#include "brain_ai/core/graph.hpp"
#include <filesystem>

TEST(ConnectionGraphTest, BasicNodeEnsure) {
    std::string db_path = "/tmp/test_graph.db";
    std::filesystem::remove(db_path);
    
    brain_ai::ConnectionGraph graph(db_path);
    graph.ensure_node(1, "test");
    graph.ensure_node(2, "test");
    
    EXPECT_EQ(graph.node_count(), 2);
}

TEST(ConnectionGraphTest, EdgeReinforcement) {
    std::string db_path = "/tmp/test_graph2.db";
    std::filesystem::remove(db_path);
    
    brain_ai::ConnectionGraph graph(db_path);
    graph.ensure_node(1, "test");
    graph.ensure_node(2, "test");
    
    graph.reinforce_edge(1, 2, 0.5f);
    
    auto neighbors = graph.get_neighbors(1, 10);
    EXPECT_GE(neighbors.size(), 1);
    if (!neighbors.empty()) {
        EXPECT_EQ(neighbors[0].dst, 2);
        EXPECT_GE(neighbors[0].weight, 0.5f);
    }
}

TEST(ConnectionGraphTest, AutoUpdateFromRecall) {
    std::string db_path = "/tmp/test_graph3.db";
    std::filesystem::remove(db_path);
    
    brain_ai::ConnectionGraph graph(db_path);
    
    std::vector<int64_t> hits = {10, 11, 12, 13, 14};
    std::vector<float> sims = {0.9f, 0.88f, 0.85f, 0.80f, 0.75f};
    
    graph.update_from_recall(1, hits, sims, 0.85f);
    
    EXPECT_GE(graph.node_count(), 1);
    EXPECT_GE(graph.edge_count(), 1);
}
