#include "file_request_handler.h"

namespace http_handler {

fs::path FileRequestHandler::DecodePath(std::string path) {
    std::ostringstream decoded_stream;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '%' && i + 2 < path.size()) {
            std::string hex_str = path.substr(i + 1, 2);
            char decoded_char = static_cast<char>(std::strtol(hex_str.c_str(), nullptr, 16));
            decoded_stream << decoded_char;
            i += 2;
        } else if (path[i] == '+') {
            decoded_stream << ' ';
        } else {
            decoded_stream << path[i];
        }
    }
    return fs::path{ decoded_stream.str() };
}

bool FileRequestHandler::IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string_view FileRequestHandler::ProcessFileExtension(std::string path) {
    const size_t pos = path.rfind('.');
    std::string file_extension{ path.begin() + (pos != std::string::npos ? pos : path.size()), path.end() };
    std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), tolower);

    static const std::unordered_map<std::string, std::string_view> file_extension_to_content_type{
        {".htm", ContentType::TEXT_HTML},
        {".html", ContentType::TEXT_HTML},
        {".css", ContentType::TEXT_CSS},
        {".txt", ContentType::TEXT_PLAIN},
        {".js", ContentType::TEXT_JAVASCRIPT},
        {".json", ContentType::APPLICATION_JSON},
        {".xml", ContentType::APPLICATION_XML},
        {".png", ContentType::IMAGE_PNG},
        {".jpg", ContentType::IMAGE_JPEG},
        {".jpe", ContentType::IMAGE_JPEG},
        {".jpeg", ContentType::IMAGE_JPEG},
        {".gif", ContentType::IMAGE_GIF},
        {".bmp", ContentType::IMAGE_BMP},
        {".ico", ContentType::IMAGE_ICO},
        {".tiff", ContentType::IMAGE_TIFF},
        {".tif", ContentType::IMAGE_TIFF},
        {".svg", ContentType::IMAGE_SVG_XML},
        {".svgz", ContentType::IMAGE_SVG_XML},
        {".mp3", ContentType::AUDIO_MPEG}
    };

    auto it = file_extension_to_content_type.find(file_extension);
    return (it != file_extension_to_content_type.end()) ? it->second : ContentType::APPLICATION_OCTET_STREAM;
}

StringResponse FileRequestHandler::ReportFileBadRequest(unsigned http_version, bool keep_alive) {
    return MakeStringResponse(
        http::status::bad_request,
        INVALID_PATH_MESSAGE,
        http_version,
        keep_alive,
        ContentType::TEXT_PLAIN
    );
}

FileRequestResult FileRequestHandler::ReturnFileOrReportNotFound(
    fs::path dir,
    unsigned http_version,
    bool keep_alive
) {
    dir = fs::weakly_canonical(dir);
    std::string_view content_type = ProcessFileExtension(dir.string());
    http::file_body::value_type body;

    if (sys::error_code ec; body.open(dir.string().data(), beast::file_mode::read, ec), ec) {
        // файл не нашелся - 404 not found
        return MakeStringResponse(
            http::status::not_found,
            NOT_FOUND_FILE_MESSAGE,
            http_version,
            keep_alive,
            ContentType::TEXT_PLAIN
        );
    }
    // файл нашелся - 200 ok
    return MakeFileResponse(
        http::status::ok,
        std::move(body),
        http_version,
        keep_alive,
        content_type
    );
}

}  // namespace http_handler