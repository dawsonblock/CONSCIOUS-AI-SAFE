#include "brain_ai/core/memory_index.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>

using namespace brain_ai;
using namespace std::chrono;

void benchmark_recall(const std::string& backend, int dim, int n_vectors, int n_queries, int topk) {
    std::cout << "\n🔍 Benchmarking " << backend << " (dim=" << dim << ", n=" << n_vectors << ")\n";
    std::cout << "───────────────────────────────────────────\n";
    
    // Create index
    auto index = create_memory_index(backend, dim);
    
    // Generate random vectors
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::cout << "Generating " << n_vectors << " random vectors...\n";
    std::vector<std::vector<float>> vectors;
    std::vector<int64_t> ids;
    for (int i = 0; i < n_vectors; ++i) {
        std::vector<float> vec(dim);
        for (int j = 0; j < dim; ++j) {
            vec[j] = dist(gen);
        }
        vectors.push_back(vec);
        ids.push_back(i);
    }
    
    // Add vectors
    auto start_add = high_resolution_clock::now();
    index->add(vectors, ids);
    auto end_add = high_resolution_clock::now();
    double add_time = duration<double, std::milli>(end_add - start_add).count();
    
    std::cout << "✅ Added " << n_vectors << " vectors in " << add_time << " ms\n";
    std::cout << "   (" << (add_time / n_vectors) << " ms/vector)\n";
    
    // Query benchmark
    std::cout << "\nQuerying " << n_queries << " times (top-" << topk << ")...\n";
    std::vector<double> query_times;
    
    for (int i = 0; i < n_queries; ++i) {
        // Generate random query
        std::vector<float> query(dim);
        for (int j = 0; j < dim; ++j) {
            query[j] = dist(gen);
        }
        
        auto start_q = high_resolution_clock::now();
        auto result = index->search(query, topk);
        auto end_q = high_resolution_clock::now();
        
        double q_time = duration<double, std::milli>(end_q - start_q).count();
        query_times.push_back(q_time);
    }
    
    // Calculate stats
    std::sort(query_times.begin(), query_times.end());
    double p50 = query_times[n_queries / 2];
    double p95 = query_times[static_cast<size_t>(n_queries * 0.95)];
    double p99 = query_times[static_cast<size_t>(n_queries * 0.99)];
    double mean = std::accumulate(query_times.begin(), query_times.end(), 0.0) / n_queries;
    
    std::cout << "\n📊 Query Performance:\n";
    std::cout << "   Mean:  " << std::fixed << std::setprecision(3) << mean << " ms\n";
    std::cout << "   p50:   " << p50 << " ms\n";
    std::cout << "   p95:   " << p95 << " ms";
    
    // Check against 30ms target
    if (p95 <= 30.0) {
        std::cout << " ✅ (target: 30 ms)\n";
    } else {
        std::cout << " ❌ (target: 30 ms, exceeded by " << (p95 - 30.0) << " ms)\n";
    }
    
    std::cout << "   p99:   " << p99 << " ms\n";
    std::cout << "   QPS:   " << (1000.0 / mean) << "\n";
}

int main(int argc, char** argv) {
    std::cout << "🚀 Brain-AI v3.6.0 - Memory Recall Benchmark\n";
    std::cout << "════════════════════════════════════════════════\n";
    
    int dim = 384;
    int n_queries = 100;
    int topk = 32;
    
    // Test different scales
    std::vector<int> scales = {1000, 10000, 100000};
    
    for (int n : scales) {
        try {
            benchmark_recall("hnswlib", dim, n, n_queries, topk);
        } catch (const std::exception& e) {
            std::cerr << "❌ Error: " << e.what() << "\n";
        }
    }
    
    std::cout << "\n════════════════════════════════════════════════\n";
    std::cout << "📈 Summary: Recall p95 target is 30ms @ 1M vectors\n";
    std::cout << "   (Note: 1M test requires significant memory)\n";
    
    return 0;
}
