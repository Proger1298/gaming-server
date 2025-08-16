#include "api_request_handler.h"

#include <iostream> //KILL ME

namespace http_handler {

    void ApiRequestHandler::PrettyPrint(std::ostream& os, json::value const& jv, std::string* indent) {
        std::string indent_;
        if (!indent)
            indent = &indent_;
        switch (jv.kind())
        {
        case json::kind::object:
        {
            os << "{\n";
            indent->append(4, ' '); // Увеличиваем отступ
            const json::object& obj = jv.get_object();
            bool first = true;
            for (const auto& [key, value] : obj) {
                if (!first) {
                    os << ",\n"; // Добавляем запятую между элементами
                }
                os << *indent << json::serialize(key) << " : ";
                PrettyPrint(os, value, indent);
                first = false;
            }
            indent->resize(indent->size() - 4); // Уменьшаем отступ
            os << "\n" << *indent << "}";
            break;
        }

        case json::kind::array:
        {
            os << "[\n";
            indent->append(4, ' '); // Увеличиваем отступ
            const json::array& arr = jv.get_array();

            bool first = true;
            for (const json::value& item : arr) {
                if (!first) {
                    os << ",\n"; // Добавляем запятую между элементами
                }
                os << *indent;
                PrettyPrint(os, item, indent);
                first = false;
            }
            indent->resize(indent->size() - 4); // Уменьшаем отступ
            os << "\n" << *indent << "]";
            break;
        }

        case json::kind::string:
        {
            os << json::serialize(jv.get_string());
            break;
        }

        case json::kind::uint64:
        case json::kind::int64:
        case json::kind::double_:
            os << jv;
            break;

        case json::kind::bool_:
            os << (jv.get_bool() ? "true" : "false");
            break;

        case json::kind::null:
            os << "null";
            break;
        }

        if (indent->empty())
            os << "\n";
    }

    std::string ApiRequestHandler::BuildAllMapsRequestJSON() {
        std::ostringstream ost;
        json::array answer;
        for (std::shared_ptr<model::Map> game_map : application_.GetMaps()) {
            answer.emplace_back(json::object{
                {"id"s,  *(game_map->GetId())},
                {"name"s, game_map->GetName()}
                });
        }
        PrettyPrint(ost, answer);
        return ost.str();
    }

