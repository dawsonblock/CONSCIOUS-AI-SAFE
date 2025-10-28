#include "brain_ai/config.hpp"
#include "brain_ai/core/qw_simple.hpp"
#include "brain_ai/core/memory_index.hpp"
#include "brain_ai/core/graph.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::string config_path = argc > 1 ? argv[1] : "configs/system.yaml";
    
    std::cout << "Step 1: Loading config..." << std::endl;
    auto config = brain_ai::SystemConfig::load(config_path);
    std::cout << "✅ Config loaded" << std::endl;
    
    std::cout << "Step 2: Creating quantum workspace..." << std::endl;
    brain::QuantumWorkspace qw(config.quantum.dimension);
    std::cout << "✅ QW created, entropy: " << qw.entropy() << std::endl;
    
    std::cout << "Step 3: Creating memory index (" << config.memory.backend << ")..." << std::endl;
    auto memory = brain_ai::create_memory_index(config.memory.backend, config.embeddings.dim);
    std::cout << "✅ Memory created, size: " << memory->size() << std::endl;
    
    std::cout << "Step 4: Creating graph..." << std::endl;
    brain_ai::ConnectionGraph graph(config.sqlite.db_path);
    std::cout << "✅ Graph created, nodes: " << graph.node_count() << std::endl;
    
    std::cout << "\\n✅ All components initialized successfully!" << std::endl;
    
    return 0;
}
