#pragma once
#include <vector>
#include <string>
#include <memory>
#include <Eigen/Dense>

namespace brain_ai {

struct RetrievalResult {
    std::vector<int64_t> ids;
    std::vector<float> distances;
    std::vector<std::vector<float>> vectors;
};

class MemoryIndex {
public:
    virtual ~MemoryIndex() = default;
    
    virtual void add(const std::vector<std::vector<float>>& vectors, 
                    const std::vector<int64_t>& ids) = 0;
    
    virtual RetrievalResult search(const std::vector<float>& query, int k) = 0;
    
    virtual void save(const std::string& path) = 0;
    virtual void load(const std::string& path) = 0;
    
    virtual size_t size() const = 0;
    virtual int dim() const = 0;
};

// Factory function
std::unique_ptr<MemoryIndex> create_memory_index(
    const std::string& backend, 
    int dim, 
    const std::string& config_str = ""
);

} // namespace brain_ai
