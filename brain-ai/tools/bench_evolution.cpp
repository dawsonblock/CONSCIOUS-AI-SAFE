#include "brain_ai/core/qw_simple.hpp"
#include "brain_ai/metrics.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace brain_ai;
using namespace std::chrono;

struct BenchmarkResult {
    std::string name;
    std::vector<double> timings_ms;
    double p50, p95, p99, mean;
};

BenchmarkResult benchmark_evolution(int dim, int iterations) {
    BenchmarkResult result;
    result.name = "Quantum Evolution (dim=" + std::to_string(dim) + ")";
    
    brain::QuantumWorkspace qw(dim);
    
    for (int i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        qw.step(0.01);
        auto end = high_resolution_clock::now();
        
        double ms = duration<double, std::milli>(end - start).count();
        result.timings_ms.push_back(ms);
    }
    
    // Calculate percentiles
    std::sort(result.timings_ms.begin(), result.timings_ms.end());
    result.p50 = result.timings_ms[iterations / 2];
    result.p95 = result.timings_ms[static_cast<size_t>(iterations * 0.95)];
    result.p99 = result.timings_ms[static_cast<size_t>(iterations * 0.99)];
    result.mean = std::accumulate(result.timings_ms.begin(), result.timings_ms.end(), 0.0) / iterations;
    
    return result;
}

void print_result(const BenchmarkResult& result, double target_ms = -1) {
    std::cout << "\n📊 " << result.name << "\n";
    std::cout << "   Mean:  " << std::fixed << std::setprecision(3) << result.mean << " ms\n";
    std::cout << "   p50:   " << result.p50 << " ms\n";
    std::cout << "   p95:   " << result.p95 << " ms";
    
    if (target_ms > 0) {
        if (result.p95 <= target_ms) {
            std::cout << " ✅ (target: " << target_ms << " ms)";
        } else {
            std::cout << " ❌ (target: " << target_ms << " ms, missed by " 
                     << (result.p95 - target_ms) << " ms)";
        }
    }
    std::cout << "\n";
    std::cout << "   p99:   " << result.p99 << " ms\n";
}

int main(int argc, char** argv) {
    int iterations = 1000;
    if (argc > 1) {
        iterations = std::atoi(argv[1]);
    }
    
    std::cout << "🚀 Brain-AI v3.6.0 - Performance Benchmark\n";
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "Iterations: " << iterations << "\n";
    
    // Test different dimensions
    std::vector<int> dimensions = {4, 7, 12, 15};
    
    for (int dim : dimensions) {
        auto result = benchmark_evolution(dim, iterations);
        
        // 10ms target for p95
        double target = 10.0;
        print_result(result, target);
    }
    
    // Summary
    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "📈 Performance Summary:\n";
    std::cout << "   Target: Evolution p95 ≤ 10ms (CPU)\n";
    
    auto dim7_result = benchmark_evolution(7, 100);
    if (dim7_result.p95 <= 10.0) {
        std::cout << "   Status: ✅ PASS (7D: " << dim7_result.p95 << " ms)\n";
    } else {
        std::cout << "   Status: ❌ FAIL (7D: " << dim7_result.p95 << " ms)\n";
    }
    
    return 0;
}
