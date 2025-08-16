# include "logger.h"

namespace logger {

void ServerLogFormater(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object log_message;

    // Add timestamp
    if (auto ts = rec[timestamp])
        log_message.emplace("timestamp", to_iso_extended_string(*ts));

    // Add additional data if present
    if (auto data = rec[additional_data])
        log_message.emplace("data", *data);

    // Add the log message
    if (auto msg = rec[expr::smessage])
        log_message.emplace("message", std::string(*msg));

    strm << log_message;
} 

void InitLogger() {
    logging::add_common_attributes();

    logging::add_console_log( 
        std::cout,
        keywords::format = &ServerLogFormater,
        keywords::auto_flush = true
    ); 
}

json::object ServerStartedData(const std::string& address, uint32_t port) {
    return {
        {"port", port},
        {"address", address}
    };
}

json::object ServerStopedData(int code, const std::optional<std::string>& exception_description) {
    if (exception_description.has_value()) {
        return {
            {"code", code},
            {"exception", exception_description.value()}
        };
    }
    return {
        {"code", 0}
    };
}

json::object ServerStopedData(int code) {
    return ServerStopedData(code, std::nullopt);
}

json::object ServerErrorData(int code, const std::string& text, const std::string& where) {
    return {
        {"code", code},
        {"text", text},
        {"where", where}
    };
}

json::object ServerRequestData(const std::string& ip, const std::string& uri, const std::string& method) {
    return {
        {"ip", ip},
        {"URI", uri},
        {"method", method}
    };
}

json::object ServerResponseData(int32_t response_duration, int code, const std::string& content_type) {
    return {
        {"response_time", response_duration},
        {"code", code},
        {"content_type", content_type}
    };
}

} // logger