    // Подготавливает StringResponse для /api/v1/maps/
    StringResponse ApiRequestHandler::HandleAllMapsRequest(unsigned http_version, bool keep_alive) {
        std::ostringstream ost;
        std::string json_str = BuildAllMapsRequestJSON();
        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            BuildAllMapsRequestJSON(),
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );
        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);
        
        return str_resp;
    }

    std::string ApiRequestHandler::BuildMapNotFoundJSON() {
        std::ostringstream ost;
        json::object answer {
            {"code"s, "mapNotFound"s},
            {"message"s, "Map not found"s}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    std::string ApiRequestHandler::BuildMapRequestJSON(std::shared_ptr<model::Map> map) {
        std::ostringstream ost;
        json::object answer;

        if (!map) {
            return BuildMapNotFoundJSON();
        }

        json::array roads;
        for (std::shared_ptr<model::Road> game_road : map->GetRoads()) {
            if (game_road->IsHorizontal()) {
                roads.emplace_back(json::object{
                    {"x0", game_road->GetStart().x},
                    {"y0", game_road->GetStart().y},
                    {"x1", game_road->GetEnd().x}
                });
            }
            else {
                roads.emplace_back(json::object{
                    {"x0", game_road->GetStart().x},
                    {"y0", game_road->GetStart().y},
                    {"y1", game_road->GetEnd().y}
                });
            }
        }

        json::array buildings;
        for (const model::Building& game_building : map->GetBuildings()) {
            buildings.emplace_back(json::object{
                {"x"s, game_building.GetBounds().position.x},
                {"y"s, game_building.GetBounds().position.y},
                {"w"s, game_building.GetBounds().size.width},
                {"h"s, game_building.GetBounds().size.height}
            });
        }

        json::array officies;
        for (const model::Office& game_office : map->GetOffices()) {
            officies.emplace_back(json::object{
                {"id"s, *(game_office.GetId())},
                {"x"s, game_office.GetPosition().x},
                {"y"s, game_office.GetPosition().y},
                {"offsetX"s, game_office.GetOffset().dx},
                {"offsetY"s, game_office.GetOffset().dy}
            });
        }

        json::array loot_types;
        const std::vector<extra_data::LootTypes::LootType>& cur_map_loot_types = application_.GetLootTypes().GetCurrentMapLootTypes(*(map->GetId()));
        for (const extra_data::LootTypes::LootType& loot_type : cur_map_loot_types) {
            loot_types.emplace_back(json::object{
                {"name"s, loot_type.name},
                {"file"s, loot_type.file},
                {"type"s, loot_type.type},
                {"scale"s, loot_type.scale},
                {"value"s, loot_type.value},
            });
            if (loot_type.rotation.has_value()) {
                loot_types.back().as_object().emplace("rotation"s, loot_type.rotation.value());
            }
            if (loot_type.color.has_value()) {
                loot_types.back().as_object().emplace("color"s, loot_type.color.value());
            }
        }

        answer = {
            {"id"s, *(map->GetId())},
            {"name"s, map->GetName()},
            {"roads"s, roads},
            {"buildings"s, buildings},
            {"offices"s, officies},
            {"lootTypes"s, loot_types}
        };

        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleMapRequest(std::shared_ptr<model::Map> map, unsigned http_version, bool keep_alive) {
        std::ostringstream ost;
        std::string json_str = BuildMapRequestJSON(map);
        StringResponse str_resp = MakeStringResponse(
            map ? http::status::ok : http::status::not_found,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );
        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);
        
        return str_resp;
    }

    std::string ApiRequestHandler::BuildMethodNotAllowedJSON(const std::string& message) {
        std::ostringstream ost;
        json::object answer{
            {"code"s, "invalidMethod"s},
            {"message"s, message}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleMethodNotAllowed(const std::string& message, const std::string& allowed_methods,
        unsigned http_version, bool keep_alive
    ) {
        std::ostringstream ost;
        std::string json_str = BuildMethodNotAllowedJSON(message);
        StringResponse str_resp = MakeStringResponse(
            http::status::method_not_allowed,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );
        str_resp.set(http::field::allow, allowed_methods);
        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);
        
        return str_resp;
    }

    std::string ApiRequestHandler::BuildUnauthorizedRequestJSON(const std::string& code, const std::string& message) {
        std::ostringstream ost;
        json::object answer{
            {"code"s, code},
            {"message"s,  message}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleUnauthorized(
        const std::string& auth_header,
        const std::string& code,
        const std::string& message,
        unsigned http_version,
        bool keep_alive
    ) {
        std::string json_str = BuildUnauthorizedRequestJSON(code, message);
        StringResponse str_resp = MakeStringResponse(
            http::status::unauthorized,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );
        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildSuccessfullPlayersRequestJSON(std::vector<std::shared_ptr<application::Player>> coplayers) {
        std::ostringstream ost;
        json::object answer;
        for (std::shared_ptr<application::Player> player : coplayers) {
            answer.emplace(std::to_string(*(player->GetId())), json::object{ {"name"s, player->GetName()} });
        }
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleSuccessfullPlayersRequest(std::shared_ptr<application::Player> player, unsigned http_version,bool keep_alive) {
        std::vector<std::shared_ptr<application::Player>> coplayers = application_.GetCurrentPlayerGameSessionPlayers(player);
        std::string json_str = BuildSuccessfullPlayersRequestJSON(coplayers);

        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildSuccessfullStateRequestJSON(std::vector<std::shared_ptr<application::Player>> coplayers) {
        std::ostringstream ost;
        // Добавляем информацию о собаках
        json::object dogs_info;
        for (std::shared_ptr<application::Player> player : coplayers) {
            std::shared_ptr<model::Dog> dog = player->GetDog();

            // Собираем информацию о предметах в рюкзаке
            json::array bag_items;
            for (const auto& item : dog->GetBag().GetItems()) {
                bag_items.emplace_back(json::object{
                    {"id"s, *item.GetId()},
                    {"type"s, item.GetType()}
                });
            }

            dogs_info.emplace(
                std::to_string(*(player->GetId())),
                json::object{
                    { "pos"s, json::array{dog->GetDogPosition().x, dog->GetDogPosition().y} },
                    { "speed"s, json::array{dog->GetDogSpeed().v_x, dog->GetDogSpeed().v_y} },
                    { "dir"s, dog->GetStringDirection() },
                    { "bag"s, bag_items },
                    { "score"s, dog->GetScore()}
                }
            );
        }

        // Добавляем информацию о потерянных предметах
        json::object lost_objects;
        if (!coplayers.empty()) {
            std::shared_ptr<model::GameSession> session = coplayers.front()->GetSession();
            for (const auto& [id, obj] : session->GetLostObjects()) {
                lost_objects.emplace(
                    std::to_string(*id),
                    json::object{
                        {"type"s, obj.GetType()},
                        {"pos"s, json::array{obj.GetPosition().x, obj.GetPosition().y}}
                    }
                );
            }
        }

        json::object answer{
            {"players"s, dogs_info},
            {"lostObjects"s, lost_objects}
        };

        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleSuccessfullStateRequest(std::shared_ptr<application::Player> player, unsigned http_version, bool keep_alive) {
        std::vector<std::shared_ptr<application::Player>> coplayers = application_.GetCurrentPlayerGameSessionPlayers(player);
        std::string json_str = BuildSuccessfullStateRequestJSON(coplayers);

        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildBadRequestJSON(const std::string& code, const std::string& message) {
        std::ostringstream ost;
        json::object answer{
            {"code"s, code},
            {"message"s,  message}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleBadRequest(const std::string& code, const std::string& message, unsigned http_version, bool keep_alive) {
        std::string json_str = BuildBadRequestJSON(code, message);
        StringResponse str_resp = MakeStringResponse(
            http::status::bad_request,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildPlayersParseErrorJSON(const std::string& message) {
        std::ostringstream ost;
        json::object answer{
            {"code"s, "invalidArgument"s},
            {"message"s, message}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleParseJSONError(const std::string& message, unsigned http_version, bool keep_alive) {
        std::string json_str = BuildPlayersParseErrorJSON(message);
        StringResponse str_resp = MakeStringResponse(
            http::status::bad_request,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    // Подготавливает StringResponse для случая когда имя игрока пустое
    StringResponse ApiRequestHandler::HandleEmptyPlayerName(unsigned http_version, bool keep_alive) {
        std::string json_str = BuildBadRequestJSON("invalidArgument"s, "Invalid name"s);

        StringResponse str_resp = MakeStringResponse(
            http::status::bad_request,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    // Подготавливает StringResponse для случая когда карты с такий ID не существует
    StringResponse ApiRequestHandler::HandleMapNotFound(unsigned http_version, bool keep_alive) {
        std::string json_str = BuildMapNotFoundJSON();

        StringResponse str_resp = MakeStringResponse(
            http::status::not_found,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildSuccessfullGameJoinRequestJSON(const std::string& authToken, size_t playerId) {
        std::ostringstream ost;
        json::object answer{
            {"authToken"s, authToken},
            {"playerId"s,  playerId}
        };
        PrettyPrint(ost, answer);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleSuccessfullGameJoinRequest(const std::string& player_name, const std::string& map_id,
        unsigned http_version,bool keep_alive
    ) {
        auto [token, player_id] = application_.JoinGame(player_name, model::Map::Id{ map_id });
        std::string json_str = BuildSuccessfullGameJoinRequestJSON(*token, *player_id);

        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    StringResponse ApiRequestHandler::HandleSuccessfullPlayerActionRequest(std::shared_ptr<application::Player> player, const std::string& move_direction,
        unsigned http_version, bool keep_alive
    ) {
        player->GetDog()->MoveDog(move_direction, player->GetSession()->GetMap()->GetDogSpeedOnMap());
        std::string json_str = "{}"s;

        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    StringResponse ApiRequestHandler::HandleSuccessfullTickRequest(int64_t time_delta, unsigned http_version, bool keep_alive) {
        application_.Tick(std::chrono::milliseconds{time_delta});

        std::string json_str = "{}"s;
        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }

    std::string ApiRequestHandler::BuildRecordsRequestJSON(const std::vector<domain::Player>& records) {
        std::ostringstream ost;
        json::array records_json;
        for (const domain::Player& record : records) {
            records_json.push_back(json::object{
                {"name", record.GetName()},
                {"score", record.GetScore()},
                {"playTime", record.GetPlayTime()}
            });
        }
        PrettyPrint(ost, records_json);
        return ost.str();
    }

    StringResponse ApiRequestHandler::HandleRecordsRequest(
        const std::unordered_map<std::string, std::string>& params,
        unsigned http_version,
        bool keep_alive
    ) {
        // Парсим параметры
        size_t start = 0;
        size_t max_items = 100;

        if (params.count("start")) {
            start = std::stoul(params.at("start"));
        }
        if (params.count("maxItems")) {
            max_items = std::stoul(params.at("maxItems"));
            if (max_items > 100) {
                return HandleBadRequest(
                    "invalidArgument",
                    "maxItems cannot exceed 100",
                    http_version,
                    keep_alive
                );
            }
        }
        
        // Получаем данные через use cases
        std::vector<domain::Player> records = application_.GetRecordsTable(start, max_items);

        std::string json_str = BuildRecordsRequestJSON(records);

        StringResponse str_resp = MakeStringResponse(
            http::status::ok,
            json_str,
            http_version,
            keep_alive,
            ContentType::APPLICATION_JSON
        );

        str_resp.set(http::field::content_length, std::to_string(json_str.size()));
        str_resp.set(http::field::cache_control, "no-cache"s);

        return str_resp;
    }
    
    //
    // TWO MAIN FUCNTIONS
    //

    StringResponse ApiRequestHandler::HandleSafeApiRequest(const std::string& path, const std::string& auth_header,
        unsigned http_version, bool keep_alive
    ) {

        std::smatch match;

        if (std::regex_match(path, match, case_maps)) {
            // api/v1/maps/ or api/v1/maps
            return HandleAllMapsRequest(http_version,keep_alive);
        } else if (std::regex_match(path, match, case_map)) {
            // api/v1/maps/{id-карты}
            std::shared_ptr<model::Map> map = application_.FindMap(model::Map::Id{ match[1] });
            return HandleMapRequest(map, http_version,keep_alive);
        } else if (std::regex_match(path, match, case_game_records)) {
            std::unordered_map<std::string, std::string> params;
            std::stringstream ss(match[1]);
            std::string item;
            
            while (std::getline(ss, item, '&')) {
                auto pos = item.find('=');
                if (pos != std::string::npos) {
                    auto key = item.substr(0, pos);
                    auto value = item.substr(pos + 1);
                    params[key] = value;
                }
            }
            return HandleRecordsRequest(params, http_version, keep_alive);
        } else if (std::regex_match(path, match, case_game)) {
            // Получили НЕ POST запрос на /api/v1/game/..
            
            if (match[1] == "join"s) {
                // /api/v1/game/join/ - METHOD NOT ALLOWED
                return HandleMethodNotAllowed("Only POST method is expected"s, "POST"s, http_version, keep_alive);
            } else if (match[1] == "tick"s) {
                // /api/v1/game/tick/ - METHOD NOT ALLOWED
                if (!is_tick_needed_) {
                    return HandleBadRequest("badRequest"s, "Invalid endpoint"s, http_version, keep_alive);
                }
                return HandleMethodNotAllowed("Only POST method is expected"s, "POST"s, http_version, keep_alive);
            } else if (match[1] == "players") {
                // /api/v1/game/players/ - обрабатываем дальше
                
                // Обрабатываем случай невалидного токена
                if (
                    !auth_header.size() ||
                    !auth_header.starts_with(application::detail::TOKEN_STRATS_WITH) ||
                    auth_header.size() != application::detail::TOKEN_SIZE
                ) {
                    return HandleUnauthorized(
                        auth_header,
                        "invalidToken"s,
                        "Authorization header is missing"s,
                        http_version,
                        keep_alive
                    );
                }

                // На этом этапе токен точно валидный
                application::Token token{ auth_header.substr(application::detail::TOKEN_POS_TO_DISCARD_BEARER) };

                // Находим игрока по токену
                std::shared_ptr<application::Player> player = application_.FindPlayerByToken(token);

                // Обрабатываем случай, когда игрок по токену не найден
                if (!player) {
                    return HandleUnauthorized(
                        auth_header,
                        "unknownToken"s,
                        "Player token has not been found"s,
                        http_version,
                        keep_alive
                    );
                }

                // На этом этапе игрок найден, всё готово для 200 ответа
                return HandleSuccessfullPlayersRequest(player, http_version, keep_alive);
            } else if (match[1] == "state") {
                // /api/v1/game/state/ - обрабатываем дальше

                // Обрабатываем случай невалидного токена
                if (
                    !auth_header.size() ||
                    !auth_header.starts_with(application::detail::TOKEN_STRATS_WITH) ||
                    auth_header.size() != application::detail::TOKEN_SIZE
                ) {
                    return HandleUnauthorized(
                        auth_header,
                        "invalidToken"s,
                        "Authorization header is missing"s,
                        http_version,
                        keep_alive
                    );
                }

                // На этом этапе токен точно валидный
                application::Token token{ auth_header.substr(application::detail::TOKEN_POS_TO_DISCARD_BEARER) };

                // Находим игрока по токену
                std::shared_ptr<application::Player> player = application_.FindPlayerByToken(token);

                // Обрабатываем случай, когда игрок по токену не найден
                if (!player) {
                    return HandleUnauthorized(
                        auth_header,
                        "unknownToken"s,
                        "Player token has not been found"s,
                        http_version,
                        keep_alive
                    );
                }

                // На этом этапе игрок найден, всё готово для 200 ответа
                return HandleSuccessfullStateRequest(player, http_version, keep_alive);
            }
        } else if (std::regex_match(path, match, case_player_action)) {
            // safe запрос на /api/v1/game/player/action - method not allowed
            return HandleMethodNotAllowed("Invalid method"s, "POST"s, http_version, keep_alive);
        }
        // bad request к API, который не изменяет состояние игры
        return HandleBadRequest("badRequest"s, "Bad request"s, http_version, keep_alive);
    }

    StringResponse ApiRequestHandler::HandleChangingApiRequest(const std::string& path, const std::string& auth_header,
        const std::string& request_body, http::verb request_method, const std::string& content_type,
        unsigned http_version, bool keep_alive
    ) {

        std::smatch match;

        if (std::regex_match(path, match, case_maps) || std::regex_match(path, match, case_map)) {
            // Получили POST на url, не изменяющий состояние игры (/api/v1/maps/ например)
            // Вернем method not allowed
            return HandleMethodNotAllowed(
                "Invalid method"s,
                "GET, HEAD"s,
                http_version,
                keep_alive
            );
        } else if (std::regex_match(path, match, case_game_records)) {
            return HandleMethodNotAllowed(
                "Invalid method"s,
                "GET, HEAD"s,
                http_version,
                keep_alive
            );
        } else if (std::regex_match(path, match, case_game)) {
            
            if (match[1] == "join"s) {
                // /api/v1/game/join/ - обрабатываем дальше
                if (request_method != http::verb::post) {
                    // /api/v1/game/join/ - METHOD NOT ALLOWED
                    return HandleMethodNotAllowed(
                        "Only POST method is expected"s,
                        "POST"s,
                        http_version,
                        keep_alive
                    );
                }
                // Получили POST запрос на /api/v1/game/join/

                json::value json_data;
                std::string player_name;
                std::string map_id;

                try {
                    // Парсим JSON
                    json_data = json::parse(request_body);
                    if (json_data.is_null()) {
                        // Ошибка при парсинге, возможно передали не JSON
                        return HandleParseJSONError("Invalid Join game JSON: parsed data is null!"s, http_version, keep_alive);
                    }
                    // Извлекаем данные
                    player_name = json_data.at("userName").as_string().c_str();
                    map_id = json_data.at("mapId").as_string().c_str();
                } catch (const std::out_of_range&) {
                    // Ошибка: ключ "userName" или "mapId" не найден
                    return HandleParseJSONError("Missing required field 'userName' or 'map_id'"s, http_version, keep_alive);
                } catch (const std::invalid_argument&) {
                    // значение по ключу "userName" или "mapId" не является string
                    return HandleParseJSONError("Fields 'userName' and 'map_id' must contain string value"s, http_version, keep_alive);
                } catch (const std::exception& e) {
                    // Любая другая стандартная ошибка (например, от json::parse, если всё же бросает исключение)
                    return HandleParseJSONError("Failed to parse Join game JSON: "s + e.what(), http_version, keep_alive);
                } catch (...) {
                    // Совершенно непредвиденная ошибка (не из std::exception)
                    return HandleParseJSONError("Unexpected error while processing JSON"s, http_version, keep_alive);
                }

                // Все JSON ошибки обработаны, продолжаем обрабатывать запрос

                if (!player_name.size()) {
                    // было передано пустое имя игрока
                    return HandleEmptyPlayerName(http_version, keep_alive);
                }

                std::shared_ptr<model::Map> map = application_.FindMap(model::Map::Id{ map_id });
                if (!map) {
                    //В качестве mapId указан несуществующий id карты
                    return HandleMapNotFound(http_version, keep_alive);
                }

                // На данный момент JSON спарсился успешно, имя игрока валидно и такая карта есть
                return HandleSuccessfullGameJoinRequest(player_name, map_id, http_version, keep_alive);

            } else if (match[1] == "players" || match[1] == "state") {
                // /api/v1/game/players/ или /api/v1/game/state/ или /api/v1/game/records/ - METHOD NOT ALLOWED
                return HandleMethodNotAllowed(
                    "Invalid method"s,
                    "GET, HEAD"s,
                    http_version,
                    keep_alive
                );
            } else if (match[1] == "tick"s) {
                if (!is_tick_needed_) {
                    return HandleBadRequest(
                        "badRequest"s,
                        "Tick requests are disabled when tick period is set"s,
                        http_version,
                        keep_alive
                    );
                }
                // /api/v1/game/tick/ - обрабатываем дальше
                if (request_method != http::verb::post) {
                    // /api/v1/game/tick/ - METHOD NOT ALLOWED
                    return HandleMethodNotAllowed(
                        "Only POST method is expected"s,
                        "POST"s,
                        http_version,
                        keep_alive
                    );
                }

                // Получили POST запрос на /api/v1/game/tick/

                json::value json_data;
                int64_t time_delta;

                try {
                    // Парсим JSON
                    json_data = json::parse(request_body);
                    if (json_data.is_null()) {
                        // Ошибка при парсинге, возможно передали не JSON
                        return HandleParseJSONError("Invalid Tick JSON: parsed data is null!"s, http_version, keep_alive);
                    }
                    // Извлекаем данные
                    time_delta = json_data.at("timeDelta").as_int64();
                } catch (const std::out_of_range&) {
                    // Ошибка: ключ "timeDelta" не найден
                    return HandleParseJSONError("Missing required field 'timeDelta'"s, http_version, keep_alive);
                } catch (const std::invalid_argument&) {
                    // значение по ключу "timeDelta" не является int64
                    return HandleParseJSONError("Field 'timeDelta' must contain int64 value"s, http_version, keep_alive);
                } catch (const std::exception& e) {
                    // Любая другая стандартная ошибка (например, от json::parse, если всё же бросает исключение)
                    return HandleParseJSONError("Failed to parse Tick JSON: "s + e.what(), http_version, keep_alive);
                } catch (...) {
                    // Совершенно непредвиденная ошибка (не из std::exception)
                    return HandleParseJSONError("Unexpected error while processing Tick JSON"s, http_version, keep_alive);
                }

                // К этому моменту все проверки пройдены
                // Двигаем собак во всех игровых сессиях
                return HandleSuccessfullTickRequest(time_delta, http_version, keep_alive);
            }
        } else if (std::regex_match(path, match, case_player_action)) {
            // /api/v1/game/player/action/ - обрабатываем дальше
            if (request_method != http::verb::post) {
                // /api/v1/game/player/action - METHOD NOT ALLOWED
                return HandleMethodNotAllowed(
                    "Only POST method is expected"s,
                    "POST"s,
                    http_version,
                    keep_alive
                );
            }
            // Получили POST запрос на /api/v1/game/player/action/
            // Обрабатываем случай невалидного токена
            if (
                !auth_header.size() ||
                !auth_header.starts_with(application::detail::TOKEN_STRATS_WITH) ||
                auth_header.size() != application::detail::TOKEN_SIZE
            ) {
                return HandleUnauthorized(
                    auth_header,
                    "invalidToken"s,
                    "Authorization header is required"s,
                    http_version,
                    keep_alive
                );
            }

            if (content_type != "application/json") {
                // заголовок Content-Type отсутствует, либо имеет значение, отличное от application/json
                return MakeStringResponse(
                    http::status::bad_request,
                    BuildBadRequestJSON("invalidArgument"s, "Invalid content type"s),
                    http_version,
                    keep_alive,
                    ContentType::APPLICATION_JSON
                );
            }

            // На этом этапе токен точно валидный
            application::Token token{ auth_header.substr(application::detail::TOKEN_POS_TO_DISCARD_BEARER) };

            // Находим игрока по токену
            std::shared_ptr<application::Player> player = application_.FindPlayerByToken(token);

            // Обрабатываем случай, когда игрок по токену не найден
            if (!player) {
                return HandleUnauthorized(
                    auth_header,
                    "unknownToken"s,
                    "Player token has not been found"s,
                    http_version,
                    keep_alive
                );
            }

            json::value json_data;
            std::string move_direction;

            try {
                // Парсим JSON
                json_data = json::parse(request_body);
                // Извлекаем данные
                move_direction = json_data.at("move").as_string().c_str();
                if (valid_directions.find(move_direction) == valid_directions.end()) {
                    throw std::exception();
                }
            } catch (...) {
                // Произошла ошибка при парсинге JSON или получении его свойств
                return HandleParseJSONError("Failed to parse action"s, http_version, keep_alive);
            }

            // На этом этапе токен валидный, такой игрок есть, JSON валидный
            // Двигаем собаку и возвращаем 200
            return HandleSuccessfullPlayerActionRequest(player, move_direction, http_version, keep_alive);
        }
        // bad request к API, который изменяет состояние игры
        return HandleBadRequest("badRequest"s, "Bad request"s, http_version, keep_alive);
    }
}