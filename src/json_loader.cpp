#include "json_loader.h"
#include "extra_data.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace json_loader {

static const std::string X0 = "x0"s;
static const std::string X1 = "x1"s;
static const std::string Y0 = "y0"s;
static const std::string Y1 = "y1"s;

static const std::string X = "x"s;
static const std::string Y = "y"s;
static const std::string W = "w"s;
static const std::string H = "h"s;

static const std::string OFFSET_X = "offsetX"s;
static const std::string OFFSET_Y = "offsetY"s;

void AddRoadsToTheMap(model::Map& map, const json::array& roads_arr) {
    for (const json::value& road : roads_arr) {
        // Если горизонтальная дорога
        if (road.as_object().contains(X1)) {
            map.AddRoad(std::make_shared<model::Road>(
                model::Road::HORIZONTAL,
                model::Point{
                    static_cast<int>(road.at(X0).as_int64()),
                    static_cast<int>(road.at(Y0).as_int64())
                },
                static_cast<int>(road.at(X1).as_int64())
            ));
        } else {
            map.AddRoad(std::make_shared<model::Road>(
                model::Road::VERTICAL,
                model::Point{
                    static_cast<int>(road.at(X0).as_int64()),
                    static_cast<int>(road.at(Y0).as_int64())
                },
                static_cast<int>(road.at(Y1).as_int64())
            ));
        }
    }
}

void AddBuildingsToTheMap(model::Map& map, const json::array& buildings_arr) {
    for (const json::value& building : buildings_arr) {
        map.AddBuilding(model::Building {
            {
                {
                    static_cast<int>(building.at(X).as_int64()),
                    static_cast<int>(building.at(Y).as_int64())
                },
                {
                    static_cast<int>(building.at(W).as_int64()),
                    static_cast<int>(building.at(H).as_int64())
                }
            }
        });
    }
}

void AddOfficesToTheMap(model::Map& map, const json::array& offices_arr) {
    for (const json::value& office : offices_arr) {
        map.AddOffice({
            model::Office::Id{
                office.at("id").as_string().c_str()
            },
            {
                static_cast<int>(office.at(X).as_int64()),
                static_cast<int>(office.at(Y).as_int64())
            },
            {
                static_cast<int>(office.at(OFFSET_X).as_int64()),
                static_cast<int>(office.at(OFFSET_Y).as_int64())
            }
        });
    }
}

void AddMapsToTheGame(
    model::Game& game,
    const json::array& map_arr,
    double default_dog_speed,
    bool randomize_spawn_points,
    int64_t default_bag_capacity
) {
    extra_data::LootTypes loot_types;
    for (const json::value& map : map_arr) {
        if (!map.as_object().contains("lootTypes"s)) {
            throw std::runtime_error("Invalid 'maps'! 'maps' does not contain 'loot_types'!"s);
        }
        json::array loot_types_arr = map.at("lootTypes").as_array();
        std::string str_map_id = map.at("id").as_string().c_str();
        loot_types.AddLootTypes(str_map_id, loot_types_arr);
        model::Map cur_map{
            model::Map::Id{str_map_id},
            map.at("name").as_string().c_str(),
            map.as_object().contains("dogSpeed"s) ? map.at("dogSpeed"s).as_double() : default_dog_speed,
            randomize_spawn_points,
            loot_types_arr.size(),
            map.as_object().contains("bagCapacity"s) ? map.at("bagCapacity"s).as_int64() : default_bag_capacity,
        };
        AddRoadsToTheMap(cur_map, map.at("roads").as_array());
        AddBuildingsToTheMap(cur_map, map.at("buildings").as_array());
        AddOfficesToTheMap(cur_map, map.at("offices").as_array());
        game.AddMap(cur_map);
    }
    game.SetLootTypes(std::move(loot_types));
}

model::Game LoadGame(const std::filesystem::path& json_path, bool randomize_spawn_points) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    // Открываем файл для чтения
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Can not open file: "s + json_path.string());
    }

    // Читаем содержимое файла в строку
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_string = buffer.str();

    json::object json_obj = json::parse(json_string).as_object();

    json::array maps_arr = json_obj.at("maps"s).as_array();

    double default_dog_speed = DOG_SPEED_BY_DEFAULT;
    if (json_obj.contains("defaultDogSpeed"s)) {
        default_dog_speed = json_obj.at("defaultDogSpeed"s).as_double();
    }

    int64_t default_bag_capacity = BAG_CAPACITY_BY_DEFAULT;
    if (json_obj.contains("defaultBagCapacity"s)) {
        default_dog_speed = json_obj.at("defaultBagCapacity"s).as_int64();
    }

    if (!json_obj.contains("lootGeneratorConfig"s)) {
        throw std::runtime_error("Invalid config file! 'lootGeneratorConfig' is missing!"s);
    }

    json::object loot_gen_json = json_obj.at("lootGeneratorConfig"s).as_object();

    model::LootGeneratorConfig loot_generator_config;
    try {
        loot_generator_config = {
            static_cast<int>(loot_gen_json.at("period"s).as_double()* SECONDS_TO_MILISECONDS),
            loot_gen_json.at("probability"s).as_double(),
        };
    } catch (const std::out_of_range&) {
        throw std::runtime_error("'lootGeneratorConfig' is incorrect! Missing required field 'period' or 'probability'"s);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("'lootGeneratorConfig' is incorrect! Fields 'period' and 'probability' must contain double value"s);
    } catch (const std::exception& e) {
        // Любая другая стандартная ошибка
        throw std::runtime_error("Failed to parse 'lootGeneratorConfig':" + std::string(e.what()));
    } catch (...) {
        // Совершенно непредвиденная ошибка (не из std::exception)
        throw std::runtime_error("Unexpected error while processing 'lootGeneratorConfig'!"s);
    }

    double retirement_time_sec = DEFAULT_RETIREMENT_TIME_SEC;
    if (json_obj.contains("dogRetirementTime"s)) {
        retirement_time_sec = json_obj.at("dogRetirementTime"s).as_double();
    }

    model::Game game {
        loot_generator_config,
        std::chrono::milliseconds{
            static_cast<int64_t>(retirement_time_sec * SECONDS_TO_MILISECONDS)
        }
    };

    AddMapsToTheGame(
        game, maps_arr,
        default_dog_speed,
        randomize_spawn_points,
        default_bag_capacity
    );

    return game;
}

}  // namespace json_loader
