#include "model_serialization.h"

namespace serialization {

[[nodiscard]] std::shared_ptr<model::Dog> DogRepr::Restore() const {
    std::shared_ptr<model::Dog> dog = std::make_shared<model::Dog>(id_, name_, position_, bag_capacity_);
    dog->SetPrevPosition(prev_position_);
    dog->SetDogSpeed(speed_);
    dog->SetDirection(direction_);
    dog->AddScore(score_);
    
    for (const LostObjectRepr& item : bag_items_) {
        dog->CollectItem(item.Restore());
    }
    return dog;
}

[[nodiscard]] model::LostObject LostObjectRepr::Restore() const {
    model::LostObject obj(id_, type_, position_, value_);
    if (is_collected_) {
        obj.MarkAsCollected();
    }
    return obj;
}

[[nodiscard]] std::shared_ptr<application::Player> PlayerRepr::Restore() const {
    std::shared_ptr<application::Player> player = std::make_shared<application::Player>(id_, name_);
    return player;
}

} // namespace serialization