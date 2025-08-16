#pragma once

#include <boost/asio/strand.hpp>
#include <boost/json.hpp>

#include "response_utils.h"
#include "application.h"
#include "model.h"
#include <regex>
#include <memory>

namespace http_handler {

namespace json = boost::json;
namespace net = boost::asio;

using Strand = net::strand<net::io_context::executor_type>;

static const std::regex case_maps{ R"(/api/v1/maps/?)" };

static const std::regex case_map{ R"(/api/v1/maps/(.+))" };

static const std::regex case_game{ R"(/api/v1/game/([^/]+)/?)" };

static const std::regex case_game_records{R"(/api/v1/game/records(?:\?([^#]+)?)?)"};

static const std::regex case_player_action{ R"(/api/v1/game/player/action/?)" };
// Допустимые значения для поля "move" запроса /api/v1/game/player/action/
static const std::unordered_set<std::string> valid_directions = { "U"s, "D"s, "R"s, "L"s, ""s };

class ApiRequestHandler : public std::enable_shared_from_this<ApiRequestHandler> {
public:

    explicit ApiRequestHandler(application::Application& application, Strand api_strand, bool is_tick_needed)
        : application_{application}, api_strand_{std::move(api_strand)}, is_tick_needed_(is_tick_needed) {
    }

    static std::shared_ptr<ApiRequestHandler> Create(application::Application& app, Strand strand, bool is_tick_needed) {
        return std::make_shared<ApiRequestHandler>(app, std::move(strand), is_tick_needed);
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto http_version = req.version();
        auto keep_alive = req.keep_alive();
        std::string auth_header{req[http::field::authorization]};
        std::string path = std::string(req.target());

        auto shared_req = std::make_shared<http::request<Body, http::basic_fields<Allocator>>>(std::move(req));
        auto shared_send = std::make_shared<std::decay_t<Send>>(std::forward<Send>(send));

        auto handler = [
            self = shared_from_this(),
            shared_send,
            shared_req,
            path,
            auth_header,
            http_version,
            keep_alive
        ]() {
            try {
                // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                assert(self->api_strand_.running_in_this_thread());
                // Получаем различные данные из запроса
                const std::string request_body = shared_req->body();
                http::verb request_method = shared_req->method();
                const std::string content_type = shared_req->count(http::field::content_type) ? std::string(shared_req->at(http::field::content_type)) : "";
                // Обрабатываем безопасные запросы (не изменяющие состояния игры)
                if (request_method == http::verb::get || request_method == http::verb::head) {
                    (*shared_send)(self->HandleSafeApiRequest(
                        path,
                        auth_header,
                        http_version, 
                        keep_alive
                    ));
                } else {
                    // Обрабатываем запросы изменяющие состояния игры
                    (*shared_send)(self->HandleChangingApiRequest(
                        path,
                        auth_header,
                        request_body,
                        request_method,
                        content_type,
                        http_version,
                        keep_alive
                    ));
                }
            } catch (...) {
                (*shared_send)(ReportServerError(http_version, keep_alive));
            }
        };
        // Ставим в очередь
        net::dispatch(
            api_strand_,
            [handler = std::move(handler)]() mutable {
                handler();
            }
        );
    }
    
private:
    application::Application& application_;
    Strand api_strand_;

    bool is_tick_needed_;

    // Выводит текст в формате JSON с красивым форматированием 
    void PrettyPrint(std::ostream& os, json::value const& jv, std::string* indent = nullptr);

    // Подготавливает тело JSON ответа с информацией о всех картах
    std::string BuildAllMapsRequestJSON();

    // Подготавливает StringResponse для /api/v1/maps/
    StringResponse HandleAllMapsRequest(unsigned http_version, bool keep_alive);

    // Подготавливает тело JSON ответа когда карта не найдена
    std::string BuildMapNotFoundJSON();

    // Подготавливает тело JSON ответа с информацией о конкретной карте
    std::string BuildMapRequestJSON(std::shared_ptr<model::Map> map);

