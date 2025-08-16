#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <string_view>

namespace http_handler {

using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;

// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ в виде фалйа
using FileResponse = http::response<http::file_body>;

struct ContentType {
    ContentType() = delete;

    // При необходимости внутрь ContentType можно добавить и другие типы контента
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_CSS = "text/css"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view TEXT_JAVASCRIPT = "text/javascript"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
    constexpr static std::string_view APPLICATION_XML = "application/xml"sv;
    constexpr static std::string_view IMAGE_PNG = "image/png"sv;
    constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;
    constexpr static std::string_view IMAGE_GIF = "image/gif"sv;
    constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;
    constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;
    constexpr static std::string_view IMAGE_SVG_XML = "image/svg+xml"sv;
    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;
    constexpr static std::string_view APPLICATION_OCTET_STREAM = "application/octet-stream"sv;
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(
    http::status status,
    std::string_view body,
    unsigned http_version,
    bool keep_alive,
    std::string_view content_type = ContentType::TEXT_HTML);

// Создаёт FileResponse с заданными параметрами
FileResponse MakeFileResponse(
    http::status status,
    http::file_body::value_type body,
    unsigned http_version,
    bool keep_alive,
    std::string_view content_type = ContentType::TEXT_JAVASCRIPT);

// В случае непредвиденной ошибки с обработкой запроса, возвращается 500 - internal server error
StringResponse ReportServerError(unsigned http_version, bool keep_alive);

}  // namespace http_handler