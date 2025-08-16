#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

// --- MAP ------ MAP ------ MAP ------ MAP ------ MAP ------ MAP ------ MAP ---
void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::AddRoad(std::shared_ptr<Road> road) {
    roads_.emplace_back(road);

    if (road->IsHorizontal()) {
        for (int begin = std::min(road->GetStart().x, road->GetEnd().x);
            begin <= std::max(road->GetStart().x, road->GetEnd().x); ++begin) {
            point_to_road_segments_[Point{ begin, road->GetStart().y }].push_back(road);
        }
    }
    else {
        for (int begin = std::min(road->GetStart().y, road->GetEnd().y);
            begin <= std::max(road->GetStart().y, road->GetEnd().y); ++begin) {
            point_to_road_segments_[Point{ road->GetStart().x, begin }].push_back(road);
        }
    }
}

Position Map::GetRandomPositionOnRandomRoad() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, roads_.size() - 1);

    size_t random_road_index = dist(gen);

    std::shared_ptr<Road> random_road = roads_[random_road_index];

    Position rand_position_on_road{};
    if (random_road->IsHorizontal()) {
        int min_x = std::min(random_road->GetStart().x, random_road->GetEnd().x);
        int max_x = std::max(random_road->GetStart().x, random_road->GetEnd().x);
        std::uniform_int_distribution<int> dist(min_x, max_x);
        rand_position_on_road.x = dist(gen);
        rand_position_on_road.y = static_cast<double>(random_road->GetStart().y);
    }
    else {
        int min_y = std::min(random_road->GetStart().y, random_road->GetEnd().y);
        int max_y = std::max(random_road->GetStart().y, random_road->GetEnd().y);
        std::uniform_int_distribution<int> dist(min_y, max_y);
        rand_position_on_road.x = static_cast<double>(random_road->GetStart().x);
        rand_position_on_road.y = dist(gen);
    }
    
    return rand_position_on_road;
}

Position Map::GetStartPointOnFirstRoad() const {
    std::shared_ptr<Road> first_road = roads_.front();
    return {
        static_cast<double>(first_road->GetStart().x),
        static_cast<double>(first_road->GetStart().y)
    };
}
// --- MAP ------ MAP ------ MAP ------ MAP ------ MAP ------ MAP ------ MAP ---
//
//
// 
// --- DOG ------ DOG ------ DOG ------ DOG ------ DOG ------ DOG ------ DOG ---
std::string Dog::GetStringDirection() const noexcept {
    switch (direction_) {
    case(Direction::NORTH):
        return "U"s;
    case(Direction::SOUTH):
        return "D"s;
    case(Direction::EAST):
        return "R"s;
    case(Direction::WEST):
        return "L"s;
    }
    return "U"s;
}

void Dog::MoveDog(const std::string& str_dir, double speed) {
    if (str_dir == "U"s) {
        direction_ = Direction::NORTH;
        speed_ = { 0.0, -speed };
    }
    else if (str_dir == "D"s) {
        direction_ = Direction::SOUTH;
        speed_ = { 0.0, speed };
    }
    else if (str_dir == "R"s) {
        direction_ = Direction::EAST;
        speed_ = { speed, 0.0 };
    }
    else if (str_dir == "L"s) {
        direction_ = Direction::WEST;
        speed_ = { -speed, 0.0 };
    }
    else {
        speed_ = { 0.0, 0.0 };
    }
}

bool Dog::IsPositionValid(const Position& pos, const std::vector<std::shared_ptr<Road>>& dog_roads) const {
    return std::any_of(dog_roads.begin(), dog_roads.end(), [&pos](const auto& road) {
        const auto& segment = road->GetRoadSegment();
        return pos.x >= segment.corner.x - EPSILON &&
               pos.x <= segment.corner.x + segment.size.width + EPSILON &&
               pos.y >= segment.corner.y - EPSILON &&
               pos.y <= segment.corner.y + segment.size.height + EPSILON;
    });
}

