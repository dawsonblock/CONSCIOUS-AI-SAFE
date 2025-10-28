#include "brain_ai/core/memory_index.hpp"
#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_io.h>
#include <stdexcept>

namespace brain_ai {

class FAISSIndex : public MemoryIndex {
public:
    FAISSIndex(int dim, const std::string& config) : dim_(dim) {
        // Use simple Flat index for now (exact search)
        // IVF+PQ requires training data which we don't have at startup
        index_ = new faiss::IndexFlatL2(dim);
    }
    
    ~FAISSIndex() override {
        delete index_;
    }
    
    void add(const std::vector<std::vector<float>>& vectors, 
             const std::vector<int64_t>& ids) override {
        if (vectors.empty()) return;
        
        // Flatten vectors
        std::vector<float> flat;
        flat.reserve(vectors.size() * dim_);
        for (const auto& v : vectors) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        
        // Add with IDs (Flat index doesn't need training)
        index_->add_with_ids(vectors.size(), flat.data(), ids.data());
    }
    
    RetrievalResult search(const std::vector<float>& query, int k) override {
        RetrievalResult result;
        result.ids.resize(k);
        result.distances.resize(k);
        
        index_->search(1, query.data(), k, result.distances.data(), 
                      reinterpret_cast<faiss::idx_t*>(result.ids.data()));
        
        return result;
    }
    
    void save(const std::string& path) override {
        faiss::write_index(index_, path.c_str());
    }
    
    void load(const std::string& path) override {
        delete index_;
        index_ = faiss::read_index(path.c_str());
        if (!index_) {
            throw std::runtime_error("Failed to load FAISS index");
        }
    }
    
    size_t size() const override {
        return index_->ntotal;
    }
    
    int dim() const override {
        return dim_;
    }
    
private:
    int dim_;
    faiss::IndexFlatL2* index_ = nullptr;
};

std::unique_ptr<MemoryIndex> create_faiss_index(int dim, const std::string& config) {
    return std::make_unique<FAISSIndex>(dim, config);
}

} // namespace brain_ai
