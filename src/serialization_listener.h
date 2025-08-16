#pragma once

#include "application.h"
#include "model_serialization.h"
#include <filesystem>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace serialization {

class SerializingListener : public application::ApplicationListener {
public:
    SerializingListener(application::Application& app, 
                      const std::filesystem::path& state_file,
                      std::chrono::milliseconds save_period = std::chrono::milliseconds::max());

    void OnTick(std::chrono::milliseconds time_delta) override;
    void OnShutdown() override;
    bool TryLoadState();
    void SaveState();
    
private:
    application::Application& app_;
    std::filesystem::path state_file_;
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds time_since_last_save_;

    GameStateRepr CaptureState() const;
    void ApplyState(const GameStateRepr& game_state_repr);
};

} // namespace serialization