void Dog::GetWallStopAndSetPosition(const Position& pos, const std::vector<std::shared_ptr<Road>>& dog_roads) {

    // Если движение невалидно, значит на пути стена 
    // (в конце надо будет обнулить скорость)
    // Нужно найти максимальное перемещение среди всех дорог,
    // проходящих через текущую точку
    Position clamped_position = position_;

    double max_dx;
    double max_dy;
    double tmp_x = position_.x;
    double tmp_y = position_.y;
    bool first_road = true;

    for (std::shared_ptr<Road> road : dog_roads) {
        const Road::RealRectangle& segment = road->GetRoadSegment();
        // Если двигались вдоль Х (любое направление)
        // Надо найти левую или правую границу
        if (std::abs(speed_.v_x) > EPSILON) {
            tmp_x = std::clamp(
                pos.x,
                segment.corner.x,
                segment.corner.x + segment.size.width
            );
            // Если двигались вдоль Y (любое направление)
            // Надо найти верхнюю или нижнюю границу
        }
        else if (std::abs(speed_.v_y) > EPSILON) {
            tmp_y = std::clamp(
                pos.y,
                segment.corner.y,
                segment.corner.y + segment.size.height
            );
        }
        // Если первая итерация (первая дорога в списке)
        // то инициализируем максимальное расстояние
        // Иначе обновляем максимальное (при необходимости)
        if (first_road) {
            max_dx = std::abs(tmp_x - position_.x);
            max_dy = std::abs(tmp_y - position_.y);
            clamped_position.x = tmp_x;
            clamped_position.y = tmp_y;
            first_road = false;
        }
        else {
            if (std::abs(tmp_x - position_.x) > max_dx) {
                max_dx = std::abs(tmp_x - position_.x);
                clamped_position.x = tmp_x;
            }
            if (std::abs(tmp_y - position_.y) > max_dy) {
                max_dy = std::abs(tmp_y - position_.y);
                clamped_position.y = tmp_y;
            }
        }
    }
    // Т.к. ранее была стена обнуляем скорость
    // Обновляем позицию собаки по максимальной
    speed_ = { 0.0, 0.0 };
    position_ = clamped_position;
}

void Dog::MoveDogByTick(int64_t time_delta, const Map::PointToRoadSegments& point_to_road_segments_) {
    UpdateTimeSinceJoin(time_delta);
    if (std::abs(speed_.v_x) < EPSILON && std::abs(speed_.v_y) < EPSILON) {
        UpdateTimeSinceLastMove(time_delta);
        return;
    }

    // Получаем текущие дороги
    Point current_point{
        static_cast<int>(std::round(position_.x)),
        static_cast<int>(std::round(position_.y))
    };

    const std::vector<std::shared_ptr<Road>>& dog_roads = point_to_road_segments_.at(current_point);

    // Рассчитываем новую позицию
    Position new_position = position_;
    new_position.x += speed_.v_x * (time_delta * MS_TO_S);
    new_position.y += speed_.v_y * (time_delta * MS_TO_S);

    // Проверяем новую позицию на валидность
    // Ели валидна, то есть не уперлись ни в одну границу,
    // то перемещаем собаку и выходим из функции
    if (IsPositionValid(new_position, dog_roads)) {
        position_ = new_position;
        return;
    }

    GetWallStopAndSetPosition(new_position, dog_roads);

    UpdateTimeSinceLastMove();
}

bool Dog::IsInactive(const std::chrono::milliseconds& inactivity_threshold) const {
    if (speed_.v_x != 0.0 || speed_.v_y != 0.0) {
        return false;
    }
    return time_since_last_move_ >= inactivity_threshold;
}
// --- DOG ------ DOG ------ DOG ------ DOG ------ DOG ------ DOG ------ DOG ---
//
//
//
// --- GAME SESSION ------ GAME SESSION ------ GAME SESSION ------ GAME SESSION ---
std::shared_ptr<Dog> GameSession::CreateDog(const std::string& dog_name) {
    assert(dogs_.size() != max_dogs_amount_);
    dogs_.emplace_back(std::make_shared<Dog>(
        dog_name,
        map_->GetDogPosition(),
        map_->GetBagCapacityOnMap()
    ));

    // Генерируем 1 новый предмет при каждом входе игрока
    GenerateLoot(1);
    
    return dogs_.back();
}

void GameSession::GenerateLoot(unsigned count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> type_dist(0, map_->GetLootTypesAmount() - 1);
    
    for (unsigned i = 0; i < count; ++i) {
        size_t type = type_dist(gen);
        AddLostObject(LostObject{
            LostObject::Id{lost_objects_ids_++},
            type,
            map_->GetRandomPositionOnRandomRoad(),
            loot_types_ptr_->GetLootTypeValue(*map_->GetId(), type)
        });
    }
}


// ItemGathererProviderImpl — это внутренний (implementation) класс, 
// который нужен только для реализации HandleCollisions() в GameSession
// Это адаптер, который преобразует данные игры в формат, 
// понятный collision_detector::FindGatherEvents()
class ItemGathererProviderImpl : public collision_detector::ItemGathererProvider {
public:
    ItemGathererProviderImpl(
        const std::vector<std::shared_ptr<Dog>>& dogs,
        const std::unordered_map<LostObject::Id, LostObject, GameSession::LostObjectIdHasher>& lost_objects,
        const std::vector<Office>& offices
    ) : dogs_(dogs), lost_objects_(lost_objects), offices_(offices) {}

