#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <functional>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>

#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "PlayScene.hpp"
#include "InputDataScene.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include "UI/Component/Slider.hpp"

void InputDataScene::Initialize() {
    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;
    cornerR = 15.0f;
    boxColor = al_map_rgb(50, 50, 50);
    textInputActive = false;
    playerName = "";
    cursorTimer = 0;
    showCursor = false;
    Engine::ImageButton *btn;

    // Title
    AddNewObject(new Engine::Label("Enter Your Name", "pirulen.ttf", 48, halfW, halfH / 3 + 150, 255, 255, 255, 255, 0.5, 0.5));

    // Continue button
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW - 200, halfH * 3 / 2 - 50, 400, 100);
    btn->SetOnClickCallback(std::bind(&InputDataScene::BackOnClick, this, 1));
    AddNewControlObject(btn);
    AddNewObject(new Engine::Label("Continue", "pirulen.ttf", 48, halfW, halfH * 3 / 2, 0, 0, 0, 255, 0.5, 0.5));
    bgmInstance = AudioHelper::PlaySample("select.ogg", true, AudioHelper::BGMVolume);

    al_set_target_bitmap(al_get_backbuffer(al_get_current_display()));
}
void InputDataScene::Terminate() {
    AudioHelper::StopSample(bgmInstance);
    bgmInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
    IScene::Terminate();
}
void InputDataScene::BackOnClick(int stage) {
    PlayScene* playScene = dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetScene("play"));
    if (!playScene) return;

    currStage = playScene->MapId;
    
    // Calculate score
    int score = playScene->GetLives() * 10 + playScene->GetEnemiesKilled() * 5 + playScene->GetMoney() * 2;

    std::string filename = "Resource/Scoreboard" + std::to_string(currStage) + ".txt";

    std::time_t now = std::time(nullptr);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %H:%M:%S", std::localtime(&now));

    // Open once with append
    std::ofstream outFile(filename, std::ios::app);
    if (outFile.is_open()) {
        outFile << playerName << " " << score << " " << timeStr << std::endl;
        outFile.flush();
        outFile.close();
    }

    std::string scoreboardSceneName = "scoreboard-scene" + std::to_string(currStage);
    Engine::GameEngine::GetInstance().ChangeScene(scoreboardSceneName);
}
void InputDataScene::PlayOnClick(int stage) {
    PlayScene *scene = dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetScene("play"));
    scene->MapId = stage;
    Engine::GameEngine::GetInstance().ChangeScene("play");
}
void InputDataScene::ScoreboardOnClick() {
    //if(playerName.empty()) return;
    
    PlayScene* playScene = dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetScene("play"));
    if (!playScene) return;
    
    // Calculate score
    int score = playScene->GetLives() * 10 + playScene->GetEnemiesKilled() * 5 + playScene->GetMoney() * 2;

    // Open once with append
    std::ofstream outFile("scoreboard.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << playerName << " " << score << std::endl;
        outFile.flush();
        outFile.close();
    }

    Engine::GameEngine::GetInstance().ChangeScene("scoreboard-scene1");
}
void InputDataScene::BGMSlideOnValueChanged(float value) {
    AudioHelper::ChangeSampleVolume(bgmInstance, value);
    AudioHelper::BGMVolume = value;
}
void InputDataScene::SFXSlideOnValueChanged(float value) {
    AudioHelper::SFXVolume = value;
}
void InputDataScene::Draw() const {
    IScene::Draw();

    float boxX = halfW + 400; // Position of the text input box
    float boxY = halfH + 390;
    float boxW = 800;
    float boxH = 100;

    // Draw rounded input box
    DrawRoundedBox(boxX, boxY, boxW, boxH, cornerR, boxColor);

    int boxCenterX = halfW + 400 + 800 / 2;
    int boxCenterY = halfH + 390 + 100 / 2;
    float textX = boxCenterX;
    float textY = boxCenterY;

    std::shared_ptr<ALLEGRO_FONT> font = Engine::Resources::GetInstance().GetFont("pirulen.ttf", 32);
    const char* displayText = playerName.empty() ? "Click here to type..." : playerName.c_str(); // Placeholder when empty
    ALLEGRO_COLOR textColor = playerName.empty() ? al_map_rgb(150, 150, 150) : al_map_rgb(255, 255, 255);
    int textWidth = al_get_text_width(font.get(), displayText);
    int textHeight = al_get_font_line_height(font.get());
    al_draw_text(font.get(), textColor, textX, textY - 17, ALLEGRO_ALIGN_CENTER, displayText); // Draw text

    // Draw blinking cursor if textInputActive
    if (textInputActive && showCursor) {
        int textWidth = al_get_text_width(font.get(), playerName.c_str());
        float cursorX = textX + textWidth / 2;
        float cursorTop = textY - al_get_font_ascent(font.get()) + 10.0f;
        float cursorBottom = textY + al_get_font_descent(font.get()) + 10.0f;
        al_draw_line(cursorX, cursorTop, cursorX, cursorBottom, al_map_rgb(255, 255, 255), 4.5f); // Draw cursor
    }
}
void InputDataScene::Update(float deltaTime) {
    IScene::Update(deltaTime);
    
    // Handle cursor blinking
    cursorTimer += deltaTime;
    if (cursorTimer > 0.5f) {
        cursorTimer = 0;
        showCursor = !showCursor;
    }
    
    if (!textInputActive) showCursor = false;
}
void InputDataScene::OnMouseDown(int button, int mx, int my) {
    IScene::OnMouseDown(button, mx, my);

    float boxX = halfW + 400 - 800/2; // Center the box around halfW + 400
    float boxY = halfH + 390 - 100/2; // Center the box around halfH + 390
    float boxW = 800;
    float boxH = 100;

    if (button == 1 && mx >= boxX && mx <= boxX + boxW &&
        my >= boxY && my <= boxY + boxH) {
        textInputActive = true;
        cursorTimer = 0; 
        showCursor = true; 
    } else {
        textInputActive = false;
        showCursor = false; // hide cursor immediately if deactivated
    }
}
void InputDataScene::OnKeyDown(int keyCode) {
    IScene::OnKeyDown(keyCode);

    if (!textInputActive) return;

    if (keyCode == ALLEGRO_KEY_BACKSPACE && !playerName.empty()) {
        playerName.pop_back();
    } else if (keyCode >= ALLEGRO_KEY_A && keyCode <= ALLEGRO_KEY_Z) {
        if (playerName.length() < 13) {
            char c = 'a' + (keyCode - ALLEGRO_KEY_A); // Convert keyCode to letter
            playerName += c;
        }
    } else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
        if (playerName.length() < 13) {
            char c = '0' + (keyCode - ALLEGRO_KEY_0); // Convert keyCode to digit
            playerName += c;
        }
    } else if (keyCode == ALLEGRO_KEY_SPACE) {
        if (playerName.length() < 13) {
            playerName += ' '; // Add space
        }
    }
}
void InputDataScene::OnTextInput(const ALLEGRO_EVENT& event) {
     if (textInputActive && playerName.length() < 13) {
        // Only allow printable characters
        if (event.keyboard.unichar >= 32 && event.keyboard.unichar <= 126) {
            playerName += static_cast<char>(event.keyboard.unichar); // Add character to playerName
        }
    }
}
void InputDataScene::DrawRoundedBox(float x, float y, float w, float h, float radius, ALLEGRO_COLOR color) const {
    // Main rectangle
    al_draw_filled_rectangle(x + radius, y, x + w - radius, y + h, color);
    al_draw_filled_rectangle(x, y + radius, x + w, y + h - radius, color);
    
    // Rounded corners
    al_draw_filled_circle(x + radius, y + radius, radius, color);
    al_draw_filled_circle(x + w - radius, y + radius, radius, color);
    al_draw_filled_circle(x + radius, y + h - radius, radius, color);
    al_draw_filled_circle(x + w - radius, y + h - radius, radius, color);
    
    // Border
    al_draw_rounded_rectangle(x, y, x + w, y + h, radius, radius, al_map_rgb(250, 250, 250), 5.0f);
}