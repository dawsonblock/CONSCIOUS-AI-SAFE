#pragma once
#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>

namespace brain_ai {

class Metrics {
public:
    static Metrics& instance() {
        static Metrics m;
        return m;
    }
    
    // Counters
    void inc_counter(const std::string& name, int64_t val = 1);
    int64_t get_counter(const std::string& name) const;
    
    // Histograms (simple percentiles)
    void observe(const std::string& name, double value);
    double percentile(const std::string& name, double p) const;
    
    // Gauges
    void set_gauge(const std::string& name, double value);
    double get_gauge(const std::string& name) const;
    
    // Export Prometheus format
    std::string export_prometheus() const;
    
private:
    Metrics() = default;
    mutable std::mutex mu_;
    std::map<std::string, std::atomic<int64_t>> counters_;
    std::map<std::string, std::vector<double>> histograms_;
    std::map<std::string, std::atomic<double>> gauges_;
};

class Timer {
public:
    explicit Timer(const std::string& metric) : metric_(metric), 
        start_(std::chrono::steady_clock::now()) {}
    
    ~Timer() {
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration<double, std::milli>(end - start_).count();
        Metrics::instance().observe(metric_, dur);
    }
    
private:
    std::string metric_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace brain_ai
