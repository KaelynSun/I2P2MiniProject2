#include "LoginScene.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "Engine/LOG.hpp"
#include "LocalAccount.h"
#include <allegro5/allegro_primitives.h>

void LoginScene::Initialize() {
    loginComplete = false;
    username.clear();
    password.clear();
    enteringPassword = false;
    showError = false;
    errorMessage = "";

    al_init_primitives_addon();
    bgmId = AudioHelper::PlayBGM("play.ogg");

    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int centerX = w / 2;
    int centerY = h / 2;

    // Create Create Account button following the same pattern
    btnCreateAccount = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", 
                                             centerX - 200, centerY + 180, 400, 100);
    btnCreateAccount->SetOnClickCallback(std::bind(&LoginScene::OnCreateAccountClick, this));
    AddNewControlObject(btnCreateAccount);
    AddNewObject(new Engine::Label("Create Account", "pirulen.ttf", 30, 
                                 centerX, centerY + 240, 0, 0, 0, 255, 0.5, 0.5));
}

void LoginScene::Terminate() {
    AudioHelper::StopBGM(bgmId);
    IScene::Terminate();
}

void LoginScene::ClearError() {
    showError = false;
    errorMessage.clear();
}

void LoginScene::OnKeyDown(int keyCode) {
    if (loginComplete) return;

    std::string& currentInput = enteringPassword ? password : username;

    if ((keyCode >= ALLEGRO_KEY_A && keyCode <= ALLEGRO_KEY_Z) || 
        keyCode == ALLEGRO_KEY_BACKSPACE) {
        ClearError();
    }

    if (keyCode >= ALLEGRO_KEY_A && keyCode <= ALLEGRO_KEY_Z) {
        currentInput += (char)('A' + (keyCode - ALLEGRO_KEY_A));
    } else if (keyCode == ALLEGRO_KEY_BACKSPACE && !currentInput.empty()) {
        currentInput.pop_back();
    } else if (keyCode == ALLEGRO_KEY_TAB) {
        enteringPassword = !enteringPassword;
    } else if (keyCode == ALLEGRO_KEY_ENTER) {
        if (!username.empty() && !password.empty()) {
            if (accountManager.login(username, password)) {
                loginComplete = true;
                Engine::GameEngine::GetInstance().ChangeScene("stage-select");
            } else {
                showError = true;
                errorMessage = "Login failed! Try again or create an account.";
                password.clear();
            }
        }
    }
}

void LoginScene::OnCreateAccountClick() {
    if (username.empty() || password.empty()) {
        showError = true;
        errorMessage = "Please enter both username and password!";
        return;
    }

    if (password.length() < 4) {
        showError = true;
        errorMessage = "Password must be at least 4 characters!";
        return;
    }

    if (accountManager.createAccount(username, password)) {
        // Try to automatically login after successful creation
        if (accountManager.login(username, password)) {
            loginComplete = true;
            Engine::GameEngine::GetInstance().ChangeScene("stage-select");
        } else {
            showError = true;
            errorMessage = "Account created! Please login.";
            password.clear();
        }
    } else {
        showError = true;
        errorMessage = "Username already taken! Try a different one.";
    }
}

void LoginScene::Update(float) {}

void LoginScene::Draw() const {
    IScene::Draw();

    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int centerX = w / 2;
    int centerY = h / 2;

    // Draw login box
    al_draw_filled_rounded_rectangle(centerX - 400, centerY - 200, centerX + 400, centerY + 200, 10, 10, al_map_rgb(0, 0, 0));
    al_draw_rounded_rectangle(centerX - 400, centerY - 200, centerX + 400, centerY + 200, 10, 10, al_map_rgb(255, 255, 255), 2);

    Engine::Label title("Login", "pirulen.ttf", 48, centerX, centerY - 150, 255, 255, 255, 255, 0.5, 0.5);
    Engine::Label userLabel("Username:", "pirulen.ttf", 24, centerX - 150, centerY - 70, 255, 255, 255, 255, 0, 0.5);
    Engine::Label userInput(username + (enteringPassword ? "" : "_"), "pirulen.ttf", 24, centerX, centerY - 40, 255, 255, 255, 255, 0.5, 0.5);
    
    Engine::Label passLabel("Password:", "pirulen.ttf", 24, centerX - 150, centerY + 20, 255, 255, 255, 255, 0, 0.5);
    std::string maskedPass(password.length(), '*');
    Engine::Label passInput(maskedPass + (enteringPassword ? "_" : ""), "pirulen.ttf", 24, centerX, centerY + 50, 255, 255, 255, 255, 0.5, 0.5);

    Engine::Label hintLabel("[TAB] to switch, [ENTER] to login", "pirulen.ttf", 20, centerX, centerY + 110, 128, 255, 128, 255, 0.5, 0.5);

    title.Draw();
    userLabel.Draw();
    userInput.Draw();
    passLabel.Draw();
    passInput.Draw();
    hintLabel.Draw();

    if (showError) {
        Engine::Label error(errorMessage, "pirulen.ttf", 20, centerX, centerY + 160, 255, 100, 100, 255, 0.5, 0.5);
        error.Draw();
    }
}