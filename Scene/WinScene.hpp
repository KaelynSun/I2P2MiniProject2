#ifndef WINSCENE_HPP
#define WINSCENE_HPP
#include "Engine/IScene.hpp"
#include <allegro5/allegro_audio.h>

class WinScene final : public Engine::IScene {
private:
    std::string playerName;
    bool nameEntered = false;
    int finalScore = 0;
    float ticks;
    ALLEGRO_SAMPLE_ID bgmId;

public:
    explicit WinScene() = default;
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void BackOnClick(int stage);
    void OnKeyDown(int keyCode) override;
    void SetFinalScore(int score);
    void Draw() const override;
    void SetPlayerName(const std::string& name);
};

#endif   // WINSCENE_HPP