    // Подготавливает StringResponse для /api/v1/map/{id}
    StringResponse HandleMapRequest(std::shared_ptr<model::Map> map, unsigned http_version, bool keep_alive);

    // Подготавливает тело JSON ответа - method not allowed
    std::string BuildMethodNotAllowedJSON(const std::string& message);

    // Подготавливает StringResponse для method not allowed
    StringResponse HandleMethodNotAllowed(const std::string& message, const std::string& allowed_methods,
        unsigned http_version, bool keep_alive
    );

    // Подготавливает тело JSON ответа - not authorized
    std::string BuildUnauthorizedRequestJSON(const std::string& code, const std::string& message);

    // Подготавливает StringResponse для not authorized
    StringResponse HandleUnauthorized(const std::string& auth_header, const std::string& code,
        const std::string& message, unsigned http_version, bool keep_alive
    );

    // Подготавливает тело JSON ответа - 200 на запрос api/v1/game/players/
    std::string BuildSuccessfullPlayersRequestJSON(std::vector<std::shared_ptr<application::Player>> coplayers);

    // Подготавливает StringResponse для 200 на запрос api/v1/game/players/
    StringResponse HandleSuccessfullPlayersRequest(std::shared_ptr<application::Player> player, unsigned http_version, bool keep_alive);

    // Подготавливает тело JSON ответа - 200 на запрос api/v1/game/state/
    std::string BuildSuccessfullStateRequestJSON(std::vector<std::shared_ptr<application::Player>> coplayers);

    // Подготавливает StringResponse для 200 на запрос api/v1/game/players/
    StringResponse HandleSuccessfullStateRequest(std::shared_ptr<application::Player> player, unsigned http_version, bool keep_alive);

    // Обрабатывает bad request запрос к API
    std::string BuildBadRequestJSON(const std::string& code, const std::string& message);

    // Подготавливает StringResponse для bad request
    StringResponse HandleBadRequest(const std::string& code, const std::string& message, unsigned http_version, bool keep_alive);

    // Обрабатывает ситуацию, когда при парсинге JSON или получении его свойств произошла ошибка
    std::string BuildPlayersParseErrorJSON(const std::string& message);

    // Подготавливает StringResponse когда при парсинге JSON или получении его свойств произошла ошибка
    StringResponse HandleParseJSONError(const std::string& message, unsigned http_version, bool keep_alive);

    // Подготавливает StringResponse для случая когда имя игрока пустое
    StringResponse HandleEmptyPlayerName(unsigned http_version, bool keep_alive);

    // Подготавливает StringResponse для случая когда карты с такий ID не существует
    StringResponse HandleMapNotFound(unsigned http_version, bool keep_alive);

    // Подготавливает тело JSON ответа - 200 на запрос api/v1/game/join/
    std::string BuildSuccessfullGameJoinRequestJSON(const std::string& authToken, size_t playerId);

    // Подготавливает StringResponse для 200 на запрос api/v1/game/join/
    StringResponse HandleSuccessfullGameJoinRequest(const std::string& player_name, const std::string& map_id,
        unsigned http_version, bool keep_alive
    );

    StringResponse HandleSuccessfullPlayerActionRequest(std::shared_ptr<application::Player> player, const std::string& move_direction,
        unsigned http_version, bool keep_alive
    );

    StringResponse HandleSuccessfullTickRequest(int64_t time_delta, unsigned http_version, bool keep_alive);

    std::string BuildRecordsRequestJSON(const std::vector<domain::Player>& players);
    
    StringResponse HandleRecordsRequest(const std::unordered_map<std::string, std::string>& params, unsigned http_version, bool keep_alive);

    // Обрабатывает запросы к API, НЕ изменяющие состояние игры
    StringResponse HandleSafeApiRequest(const std::string& path, const std::string& auth_header,
        unsigned http_version, bool keep_alive
    );

    // Обрабатывает запросы к API, изменяющие состояние игры
    StringResponse HandleChangingApiRequest(const std::string& path, const std::string& auth_header,
        const std::string& request_body, http::verb request_method, const std::string& content_type,
        unsigned http_version, bool keep_alive
    );
};

}  // namespace http_handler