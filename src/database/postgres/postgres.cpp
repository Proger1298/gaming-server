#include "postgres.h"

#include <stdexcept>
#include <iostream>
#include "../../logger.h"

namespace postgres {

using namespace logger;
using namespace std::literals;
using pqxx::operator"" _zv;

Database::Database(const std::string& db_url)
: connection_pool_{
    std::thread::hardware_concurrency(),
    [db_url = std::move(db_url)]() {
        return std::make_shared<pqxx::connection>(db_url);
    }
}  
{
    postgres::ConnectionPool::ConnectionWrapper conn = connection_pool_.GetConnection();
    {
        pqxx::work work{*conn};

        work.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id SERIAL PRIMARY KEY,
                name VARCHAR(100) NOT NULL,
                score INTEGER NOT NULL,
                play_time DOUBLE PRECISION NOT NULL
            );
            
            CREATE INDEX IF NOT EXISTS retired_players_score_play_time_name_idx 
            ON retired_players (score DESC, play_time ASC, name ASC);
        )");

        work.commit();
    }
}

void PlayerRepositoryImpl::RetirePlayer(const domain::Player& player) {
    postgres::ConnectionPool::ConnectionWrapper conn = connection_pool_.GetConnection();
    {
        pqxx::work work{*conn};

        work.exec_params(
            "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
            player.GetName(), player.GetScore(), player.GetPlayTime()
        );

        work.commit();
    }
}

std::vector<domain::Player> PlayerRepositoryImpl::GetRecordsTable(size_t offset, size_t limit) const {
    postgres::ConnectionPool::ConnectionWrapper conn = connection_pool_.GetConnection();
    std::vector<domain::Player> players;
    {
        pqxx::work work{*conn};

        if (limit > 100) {
            // Логируем ошибку и возвращаем пустой вектор
            LOG_WITH_DATA(error, json::object{}, "Limit cannot exceed 100"s);
            return {};
        }
        
        pqxx::result result = work.exec_params(
            "SELECT name, score, play_time FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC "
            "LIMIT $1 OFFSET $2",
            limit,
            offset
        );
        
        work.commit();
        
        for (const pqxx::row& row : result) {
            players.emplace_back(
                row["name"].as<std::string>(),
                row["score"].as<size_t>(),
                row["play_time"].as<double>()
            );
        }
    }

    return players;
}

} // namespace postgres