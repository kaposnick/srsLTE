#include "srsenb/hdr/metrics_http_scrape.h"
#include <iomanip>

using namespace std;

namespace srsenb {

static std::string toPrometheusLabel(const std::string & stackLabel, const std::string & metricLabel) {
    std::string result;
    result = std::string("srsenb_") + stackLabel + "_" + metricLabel;
    return result;
}

static std::string getEnbSelector(std::string & enbId) {
    std::string result;
    result = std::string("enb_id=\"") + enbId + "\"";
    return result;
}

// static std::string getUeSelector(const uint32_t ueId) {
//     std::string result;
//     result += std::string("ue_id=\"") + std::to_string(ueId) + "\"";
//     return result;
// }

static MHD_Result request_handler(void* cls, 
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* version,
    const char* upload_data,
    size_t* upload_data_size,
    void** ptr) {
        cout << "Request: " << url << ", Method: " << method << endl;;

        if (0 != strcmp(method, "GET") || 0 != strcmp(url, "/metrics")) {
            struct MHD_Response* response = MHD_create_response_from_buffer(0, 0, MHD_RESPMEM_PERSISTENT);
            return MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
        }

        metrics_http_scrape* metrics_provider = (metrics_http_scrape *) cls;

        std::string enbId = metrics_provider->getEnbId();
        const enb_metrics_t metrics = metrics_provider->getLatestMetrics();

        
        std::string resBody;
        resBody = toPrometheusLabel("rf", "error_overflow") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string(metrics.rf.rf_o) + "\n";
        resBody += (toPrometheusLabel("rf", "error_underflow") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string(metrics.rf.rf_u) + "\n");
        resBody += (toPrometheusLabel("rf", "error_late") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string(metrics.rf.rf_l) + "\n");
        
        struct MHD_Response* response;

        const char * var = resBody.c_str();
        
        response = MHD_create_response_from_buffer(resBody.length(), (void* ) resBody.c_str(), MHD_RESPMEM_MUST_COPY);
        MHD_Result result;
        result = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return result;
    }


metrics_http_scrape::metrics_http_scrape() {

}

metrics_http_scrape::~metrics_http_scrape() {
    stop();
}

void metrics_http_scrape::init (const std::string & bindIp, const uint32_t bindPort) {
    d = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION, 
        bindPort,
        NULL, NULL,
        &srsenb::request_handler, this,
        MHD_OPTION_END);
    if (d != NULL) {
        cout << "HTTP Server started for port " << bindPort << endl;
    }
}

void metrics_http_scrape::stop() {
    if (d != NULL) {
        MHD_stop_daemon(d);
        cout << "HTTP Server stopped" << endl;
        d = NULL;
    }
}

void metrics_http_scrape::set_handle(const uint32_t enbId_) {
    std::stringstream ss;
    ss << std::string("0x") << std::hex << (int) enbId_ ;

    std::string s = ss.str();
    enbId = s;
    
    cout  << "enbId: " << enbId << endl;
}

std::string metrics_http_scrape::getEnbId() {
    return enbId;
}

const enb_metrics_t & metrics_http_scrape::getLatestMetrics() {
    return metrics;
}

void metrics_http_scrape::set_metrics(const enb_metrics_t& m_, const uint32_t period_usec) {
    metrics = m_;
}

}