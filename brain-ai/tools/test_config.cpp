#include "brain_ai/config.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::string config_path = argc > 1 ? argv[1] : "configs/system.yaml";
    
    std::cout << "Testing config loading from: " << config_path << std::endl;
    
    try {
        auto config = brain_ai::SystemConfig::load(config_path);
        std::cout << "✅ Config loaded successfully" << std::endl;
        std::cout << "   Quantum dimension: " << config.quantum.dimension << std::endl;
        std::cout << "   Memory backend: " << config.memory.backend << std::endl;
        std::cout << "   Graph enabled: " << config.graph.enable << std::endl;
        std::cout << "   gRPC port: " << config.server.grpc_port << std::endl;
        
        config.validate();
        std::cout << "✅ Config validation passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
