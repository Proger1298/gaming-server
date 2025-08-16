#include "serialization_listener.h"
#include "logger.h"

namespace serialization {

using namespace logger;
using namespace std::literals;

SerializingListener::SerializingListener(application::Application& app, 
                                     const std::filesystem::path& state_file,
                                     std::chrono::milliseconds save_period)
    : app_(app)
    , state_file_(state_file)
    , save_period_(save_period)
    , time_since_last_save_(0) {}

void SerializingListener::OnTick(std::chrono::milliseconds time_delta) {
    if (save_period_ == std::chrono::milliseconds::max()) {
        return;
    }

    time_since_last_save_ += time_delta;
    if (time_since_last_save_ >= save_period_) {
        SaveState();
        time_since_last_save_ = std::chrono::milliseconds(0);
    }
}

void SerializingListener::OnShutdown() {
    SaveState();
}

GameStateRepr SerializingListener::CaptureState() const {
    GameStateRepr game_state_repr;

    // Сохраняем текущие значения счетчиков ID
    game_state_repr.SetIdCounters(
        model::GameSession::GetLastSessionId(),
        model::Dog::GetLastDogId(),
        application::Player::GetLastPlayerId()
    );

    // Сохраняем данные сессий
    for (const auto& [map_id, sessions] : app_.GetMapIdToSession()) {
        for (const auto& session : sessions) {
            game_state_repr.AddSession(SessionRepr{*session});
        }
    }

    // Сохраняем данные игроков и их токены
    for (const auto& [token, player] : app_.GetPlayerTokens().GetTokenToPlayer()) {
        PlayerRepr player_repr{*player};
        player_repr.SetToken(*token);
        game_state_repr.AddPlayer(player_repr);
    }

    return game_state_repr;
}

void SerializingListener::ApplyState(const GameStateRepr& game_state_repr) {

    // Восстанавливаем счетчики ID перед созданием объектов
    const IdCountersRepr& counters = game_state_repr.GetIdCounters();
    model::GameSession::SetLastSessionId(counters.GetSessionsIds());
    model::Dog::SetLastDogId(counters.GetDogsIds());
    application::Player::SetLastPlayerId(counters.GetPlayersIds());

    // Восстанавливаем объекты
    std::unordered_map<model::LostObject::Id, model::LostObject, model::GameSession::LostObjectIdHasher> lost_objects;
    std::unordered_map<model::Dog::Id, std::shared_ptr<model::Dog>, model::Dog::DogIdHasher> dogs;
    std::unordered_map<model::GameSession::Id, std::shared_ptr<model::GameSession>, model::GameSession::GameSesionIdHasher> sessions;

    for (const SessionRepr& session_repr : game_state_repr.GetSessions()) {
        // 1. Восстанавливаем потерянные предметы
        for (const LostObjectRepr& lost_obj_repr : session_repr.GetLostObjects()) {
            lost_objects.emplace(lost_obj_repr.GetId(), lost_obj_repr.Restore());
        }

        // 2. Восстанавливаем собак
        for (const DogRepr& dog_repr : session_repr.GetDogs()) {
            dogs.emplace(dog_repr.GetId(), dog_repr.Restore());
        }

        // 3. Восстанавливаем игровые сессии
        std::shared_ptr<model::Map> map = app_.FindMap(session_repr.GetMapId());
        if (!map) continue;

        // Создаем сессию
        std::shared_ptr<model::GameSession> session = std::make_shared<model::GameSession>(
            session_repr.GetId(),
            map,
            std::make_shared<extra_data::LootTypes>(app_.GetLootTypes())
        );

        // Добавляем собак в сессию
        for (const DogRepr& dog_repr : session_repr.GetDogs()) {
            if (auto it = dogs.find(dog_repr.GetId()); it != dogs.end()) {
                session->AddDog(it->second);
            }
        }

        // Добавляем потерянные предметы в сессию
        for (const LostObjectRepr& lost_obj_repr : session_repr.GetLostObjects()) {
            if (auto it = lost_objects.find(lost_obj_repr.GetId()); it != lost_objects.end()) {
                session->AddLostObject(it->second);
            }
        }

        sessions.emplace(session_repr.GetId(), session);
        app_.AddSession(session_repr.GetMapId(), session);

    }

    // 4. Восстанавливаем игроков и их токены
    for (const PlayerRepr& player_repr : game_state_repr.GetPlayers()) {
        std::shared_ptr<application::Player> player = player_repr.Restore();

        // Связываем игрока с сессией и собакой
        if (auto session_it = sessions.find(player_repr.GetSessionId()); session_it != sessions.end()) {
            
            player->SetGameSession(session_it->second);

            if (auto dog_it = dogs.find(player_repr.GetDogId()); 
                dog_it != dogs.end()) {
                player->SetDog(dog_it->second);
            }
        }

        app_.AddPlayer(player);

        // Восстанавливаем токен
        if (!player_repr.GetToken().empty()) {
            app_.SetPlayerToken(application::Token{player_repr.GetToken()}, player);
        }
    }
}

bool SerializingListener::TryLoadState() {
    if (!std::filesystem::exists(state_file_)) {
        return true;
    }

    try {
        std::ifstream ifs(state_file_, std::ios::binary);
        if (!ifs.is_open()) {
            return false;
        }

        boost::archive::binary_iarchive ia(ifs);
        GameStateRepr state;
        ia >> state;

        ApplyState(state);
        return true;
    } catch (const std::exception& e) {
        LOG_WITH_DATA(error, json::object{}, "Failed to load state: "s + e.what());
        return false;
    }
}

void SerializingListener::SaveState() {
    if (state_file_.empty()) {
        return;
    }

    try {
        // Сначала сохраняем во временный файл
        auto temp_file = state_file_;
        temp_file += ".tmp";

        {
            std::ofstream ofs(temp_file, std::ios::binary);
            if (!ofs.is_open()) {
                throw std::runtime_error("Failed to open temp file for writing");
            }

            GameStateRepr state = CaptureState();
            boost::archive::binary_oarchive oa(ofs);
            oa << state;
        }

        // Затем атомарно переименовываем
        std::filesystem::rename(temp_file, state_file_);
    } catch (const std::exception& e) {
        LOG_WITH_DATA(error, json::object{}, "Failed to save state: "s + e.what());
    }
}

} // namespace serialization