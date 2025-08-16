#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>
#include "model.h"
#include "application.h"
#include "geom.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

// Сериализация для Point
template <typename Archive>
void serialize(Archive& ar, Point& point, [[maybe_unused]] const unsigned version) {
    ar & point.x;
    ar & point.y;
}

// Сериализация для Speed
template <typename Archive>
void serialize(Archive& ar, Speed& speed, [[maybe_unused]] const unsigned version) {
    ar & speed.v_x;
    ar & speed.v_y;
}

// Сериализация для Position
template <typename Archive>
void serialize(Archive& ar, Position& position, [[maybe_unused]] const unsigned version) {
    ar & position.x;
    ar & position.y;
}

// Сериализация для Direction (enum class)
template <typename Archive>
void serialize(Archive& ar, Dog::Direction& direction, [[maybe_unused]] const unsigned version) {
    ar & reinterpret_cast<int&>(direction);
}

}  // namespace model

namespace serialization {

// LostObjectRepr - представление потерянного предмета
class LostObjectRepr {
public:
    LostObjectRepr() = default;

    explicit LostObjectRepr(const model::LostObject& obj)
        : id_(obj.GetId())
        , type_(obj.GetType())
        , position_(obj.GetPosition())
        , value_(obj.GetValue())
        , is_collected_(obj.IsCollected()) {}

    [[nodiscard]] model::LostObject Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & type_;
        ar & position_;
        ar & value_;
        ar & is_collected_;
    }

    [[nodiscard]] model::LostObject::Id GetId() const {
        return id_;
    }

private:
    model::LostObject::Id id_;
    size_t type_;
    model::Position position_;
    int64_t value_;
    bool is_collected_;
};

// DogRepr - представление собаки для сериализации
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , position_(dog.GetDogPosition())
        , prev_position_(dog.GetPrevPosition())
        , speed_(dog.GetDogSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_capacity_(dog.GetBag().GetCapacity()) {
        for (const auto& item : dog.GetBag().GetItems()) {
            bag_items_.emplace_back(LostObjectRepr{item});
        }
    }

    [[nodiscard]] std::shared_ptr<model::Dog> Restore() const;

    [[nodiscard]] const model::Dog::Id GetId() const {
        return id_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & name_;
        ar & position_;
        ar & prev_position_;
        ar & speed_;
        ar & direction_;
        ar & score_;
        ar & bag_capacity_;
        ar & bag_items_;
    }

private:
    model::Dog::Id id_;
    std::string name_;
    model::Position position_;
    model::Position prev_position_;
    model::Speed speed_;
    model::Dog::Direction direction_;
    int score_;
    int64_t bag_capacity_;
    std::vector<LostObjectRepr> bag_items_;
};

// PlayerRepr - представление игрока
class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const application::Player& player)
        : id_(player.GetId())
        , name_(player.GetName())
        , session_id_(player.GetSessionId())
        , dog_id_(player.GetDog()->GetId()) {}

    [[nodiscard]] std::shared_ptr<application::Player> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & name_;
        ar & session_id_;
        ar & dog_id_;
        ar & token_;
    }

    [[nodiscard]] const std::string& GetToken() const {
        return token_;
    }

    void SetToken(const std::string& token) {
        token_ = token;
    }

    [[nodiscard]] const model::GameSession::Id& GetSessionId() const {
        return session_id_;
    }

    [[nodiscard]] const model::Dog::Id& GetDogId() const {
        return dog_id_;
    }

    [[nodiscard]] const application::Player::Id& GetId() const {
        return id_;
    }

    [[nodiscard]] const std::string& GetName() const {
        return name_;
    }

private:
    application::Player::Id id_;
    std::string name_;
    model::GameSession::Id session_id_;
    model::Dog::Id dog_id_;
    std::string token_;
};

// SessionRepr - представление игровой сессии
class SessionRepr {
public:
    SessionRepr() = default;

    explicit SessionRepr(const model::GameSession& session)
        : id_(session.GetId())
        , map_id_(*session.GetMap()->GetId()) {
        for (const auto& dog : session.GetDogs()) {
            dogs_.emplace_back(DogRepr{*dog});
        }
        for (const auto& [id, obj] : session.GetLostObjects()) {
            lost_objects_.emplace_back(LostObjectRepr{obj});
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & map_id_;
        ar & dogs_;
        ar & lost_objects_;
    }

    [[nodiscard]] const model::GameSession::Id GetId() const {
        return id_;
    }

    [[nodiscard]] const model::Map::Id GetMapId() const {
        return map_id_;
    }

    [[nodiscard]] const std::vector<DogRepr>& GetDogs() const {
        return dogs_;
    }

    [[nodiscard]] const std::vector<LostObjectRepr>& GetLostObjects() const {
        return lost_objects_; 
    }

private:
    model::GameSession::Id id_;
    model::Map::Id map_id_;
    std::vector<DogRepr> dogs_;
    std::vector<LostObjectRepr> lost_objects_;
};

class IdCountersRepr {
public:
    IdCountersRepr() = default;

    explicit IdCountersRepr(size_t sessions, size_t dogs, size_t players)
        : sessions_ids_(sessions), dogs_ids_(dogs), players_ids_(players) {}

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_ids_;
        ar & dogs_ids_;
        ar & players_ids_;
    }

    [[nodiscard]] size_t GetSessionsIds() const {
        return sessions_ids_;
    }

    [[nodiscard]] size_t GetDogsIds() const {
        return dogs_ids_;
    }

    [[nodiscard]] size_t GetPlayersIds() const {
        return players_ids_;
    }

private:
    size_t sessions_ids_ = 0;
    size_t dogs_ids_ = 0;
    size_t players_ids_ = 0;
};

// GameStateRepr - полное состояние для сериализации
class GameStateRepr {
public:
    GameStateRepr() = default;

    explicit GameStateRepr(const application::Application& app) {
        for (const auto& [map_id, sessions] : app.GetMapIdToSession()) {
            for (const auto& session : sessions) {
                sessions_.emplace_back(*session);
            }
        }

        for (const auto& [token, player] : app.GetPlayerTokens().GetTokenToPlayer()) {
            players_.emplace_back(*player);
            players_.back().SetToken(*token);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_;
        ar & players_;
        ar & id_counters_;
    }

    [[nodiscard]] const std::vector<SessionRepr>& GetSessions() const {
        return sessions_;
    }

    void AddSession(const SessionRepr& session_repr) {
        sessions_.push_back(session_repr);
    }

    [[nodiscard]] const std::vector<PlayerRepr>& GetPlayers() const {
        return players_;
    }

    void AddPlayer(const PlayerRepr& player_repr) {
        players_.push_back(player_repr);
    }

    void SetIdCounters(size_t sessions, size_t dogs, size_t players) {
        id_counters_ = IdCountersRepr(sessions, dogs, players);
    }

    [[nodiscard]] const IdCountersRepr& GetIdCounters() const {
        return id_counters_;
    }

private:
    std::vector<SessionRepr> sessions_;
    std::vector<PlayerRepr> players_;
    IdCountersRepr id_counters_;
};

}  // namespace serialization
