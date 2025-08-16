#pragma once

#include <string>
#include <vector>

namespace domain {

class Player {
public:
    Player(std::string name, size_t score, double play_time)
    : name_(std::move(name)) , score_(score), play_time_(play_time) 
    {

    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    size_t GetScore() const noexcept {
        return score_;
    }

    double GetPlayTime() const noexcept {
        return play_time_;
    }

private:
    std::string name_;
    size_t score_{0};
    double play_time_{0.0};
};


class PlayerRepository {
public:
    virtual void RetirePlayer(const Player& player) = 0;
    virtual std::vector<Player> GetRecordsTable(size_t offset, size_t limit) const = 0;

protected:
    ~PlayerRepository() = default;
};

}