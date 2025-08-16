#include <sstream>
#include <iomanip>

#include "application.h"
#include "logger.h"

namespace application {

using namespace logger;
using namespace std::literals;

Token PlayerTokens::AddPlayer(std::shared_ptr<Player> player) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << 
		std::setw(16) << generator1_() << 
		std::setw(16) << generator2_();
	Token token{ ss.str() };
	token_to_player_[token] = player;
	return token;
}

std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(Token token) {
	if (token_to_player_.contains(token)) {
		return token_to_player_.at(token);
	}
	return nullptr;
}

std::shared_ptr<Player> Application::CreatePlayer(const std::string& user_name) {
    auto player = std::make_shared<Player>(user_name);
    players_.push_back(player);
    return player;
};

void Application::TiePlayerWithSession(std::shared_ptr<Player> player, std::shared_ptr<model::GameSession> session) {
	session_id_to_players_[session->GetId()].push_back(player);
	player->SetGameSession(session);
	player->SetDog(session->CreateDog(player->GetName()));
};

void Application::AddPlayer(std::shared_ptr<Player> player) {
    players_.push_back(player);
    session_id_to_players_[player->GetSessionId()].push_back(player);
}

std::tuple<Token, Player::Id> Application::JoinGame(const std::string& user_name, const model::Map::Id& map_id) {
	std::shared_ptr<Player> player = CreatePlayer(user_name);
    Token token = player_tokens_.AddPlayer(player);
    std::shared_ptr<model::GameSession> game_session = game_.FindSession(map_id);
    if(!game_session){
        game_session = std::make_shared<model::GameSession>(
            game_.FindMap(map_id),
            std::make_shared<extra_data::LootTypes>(game_.GetLootTypes())
        );
        game_.AddSession(map_id, game_session);
    }
    TiePlayerWithSession(player, game_session);
    return std::tie(token, player->GetId());
}

std::vector<std::shared_ptr<Player>> Application::GetCurrentPlayerGameSessionPlayers(std::shared_ptr<Player> player) {
	model::GameSession::Id session_id = player->GetSessionId();
	return session_id_to_players_[session_id];
}

void Application::RetirePlayer(const std::shared_ptr<model::Dog>& dog) {
    // Находим игрока по собаке
    auto player_it = std::find_if(players_.begin(), players_.end(),
        [&dog](std::shared_ptr<application::Player> player) {
            return player->GetDog() == dog;
        });
        
    if (player_it == players_.end()) {
        return;
    }
    
    std::shared_ptr<application::Player> player = *player_it;
    
    // Записываем рекорд в БД
    try {
        auto play_time_seconds = static_cast<double>(dog->GetPlayTime().count()) / 1000.0;
        domain::Player retired_player(
            player->GetName(),
            dog->GetScore(),
            play_time_seconds
        );
        
        use_cases_.RetirePlayer(retired_player);
    } catch (const std::exception& e) {
        // Логируем ошибку, но не прерываем выполнение
        LOG_WITH_DATA(error, json::object{}, "Failed to retire player: "s + e.what());
    }
    
    // Удаляем токен игрока
    RemovePlayerToken(player);
    
    // Удаляем игрока из сессии
    RemovePlayerFromSession(player);
    
    // Удаляем игрока из основного списка
    players_.erase(player_it);
}

void Application::RemovePlayerToken(const std::shared_ptr<Player>& player) {
    auto& tokens = player_tokens_.GetMutableTokenToPlayer();
    for (auto it = tokens.begin(); it != tokens.end(); ) {
        if (it->second == player) {
            it = tokens.erase(it);
        } else {
            ++it;
        }
    }
}

void Application::RemovePlayerFromSession(const std::shared_ptr<Player>& player) {
    auto session_id = player->GetSessionId();
    if (session_id_to_players_.count(session_id)) {
        std::vector<std::shared_ptr<Player>>& session_players = session_id_to_players_.at(session_id);
        session_players.erase(
            std::remove(session_players.begin(), session_players.end(), player),
            session_players.end()
        );
    }
}

void Application::Tick(std::chrono::milliseconds time_delta) {
    const std::chrono::milliseconds retirement_time = game_.GetMaxInactivityTime();
	const model::Game::MapIdToSessions& map_to_sessions = GetMapIdToSession();
	for (auto it = map_to_sessions.begin(); it != map_to_sessions.end(); ++it) {
		for (std::shared_ptr<model::GameSession> session : it->second) {
			// Сохраняем предыдущие позиции собак
            for (std::shared_ptr<model::Dog> dog : session->GetDogs()) {
                dog->SetPrevPosition(dog->GetDogPosition());
            }
            
            // Обновление позиций собак
            for (std::shared_ptr<model::Dog> dog : session->GetDogs()) {
                dog->MoveDogByTick(time_delta.count(), session->GetMap()->GetPointToRoadSegments());
            }

            // Удаляем неактивных собак и их игроков
            std::vector<std::shared_ptr<model::Dog>> inactive_dogs = session->RemoveInactiveDogs(retirement_time);
            for (std::shared_ptr<model::Dog> dog : inactive_dogs) {
                RetirePlayer(dog);
            }

            // Обработка коллизий
            session->HandleCollisions();

			// Генерация новых потерянных предметов
            unsigned loot_amount = session->GetLostObjects().size();
            unsigned looter_amount = session->GetDogs().size();
            unsigned new_loot = loot_generator_.Generate(time_delta, loot_amount, looter_amount);
            
            if (new_loot > 0) {
                session->GenerateLoot(new_loot);
            }
		}
	}
    if (listener_) {
        listener_->OnTick(time_delta);
    }
}

} // namespace application