#pragma once

#include <random>
#include <tuple>
#include <chrono>

#include "tagged.h"
#include "model.h"

#include "loot_generator.h"

#include "database/postgres/postgres.h"
#include "database/app/use_cases_impl.h"

namespace application {

namespace net = boost::asio;

namespace detail {
    using namespace std::literals;

    constexpr static int TOKEN_SIZE = 39;
    constexpr static int TOKEN_POS_TO_DISCARD_BEARER = 7;
    constexpr static std::string_view TOKEN_STRATS_WITH = "Bearer "sv;

    struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class Player {
public:
	using Id = util::Tagged<size_t, Player>;

    Player(const std::string name) noexcept
    : id_(Id{ Player::players_ids_++ }), name_(name)
    {
        
    };

    Player(Id id, const std::string name) noexcept
    : id_(id), name_(name)
    {

    };

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    void SetGameSession(std::shared_ptr<model::GameSession> session) {
        session_ = session;
    };

    void SetDog(std::shared_ptr<model::Dog> dog) {
        dog_ = dog;
    };

    std::shared_ptr<model::Dog> GetDog() const {
        return dog_;
    }

    const model::GameSession::Id GetSessionId() const {
        return session_->GetId();
    }

    std::shared_ptr<model::GameSession> GetSession() {
        return session_;
    }

    static size_t GetLastPlayerId() {
        return players_ids_;
    }

    static void SetLastPlayerId(size_t id) {
        players_ids_ = id;
    }

private:
    inline static size_t players_ids_ = 0;
    Id id_;
    std::string name_;

    std::shared_ptr<model::GameSession> session_;
    std::shared_ptr<model::Dog> dog_;
};

class PlayerTokens {
public:
    using TokenHasher = util::TaggedHasher<Token>;

    PlayerTokens() = default;

    Token AddPlayer(std::shared_ptr<Player> player);

    std::shared_ptr<Player> FindPlayerByToken(Token token);

    void SetPlayerToken(Token token, std::shared_ptr<Player> player) {
        token_to_player_[token] = player;
    }

    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetTokenToPlayer() const {
        return token_to_player_;
    }

    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetMutableTokenToPlayer() {
        return token_to_player_;
    }

private:
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    std::random_device random_device_;
    std::mt19937_64 generator1_{ [this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }() };
    std::mt19937_64 generator2_{ [this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }() };
};

// Интерфейс для паттерна "Наблюдатель"
class ApplicationListener {
public:
    virtual ~ApplicationListener() = default;
    virtual void OnTick(std::chrono::milliseconds time_delta) = 0;
    virtual void OnShutdown() = 0;
};

struct AppConfig {
    std::string db_url;
};

class Application {
public:

    Application(model::Game game, const AppConfig& config) 
    : game_{std::move(game)},
    loot_generator_{
        std::chrono::milliseconds(game_.GetLootGeneratorConfig().period),
        game_.GetLootGeneratorConfig().probability
    },
    /*db_("postgres://postgres:Mys3Cr3t@172.17.0.2:5432/test111") - ДЛЯ ТЕСТИРОВАНИЯ ЛОКАЛЬНО, НЕ В КОНТЕЙНЕРЕ */db_(config.db_url)
    {

    }

    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

    const model::Game::Maps& GetMaps() const noexcept {
        return game_.GetMaps();
    }

    const model::Game::MapIdToSessions& GetMapIdToSession() const noexcept {
        return game_.GetMapIdToSession();
    }

    const std::vector<std::shared_ptr<Player>>& GetPlayers() const {
        return players_;
    }

    const PlayerTokens& GetPlayerTokens() const {
        return player_tokens_;
    }

    void AddPlayer(std::shared_ptr<Player> player);

    void SetPlayerToken(Token token, std::shared_ptr<Player> player) {
        player_tokens_.SetPlayerToken(token, player);
    }

    std::tuple<Token, Player::Id> JoinGame(const std::string& player_name, const model::Map::Id& map_id);

    std::shared_ptr<application::Player> FindPlayerByToken(Token token) {
        return player_tokens_.FindPlayerByToken(token);
    }

    std::vector<std::shared_ptr<Player>> GetCurrentPlayerGameSessionPlayers(std::shared_ptr<Player> player);

    void Tick(std::chrono::milliseconds time_delta);

    const extra_data::LootTypes& GetLootTypes() const {
        return game_.GetLootTypes();
    }

    void SetListener(std::shared_ptr<ApplicationListener> listener) {
        listener_ = listener;
    }

    void AddSession(const model::Map::Id& map_id, std::shared_ptr<model::GameSession> session) {
        game_.AddSession(map_id, session);
    }

    std::vector<domain::Player> GetRecordsTable(size_t offset, size_t limit) const {
        return use_cases_.GetRecordsTable(offset, limit);
    }

private:
    model::Game game_;
    std::vector<std::shared_ptr<Player>> players_;
    PlayerTokens player_tokens_;

    using GameSessionIdHasher = util::TaggedHasher<model::GameSession::Id>;
    using GameSessionIdToPlayers = std::unordered_map<
        model::GameSession::Id,
        std::vector<std::shared_ptr<Player>>,
        GameSessionIdHasher
    >;

    GameSessionIdToPlayers session_id_to_players_;

    std::shared_ptr<Player> CreatePlayer(const std::string& player_name);

    void TiePlayerWithSession(
        std::shared_ptr<Player> player,
        std::shared_ptr<model::GameSession> session
    );

    loot_gen::LootGenerator loot_generator_;

    std::shared_ptr<ApplicationListener> listener_;

    postgres::Database db_;
    app::UseCasesImpl use_cases_{db_.GetPlayerRecords()};

    void RetirePlayer(const std::shared_ptr<model::Dog>& dog);

    void RemovePlayerToken(const std::shared_ptr<Player>& player);

    void RemovePlayerFromSession(const std::shared_ptr<Player>& player);
};

}