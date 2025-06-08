#pragma once
#include <string>
#include "Engine/IScene.hpp"
#include "UI/Component/Label.hpp"
#include "UI/Component/ImageButton.hpp"
#include <allegro5/allegro_audio.h> 

class LoginScene final : public Engine::IScene {
private:
    std::string username;
    std::string password;
    std::string confirmPassword;
    bool enteringPassword = false;
    bool enteringConfirmPassword = false;
    bool loginComplete = false;
    bool showError = false;
    Engine::ImageButton* btnCreateAccount;
    std::string errorMessage;
    bool creatingAccount = false;
    ALLEGRO_SAMPLE_ID bgmId;

    void OnCreateAccountClick();
    void ClearError(); // Add this

public:
    void Initialize() override;
    void Terminate() override;
    void Draw() const override;
    void Update(float deltaTime) override;
    void OnKeyDown(int keyCode) override;
    
};