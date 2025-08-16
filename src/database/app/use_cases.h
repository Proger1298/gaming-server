#pragma once

#include <string>
#include <vector>

#include "../domain/player_repository.h"

namespace app {

class UseCases {
public:
    virtual void RetirePlayer(const domain::Player& player) = 0;
    virtual std::vector<domain::Player> GetRecordsTable(size_t offset, size_t limit) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app