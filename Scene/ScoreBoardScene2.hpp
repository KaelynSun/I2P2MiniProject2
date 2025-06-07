#ifndef SCOREBOARDSCENE2_HPP
#define SCOREBOARDSCENE2_HPP
#include <vector>
#include <string>
#include <allegro5/allegro_audio.h>
#include <memory>

#include "Engine/IScene.hpp"

class ScoreBoardScene2 final : public Engine::IScene {
private:
    std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> bgmInstance;
    std::vector<std::tuple<std::string, int, std::string>> leaderboardData;

public:
    explicit ScoreBoardScene2() = default;
    void Initialize() override;
    void Terminate() override;
    void BackOnClick();
    void PrevOnClick();
    void NextOnClick();
    void LoadLeaderboardData(const std::string& filename);

    void BGMSlideOnValueChanged(float value);
    void SFXSlideOnValueChanged(float value);
};

#endif   // SCOREBOARDSCENE_HPP
