#include "brain_ai/core/memory_index.hpp"
#include <hnswlib/hnswlib.h>
#include <stdexcept>

namespace brain_ai {

class HNSWIndex : public MemoryIndex {
public:
    HNSWIndex(int dim, const std::string& config) 
        : dim_(dim), space_(dim), index_(&space_, 100000, 32, 200) {
        index_.setEf(64); // Search parameter
    }
    
    void add(const std::vector<std::vector<float>>& vectors, 
             const std::vector<int64_t>& ids) override {
        for (size_t i = 0; i < vectors.size(); ++i) {
            index_.addPoint(vectors[i].data(), ids[i]);
        }
    }
    
    RetrievalResult search(const std::vector<float>& query, int k) override {
        auto result = index_.searchKnn(query.data(), k);
        
        RetrievalResult ret;
        while (!result.empty()) {
            auto [dist, id] = result.top();
            ret.distances.push_back(dist);
            ret.ids.push_back(id);
            result.pop();
        }
        
        std::reverse(ret.distances.begin(), ret.distances.end());
        std::reverse(ret.ids.begin(), ret.ids.end());
        
        return ret;
    }
    
    void save(const std::string& path) override {
        index_.saveIndex(path);
    }
    
    void load(const std::string& path) override {
        index_.loadIndex(path, &space_);
    }
    
    size_t size() const override {
        return index_.getCurrentElementCount();
    }
    
    int dim() const override {
        return dim_;
    }
    
private:
    int dim_;
    hnswlib::L2Space space_;
    hnswlib::HierarchicalNSW<float> index_;
};

std::unique_ptr<MemoryIndex> create_hnsw_index(int dim, const std::string& config) {
    return std::make_unique<HNSWIndex>(dim, config);
}

} // namespace brain_ai
