#include "response_utils.h"

namespace http_handler {

StringResponse MakeStringResponse(
    http::status status,
    std::string_view body,
    unsigned http_version,
    bool keep_alive,
    std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
}

FileResponse MakeFileResponse(
    http::status status,
    http::file_body::value_type body,
    unsigned http_version,
    bool keep_alive,
    std::string_view content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = std::move(body);
        response.keep_alive(keep_alive);
        response.prepare_payload();
        return response;
}

StringResponse ReportServerError(unsigned http_version, bool keep_alive) {
    return MakeStringResponse(
        http::status::internal_server_error,
        "Internal Server Error! The server encountered an unexpected condition! Try again later!",
        http_version,
        keep_alive,
        ContentType::TEXT_PLAIN
    );
}

}  // namespace http_handler