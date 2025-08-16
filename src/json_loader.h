#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json_loader {

constexpr static double DOG_SPEED_BY_DEFAULT = 1.0;
constexpr static int64_t BAG_CAPACITY_BY_DEFAULT = 3;
constexpr static int SECONDS_TO_MILISECONDS = 1000;
constexpr static double DEFAULT_RETIREMENT_TIME_SEC = 60.0;

namespace json = boost::json;
using namespace std::literals;

void AddMapsToTheGame(
    model::Game& game,
    const json::array& map_arr,
    double default_dog_speed,
    bool randomize_spawn_points,
    int64_t default_bag_capacity
);

void AddRoadsToTheMap(model::Map& map, const json::array& roads_arr);
void AddBuildingsToTheMap(model::Map& map, const json::array& buildings_arr);
void AddOfficesToTheMap(model::Map& map, const json::array& offices_arr);

model::Game LoadGame(const std::filesystem::path& json_path, bool randomize_spawn_points);

}  // namespace json_loader
