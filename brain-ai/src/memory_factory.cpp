#include "brain_ai/core/memory_index.hpp"

namespace brain_ai {

// Forward declarations
std::unique_ptr<MemoryIndex> create_faiss_index(int dim, const std::string& config);
std::unique_ptr<MemoryIndex> create_hnsw_index(int dim, const std::string& config);

std::unique_ptr<MemoryIndex> create_memory_index(
    const std::string& backend, 
    int dim, 
    const std::string& config_str
) {
    if (backend == "faiss") {
        return create_faiss_index(dim, config_str);
    } else if (backend == "hnswlib") {
        return create_hnsw_index(dim, config_str);
    } else if (backend == "sqlite_vss") {
        throw std::runtime_error("SQLite-VSS not yet implemented");
    } else if (backend == "qdrant") {
        throw std::runtime_error("Qdrant not yet implemented");
    } else {
        throw std::runtime_error("Unknown backend: " + backend);
    }
}

} // namespace brain_ai