    size_t ItemsCount() const override {
        return lost_objects_.size() + offices_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        if (idx < lost_objects_.size()) {
            auto it = lost_objects_.begin();
            std::advance(it, idx);
            return {geom::Point2D{it->second.GetPosition().x, it->second.GetPosition().y}, 0.0};
        } else {
            idx -= lost_objects_.size();
            const auto& office = offices_.at(idx);
            return {geom::Point2D{static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)}, 0.5};
        }
    }

    size_t GatherersCount() const override {
        return dogs_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        std::shared_ptr<Dog> dog = dogs_.at(idx);
        return {
            geom::Point2D{dog->GetPrevPosition().x, dog->GetPrevPosition().y},
            geom::Point2D{dog->GetDogPosition().x, dog->GetDogPosition().y},
            0.3  // Половина ширины собаки
        };
    }

private:
    const std::vector<std::shared_ptr<Dog>>& dogs_;
    const std::unordered_map<LostObject::Id, LostObject, GameSession::LostObjectIdHasher>& lost_objects_;
    const std::vector<Office>& offices_;
};

void GameSession::RemoveCollectedObjects() {
    std::unordered_map<LostObject::Id, LostObject, GameSession::LostObjectIdHasher>& objects = GetMutableLostObjects();
    
    // Используем erase-remove idiom для unordered_map
    for (auto it = objects.begin(); it != objects.end(); ) {
        if (it->second.IsCollected()) {
            it = objects.erase(it);  // Безопасное удаление с возвратом следующего итератора
        } else {
            ++it;
        }
    }
}

void GameSession::HandleCollisions() {
    ItemGathererProviderImpl provider(GetDogs(), GetLostObjects(), GetMap()->GetOffices());
    auto events = collision_detector::FindGatherEvents(provider);

    for (const auto& event : events) {
        std::shared_ptr<Dog> dog = GetDogs().at(event.gatherer_id);
        
        if (event.item_id < GetLostObjects().size()) {
            // Коллизия с предметом
            if (!dog->GetBag().IsFull()) {
                auto it = GetMutableLostObjects().begin();
                std::advance(it, event.item_id);
                if (it->second.IsCollected()) {
                    continue;
                }
                it->second.MarkAsCollected();
                dog->CollectItem(it->second);
            }
        } else {
            // Коллизия с базой - сдача предметов
            for (const auto& item : dog->GetBag().GetItems()) {
                dog->AddScore(item.GetValue());
            }
            dog->ReturnItems();
        }
    }

    // Удаление всех собранных предметов
    RemoveCollectedObjects();
}

std::vector<std::shared_ptr<Dog>> GameSession::RemoveInactiveDogs(const std::chrono::milliseconds& inactivity_threshold) {
    std::vector<std::shared_ptr<Dog>> inactive_dogs;
    
    // Разделяем собак на активных и неактивных
    auto partition_it = std::partition(
        dogs_.begin(),
        dogs_.end(),
        [&inactivity_threshold](std::shared_ptr<Dog> dog) {
            return !dog->IsInactive(inactivity_threshold);
        }
    );
    
    // Переносим неактивных собак в отдельный вектор
    std::move(partition_it, dogs_.end(), std::back_inserter(inactive_dogs));

    // Удаляем неактивных собак из сесиии
    dogs_.erase(partition_it, dogs_.end());
    
    return inactive_dogs;
}
// --- GAME SESSION ------ GAME SESSION ------ GAME SESSION ------ GAME SESSION ---
//
//
//
// --- GAME ------ GAME ------ GAME ------ GAME ------ GAME ------ GAME ------ GAME ---
void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::make_shared<Map>(std::move(map)));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second);
    }
    return nullptr;
}

void Game::AddSession(const Map::Id& map_id, std::shared_ptr<GameSession> session) {

    if (auto [it, inserted] = map_id_to_sessions_[map_id].emplace(session); !inserted) {
        throw std::invalid_argument("Session with id "s + std::to_string(*session->GetId()) + " already exists"s);
    } else {
        try {
            map_id_to_sessions_.at(map_id).emplace(std::move(session));
        }
        catch (...) {
            map_id_to_sessions_.at(map_id).erase(it);
            throw;
        }
    }
}

std::shared_ptr<GameSession> Game::FindSession(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_sessions_.find(id); it != map_id_to_sessions_.end()) {
        std::unordered_set<std::shared_ptr<GameSession>> cur_map_sessions = it->second;
        for (std::shared_ptr<GameSession> session : cur_map_sessions) {
            if (!session->IsSessionFull()) {
                return session;
            }
        }
        return nullptr;
    }
    return nullptr;
}
// --- GAME ------ GAME ------ GAME ------ GAME ------ GAME ------ GAME ------ GAME ---
}  // namespace model
