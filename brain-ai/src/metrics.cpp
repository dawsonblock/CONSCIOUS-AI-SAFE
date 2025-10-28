#include "brain_ai/metrics.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace brain_ai {

void Metrics::inc_counter(const std::string& name, int64_t val) {
    std::lock_guard<std::mutex> lock(mu_);
    counters_[name].fetch_add(val);
}

int64_t Metrics::get_counter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = counters_.find(name);
    return (it != counters_.end()) ? it->second.load() : 0;
}

void Metrics::observe(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mu_);
    histograms_[name].push_back(value);
}

double Metrics::percentile(const std::string& name, double p) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = histograms_.find(name);
    if (it == histograms_.end() || it->second.empty()) {
        return 0.0;
    }
    
    auto values = it->second;
    std::sort(values.begin(), values.end());
    size_t idx = static_cast<size_t>(p * values.size());
    if (idx >= values.size()) idx = values.size() - 1;
    return values[idx];
}

void Metrics::set_gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mu_);
    gauges_[name].store(value);
}

double Metrics::get_gauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = gauges_.find(name);
    return (it != gauges_.end()) ? it->second.load() : 0.0;
}

std::string Metrics::export_prometheus() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ostringstream oss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << value.load() << "\n";
    }
    
    // Histograms (export percentiles)
    for (const auto& [name, values] : histograms_) {
        if (!values.empty()) {
            auto sorted = values;
            std::sort(sorted.begin(), sorted.end());
            
            oss << "# TYPE " << name << " summary\n";
            oss << name << "{quantile=\"0.5\"} " 
                << sorted[sorted.size() / 2] << "\n";
            oss << name << "{quantile=\"0.95\"} " 
                << sorted[static_cast<size_t>(0.95 * sorted.size())] << "\n";
            oss << name << "{quantile=\"0.99\"} " 
                << sorted[static_cast<size_t>(0.99 * sorted.size())] << "\n";
            oss << name << "_count " << sorted.size() << "\n";
        }
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << std::fixed << std::setprecision(6) 
            << value.load() << "\n";
    }
    
    return oss.str();
}

} // namespace brain_ai
