#pragma once

#include "response_utils.h"

#include <filesystem>
#include <unordered_map>
#include <variant>

namespace http_handler {

namespace fs = std::filesystem;
namespace sys = boost::system;

constexpr static std::string_view INVALID_PATH_MESSAGE = "Invalid path!"sv;
constexpr static std::string_view NOT_FOUND_FILE_MESSAGE = "File not found!"sv;

using FileRequestResult = std::variant<StringResponse, FileResponse>;

class FileRequestHandler {
public:

    explicit FileRequestHandler(fs::path static_files_dir)
        : static_files_dir_{std::move(static_files_dir)} {
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto http_version = req.version();
        auto keep_alive = req.keep_alive();
        std::string path = std::string(req.target());

        if (path.back() == '/') {
            path += "index.html"s;
        }
        
        fs::path decoded_path = DecodePath(path);
        fs::path dir = static_files_dir_ / decoded_path.relative_path();
    
        if (IsSubPath(dir, static_files_dir_)) {
            // Возвращем файл (если такой существует) или 404 not_found
            std::visit(
                [&send](auto&& result) { send(std::forward<decltype(result)>(result)); },
                ReturnFileOrReportNotFound(dir, http_version, keep_alive)
            );
        } else {
            // Вышли за пределы корневого каталога - 400 bad requset
            send(ReportFileBadRequest(http_version, keep_alive));
        }
    }

private:
    fs::path static_files_dir_;

    // Декодирует percent-encoding (а также +=' ')
    fs::path DecodePath(std::string path);

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    // Получает и обрабатывает расширение файла, возвращает Content-Type
    std::string_view ProcessFileExtension(std::string path);

    // Создает StringResponse в случае bad request к файлам
    StringResponse ReportFileBadRequest(unsigned http_version, bool keep_alive);

    // Возвращет файл либо 404 not found, если файл по пути dir не найден
    FileRequestResult ReturnFileOrReportNotFound(fs::path dir, unsigned http_version, bool keep_alive);
};

}  // namespace http_handler