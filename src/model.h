#pragma once

#include <boost/asio/io_context.hpp>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <unordered_set>
#include <random>
#include <chrono>
#include <iostream> // KILL ME

#include "tagged.h"
#include "extra_data.h"
#include "collision_detector.h"

namespace model {

using namespace std::literals;

namespace net = boost::asio;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Position {
    double x = 0.0;
    double y = 0.0;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

    struct RealSize {
        double width;
        double height;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    constexpr static double ROAD_WIDTH = 0.8;
    constexpr static double HALF_ROAD_WIDTH = 0.4;

    struct RealRectangle {
        Position corner;
        RealSize size;
    };

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{ start },
        end_{ end_x, start.y },
        road_segment_{
            {std::min(start.x, end_x) - HALF_ROAD_WIDTH, start.y - HALF_ROAD_WIDTH},
            {std::abs(end_x - start.x) + ROAD_WIDTH, ROAD_WIDTH}
        }
    {

    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{ start },
        end_{ start.x, end_y },
        road_segment_{
            {start.x - HALF_ROAD_WIDTH, std::min(start.y, end_y) - HALF_ROAD_WIDTH},
            {ROAD_WIDTH, std::abs(end_y - start.y) + ROAD_WIDTH}
        }
    {

    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    const RealRectangle& GetRoadSegment() const noexcept {
        return road_segment_;
    }

private:
    Point start_;
    Point end_;

    RealRectangle road_segment_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class LostObject {
public:
    using Id = util::Tagged<size_t, LostObject>;

    LostObject() = default;
    
    LostObject(Id id, size_t type, Position position, int64_t value) 
        : id_{std::move(id)}, type_{type}, position_{position}, value_(value) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    size_t GetType() const noexcept {
        return type_;
    }

    const Position& GetPosition() const noexcept {
        return position_;
    }

    int GetValue() const { 
        return value_; 
    }

    bool IsCollected() const {
        return is_collected_;
    }

    void MarkAsCollected() {
        is_collected_ = true;
    }

private:
    Id id_;
    size_t type_;
    Position position_;
    int64_t value_;  // Ценность предмета

    bool is_collected_ = false;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<std::shared_ptr<Road>>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    struct PointHasher {
        size_t operator()(const Point& point) const {
            size_t hash_1 = std::hash<int>{}(point.x);
            size_t hash_2 = std::hash<int>{}(point.y);
            return hash_1 ^ (hash_2 << 1);
        }
    };

    using PointToRoadSegments = std::unordered_map<Point, std::vector<std::shared_ptr<Road>>, PointHasher>;

    Map(
        Id id,
        std::string name,
        double dog_speed,
        bool randomize_spawn_points,
        size_t loot_types_amount,
        int64_t bag_capacity
    ) noexcept
    : id_(std::move(id)),
    name_(std::move(name)),
    dog_speed_(dog_speed),
    randomize_spawn_points_(randomize_spawn_points),
    loot_types_amount_(loot_types_amount),
    bag_capacity_(bag_capacity)
    {

    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(std::shared_ptr<Road> road);

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetIfRandomDogPositionOnMap(bool randomize_spawn_points) {
        randomize_spawn_points_ = randomize_spawn_points;
    }

    Position GetRandomPositionOnRandomRoad() const;

    Position GetStartPointOnFirstRoad() const;

    Position GetDogPosition() const {
        return randomize_spawn_points_ ? GetRandomPositionOnRandomRoad() : GetStartPointOnFirstRoad();
    }

    double GetDogSpeedOnMap() {
        return dog_speed_;
    }

    const PointToRoadSegments& GetPointToRoadSegments() const noexcept {
        return point_to_road_segments_;
    }

    size_t GetLootTypesAmount() const noexcept {
        return loot_types_amount_;
    }

    int64_t GetBagCapacityOnMap() {
        return bag_capacity_;
    }

private:
    double dog_speed_;
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    PointToRoadSegments point_to_road_segments_;

    bool randomize_spawn_points_;

    size_t loot_types_amount_ = 0;

    int64_t bag_capacity_;
};

struct Speed {
    double v_x = 0.0;
    double v_y = 0.0;
};

class Bag {
public:
    using Items = std::vector<LostObject>;

    explicit Bag(int64_t capacity) : capacity_(capacity) {}

    bool IsFull() const {
        return items_.size() >= capacity_;
    }

    void AddItem(const LostObject& item) {
        if (!IsFull()) {
            items_.push_back(item);
        }
    }

    void Clear() {
        items_.clear();
    }

    const Items& GetItems() const {
        return items_;
    }

    size_t Size() const {
        return items_.size();
    }

    int64_t GetCapacity() const {
        return capacity_;
    }

private:
    int64_t capacity_;
    Items items_;
};

class Dog {
public:
    enum class Direction {
        NORTH,
        SOUTH,
        WEST,
        EAST
    };

    using Id = util::Tagged<size_t, Dog>;
    using DogIdHasher = util::TaggedHasher<Dog::Id>;

    static constexpr double EPSILON = 0.001;
    static constexpr double MS_TO_S = 0.001;

    Dog(std::string name, Position position, int64_t bag_capacity) noexcept
    : id_(Id{Dog::dogs_ids_++}),
    name_(name),
    position_(position),
    bag_(bag_capacity)
    {

    };

    Dog(Id id, std::string name, Position position, int64_t bag_capacity) noexcept
    :
    id_(id),
    name_(name),
    position_(position),
    bag_(bag_capacity)
    {

    };

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Position& GetDogPosition() const noexcept {
        return position_;
    }

    const Speed& GetDogSpeed() const noexcept {
        return speed_;
    }

    void SetDogSpeed(const Speed& speed) {
        speed_ = speed;
    }

    const Direction GetDirection() const noexcept {
        return direction_;
    }

    void SetDirection(const Direction& direction) {
        direction_ = direction;
    }

    std::string GetStringDirection() const noexcept;

    void MoveDog(const std::string& str_dir, double speed);

    bool IsPositionValid(const Position& pos, const std::vector<std::shared_ptr<Road>>& dog_roads) const;

    void GetWallStopAndSetPosition(const Position& pos, const std::vector<std::shared_ptr<Road>>& dog_roads);

    void MoveDogByTick(int64_t time_delta, const Map::PointToRoadSegments& point_to_road_segments_);

    void CollectItem(const LostObject& item) {
        bag_.AddItem(item);
    }

    void ReturnItems() {
        bag_.Clear();
    }

    const Bag& GetBag() const {
        return bag_;
    }

    void SetPrevPosition(Position pos) {
        prev_position_ = pos;
    }

    const Position& GetPrevPosition() const {
        return prev_position_;
    }

    int GetScore() const { 
        return score_; 
    }

    void SetScore(int score) {
        score_ = score;
    }

    void AddScore(int points) { 
        score_ += points; 
    }

    static size_t GetLastDogId() {
        return dogs_ids_;
    }

    static void SetLastDogId(size_t id) {
        dogs_ids_ = id;
    }

    void UpdateTimeSinceLastMove(int64_t time_delta) {
        time_since_last_move_ += std::chrono::milliseconds(time_delta);
    }

    void UpdateTimeSinceLastMove() {
        time_since_last_move_ = std::chrono::milliseconds{0};
    }

    void UpdateTimeSinceJoin(int64_t time_delta) {
        time_since_join_ += std::chrono::milliseconds(time_delta);
    }
    
    bool IsInactive(const std::chrono::milliseconds& inactivity_threshold) const;
    
    std::chrono::milliseconds GetPlayTime() const {
        return time_since_join_;
    }

private:
    inline static size_t dogs_ids_ = 0;
    Id id_;
    std::string name_;

    Position position_;
    Position prev_position_;
    Speed speed_;
    Direction direction_ = Direction::NORTH;

    Bag bag_;
    int score_ = 0;

    std::chrono::milliseconds time_since_join_{0};
    std::chrono::milliseconds time_since_last_move_{0};
};

class GameSession {
public:
    using Id = util::Tagged<size_t, GameSession>;
    using GameSesionIdHasher = util::TaggedHasher<GameSession::Id>;
    using LostObjectIdHasher = util::TaggedHasher<LostObject::Id>;

    GameSession(std::shared_ptr<Map> map, std::shared_ptr<extra_data::LootTypes> loot_types_ptr)
    : id_(Id{ GameSession::sessions_ids_++ }), map_(map), loot_types_ptr_(loot_types_ptr)
    {

    };

    GameSession(Id id, std::shared_ptr<Map> map, std::shared_ptr<extra_data::LootTypes> loot_types_ptr)
    : id_(id), map_(map), loot_types_ptr_(loot_types_ptr)
    {

    };

    const Id& GetId() const noexcept {
        return id_;
    }
    
    const std::shared_ptr<Map> GetMap() const {
        return map_;
    }

    std::shared_ptr<Dog> CreateDog(const std::string& dog_name);

    void AddDog(std::shared_ptr<Dog> dog) {
        dogs_.push_back(dog);
    }

    bool IsSessionFull() const {
        return dogs_.size() == max_dogs_amount_;
    }

    const std::vector<std::shared_ptr<Dog>>& GetDogs() const {
        return dogs_;
    }

    void AddLostObject(const LostObject& object) {
        lost_objects_.emplace(object.GetId(), object);
    }

    void GenerateLoot(unsigned count);

    const std::unordered_map<LostObject::Id, LostObject, LostObjectIdHasher>& GetLostObjects() const {
        return lost_objects_;
    }

    void HandleCollisions();

    static size_t GetLastSessionId() {
        return sessions_ids_;
    }

    static void SetLastSessionId(size_t id) {
        sessions_ids_ = id;
    }

    std::vector<std::shared_ptr<Dog>> RemoveInactiveDogs(const std::chrono::milliseconds& inactivity_threshold);

private:
    inline static size_t sessions_ids_ = 0;
    Id id_;

    std::shared_ptr<Map> map_;

    const size_t max_dogs_amount_ = 5;
    std::vector<std::shared_ptr<Dog>> dogs_;

    inline static size_t lost_objects_ids_ = 0;
    std::unordered_map<LostObject::Id, LostObject, LostObjectIdHasher> lost_objects_;

    std::shared_ptr<extra_data::LootTypes> loot_types_ptr_;

    void RemoveCollectedObjects();

    // Для модификации
    std::unordered_map<LostObject::Id, LostObject, LostObjectIdHasher>& GetMutableLostObjects() { 
        return lost_objects_; 
    }
};

struct LootGeneratorConfig {
    int period;
    double probability;
};

class Game {
public:
    using Maps = std::vector<std::shared_ptr<Map>>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToSessions = std::unordered_map<Map::Id, std::unordered_set<std::shared_ptr<GameSession>>, MapIdHasher>;

    Game(LootGeneratorConfig loot_generator_config, std::chrono::milliseconds max_inactivity_time) 
    : loot_generator_config_{loot_generator_config}, max_inactivity_time_{max_inactivity_time}
    {

    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    void AddSession(const Map::Id& map_id, std::shared_ptr<GameSession> session);

    std::shared_ptr<GameSession> FindSession(const Map::Id& id) const noexcept;

    const MapIdToSessions& GetMapIdToSession() const noexcept {
        return map_id_to_sessions_;
    }

    void SetLootTypes(const extra_data::LootTypes& loot_types) {
        loot_types_ = std::move(loot_types);
    }

    const extra_data::LootTypes& GetLootTypes() const {
        return loot_types_;
    }

    const LootGeneratorConfig& GetLootGeneratorConfig() const {
        return loot_generator_config_;
    }

    const std::chrono::milliseconds& GetMaxInactivityTime() const {
        return max_inactivity_time_;
    }

private:
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;

    MapIdToSessions map_id_to_sessions_;

    LootGeneratorConfig loot_generator_config_;
    extra_data::LootTypes loot_types_;

    std::chrono::milliseconds max_inactivity_time_;
};

}  // namespace model
