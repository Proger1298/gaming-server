#include "extra_data.h"

namespace extra_data {

void LootTypes::AddLootTypes(std::string map_id, const json::array& loot_types_arr) {
    for (const json::value& loot_type_value : loot_types_arr) {
        map_id_to_loot_types_[map_id].push_back(CreateLootType(loot_type_value));
    }
}

const std::vector<LootTypes::LootType>& LootTypes::GetCurrentMapLootTypes(std::string map_id) const {
    return map_id_to_loot_types_.at(map_id);
}

LootTypes::LootType LootTypes::CreateLootType(const json::value& loot_type_value) {
    LootType loot_type;
    try {
        loot_type = {
            loot_type_value.at("name"s).as_string().c_str(),
            loot_type_value.at("file"s).as_string().c_str(),
            loot_type_value.at("type"s).as_string().c_str(),
            loot_type_value.as_object().contains("rotation"s) ? std::optional<int64_t>(loot_type_value.at("rotation"s).as_int64()) : std::nullopt,
            loot_type_value.as_object().contains("color"s) ? std::optional<std::string>(loot_type_value.at("color"s).as_string().c_str()) : std::nullopt,
            loot_type_value.at("scale"s).as_double(),
            loot_type_value.at("value"s).as_int64(),
        };
    } catch (const std::out_of_range&) {
        throw std::runtime_error("'lootTypes' is incorrect! Missing some required fields: 'name', 'file', 'type', 'scale' or 'value"s);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("'lootTypes' is incorrect! Some fields have incorrect type"s);
    } catch (...) {
        // Непредвиденная ошибка
        throw std::runtime_error("Unexpected error while processing 'lootTypes' for map!"s);
    }
    return loot_type;
}

int64_t LootTypes::GetLootTypeValue(const std::string& map_id, size_t type) const {
    return GetCurrentMapLootTypes(map_id).at(type).value;
}

} // namespace extra_data