#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <vector>

#include "../domain/player_repository.h"

#include "connection_pool.h"

namespace postgres {

class PlayerRepositoryImpl : public domain::PlayerRepository {
public:
    explicit PlayerRepositoryImpl(ConnectionPool& connection_pool)
    : connection_pool_{connection_pool}
    {

    }

    void RetirePlayer(const domain::Player& player) override;
    std::vector<domain::Player> GetRecordsTable(size_t offset, size_t limit) const override;

private:
    ConnectionPool& connection_pool_;
}; 

class Database {
public:
    explicit Database(const std::string& db_url);

    PlayerRepositoryImpl& GetPlayerRecords() & {
        return player_repository_impl_;
    }

private:
    ConnectionPool connection_pool_;
    PlayerRepositoryImpl player_repository_impl_{connection_pool_};
};

} // namespace postgres