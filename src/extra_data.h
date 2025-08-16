#pragma once

#include <boost/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace extra_data {

namespace json = boost::json;
using namespace std::literals;

class LootTypes {
public:

    struct LootType {
        std::string name;
        std::string file;
        std::string type;
        std::optional<std::int64_t> rotation;
        std::optional<std::string> color;
        double scale{0.0};
        int64_t value;
    };

    void AddLootTypes(std::string map_id, const json::array& loot_types_arr);

    const std::vector<LootType>& GetCurrentMapLootTypes(std::string map_id) const;

    int64_t GetLootTypeValue(const std::string& map_id, size_t type) const;

private:

    std::unordered_map<std::string, std::vector<LootType>> map_id_to_loot_types_;

    LootType CreateLootType(const json::value& loot_type_value);

};

} // namespace extra_data