#include "brain_ai/config.hpp"
#include "brain_ai/metrics.hpp"
#include <microhttpd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <atomic>

namespace brain_ai {

class HTTPMetricsServer {
public:
    HTTPMetricsServer(int port) : port_(port), daemon_(nullptr) {}
    
    ~HTTPMetricsServer() {
        stop();
    }
    
    bool start() {
        daemon_ = MHD_start_daemon(
            MHD_USE_SELECT_INTERNALLY,
            port_,
            nullptr, nullptr,
            &handle_request, this,
            MHD_OPTION_END
        );
        
        if (!daemon_) {
            std::cerr << "Failed to start HTTP server on port " << port_ << std::endl;
            return false;
        }
        
        std::cout << "✅ HTTP metrics server listening on :" << port_ << std::endl;
        return true;
    }
    
    void stop() {
        if (daemon_) {
            MHD_stop_daemon(daemon_);
            daemon_ = nullptr;
        }
    }
    
private:
    static MHD_Result handle_request(void* cls, struct MHD_Connection* connection,
                             const char* url, const char* method,
                             const char* version, const char* upload_data,
                             size_t* upload_data_size, void** con_cls) {
        
        HTTPMetricsServer* server = static_cast<HTTPMetricsServer*>(cls);
        
        if (strcmp(method, "GET") != 0) {
            return MHD_NO;
        }
        
        std::string response_data;
        int status_code = MHD_HTTP_OK;
        
        if (strcmp(url, "/metrics") == 0) {
            response_data = Metrics::instance().export_prometheus();
        } else if (strcmp(url, "/health") == 0) {
            response_data = "OK\n";
        } else if (strcmp(url, "/") == 0) {
            response_data = "Brain-AI v3.6.0 Metrics Server\n"
                          "Endpoints:\n"
                          "  /metrics - Prometheus format metrics\n"
                          "  /health  - Health check\n";
        } else {
            response_data = "404 Not Found\n";
            status_code = MHD_HTTP_NOT_FOUND;
        }
        
        struct MHD_Response* response = MHD_create_response_from_buffer(
            response_data.size(),
            (void*)response_data.c_str(),
            MHD_RESPMEM_MUST_COPY
        );
        
        MHD_add_response_header(response, "Content-Type", "text/plain");
        MHD_Result ret = (MHD_Result)MHD_queue_response(connection, status_code, response);
        MHD_destroy_response(response);
        
        return ret;
    }
    
    int port_;
    struct MHD_Daemon* daemon_;
};

} // namespace brain_ai
