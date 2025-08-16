#pragma once
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/json.hpp>

#include "http_server.h"

#include "response_utils.h"
#include "file_request_handler.h"
#include "api_request_handler.h"

#include <algorithm>
#include <iostream> //DELETE ME
#include <sstream>

namespace http_handler {

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:

    explicit RequestHandler(std::shared_ptr<ApiRequestHandler> api_handler, FileRequestHandler file_handler)
    : api_handler_{std::move(api_handler)}, file_handler_{std::move(file_handler)} 
    {

    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto http_version = req.version();
        auto keep_alive = req.keep_alive();
        std::string path = std::string(req.target());
        std::string prefix_for_api = "/api/"s;

        try {
            if (path.starts_with(prefix_for_api)) {
                // запрос к API
                api_handler_->HandleRequest(std::move(req), std::forward<Send>(send));
            } else {
                // запрос к файлам
                file_handler_.HandleRequest(std::move(req), std::forward<Send>(send));
            }
        } catch (...) {
            // Непредвиденная ошибка
            send(ReportServerError(http_version, keep_alive));
        }
    }

private:
    std::shared_ptr<ApiRequestHandler> api_handler_;
    FileRequestHandler file_handler_;
};

}  // namespace http_handler
