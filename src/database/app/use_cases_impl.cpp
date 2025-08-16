#include "use_cases_impl.h"

#include <ranges>
#include <algorithm>
#include <sstream>
#include <optional>

namespace app {

void UseCasesImpl::RetirePlayer(const domain::Player& player) {
    player_repository_.RetirePlayer(player);
}

std::vector<domain::Player> UseCasesImpl::GetRecordsTable(size_t offset, size_t limit) const {
    return player_repository_.GetRecordsTable(offset, limit);
}

} // namespace app