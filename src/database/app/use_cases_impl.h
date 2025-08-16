#pragma once

#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::PlayerRepository& player_repository)
    : player_repository_{player_repository}
    {

    }

    void RetirePlayer(const domain::Player& player) override;
    std::vector<domain::Player> GetRecordsTable(size_t offset, size_t limit) const override;

private:
    domain::PlayerRepository& player_repository_;
};

}  // namespace app