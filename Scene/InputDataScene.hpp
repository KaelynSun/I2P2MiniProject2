#ifndef INPUTDATASCENE_HPP
#define INPUTDATASCENE_HPP
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_primitives.h>
#include <memory>
#include <string>

#include "Engine/IScene.hpp"

class InputDataScene final : public Engine::IScene {
private:
    std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> bgmInstance;
    std::string playerName;
    bool textInputActive;
    float cursorTimer;
    bool showCursor;
    ALLEGRO_COLOR boxColor;
    float cornerR;
    int halfW, halfH;
    
    void DrawRoundedBox(float x, float y, float w, float h, float radius, ALLEGRO_COLOR color) const;

public:
    int currStage;
    explicit InputDataScene() = default;
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Draw() const override;
    
    void OnMouseDown(int button, int mx, int my) override;
    void OnKeyDown(int keyCode) override;
    void OnTextInput(const ALLEGRO_EVENT& event);
    
    void PlayOnClick(int stage);
    void ScoreboardOnClick();
    void BackOnClick(int stage);

    void BGMSlideOnValueChanged(float value);
    void SFXSlideOnValueChanged(float value);
};

#endif   // SCOREBOARDSCENE_HPP
