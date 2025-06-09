#include <functional>
#include <string>
#include <allegro5/allegro_primitives.h>

#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "PlayScene.hpp"
#include "UI/Component/Image.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include "WinScene.hpp"

void WinScene::Initialize() {
    al_init_primitives_addon();
    Clear();
    ticks = 0;
    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;
    
    AddNewObject(new Engine::Image("win/benjamin-sad.png", halfW, halfH, 0, 0, 0.5, 0.5));
    if(!nameEntered){

        AddNewObject(new Engine::Label("You Win!", "pirulen.ttf", 48, halfW, halfH / 4 - 10, 255, 255, 255, 255, 0.5, 0.5));
        
    } else{
        AddNewObject(new Engine::Label("You Win!", "pirulen.ttf", 48, halfW, halfH / 4 - 10, 255, 255, 255, 255, 0.5, 0.5));

        Engine::ImageButton *btn;
        btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200, halfH * 7 / 4 - 50, 400, 100);
        btn->SetOnClickCallback(std::bind(&WinScene::BackOnClick, this, 2));
        AddNewControlObject(btn);
        AddNewObject(new Engine::Label("Back", "pirulen.ttf", 48, halfW, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
    }
    bgmId = AudioHelper::PlayAudio("win.wav");

}

void WinScene::OnKeyDown(int keyCode){
    if(!nameEntered){
       if(keyCode >= ALLEGRO_KEY_A && keyCode <= ALLEGRO_KEY_Z){
        playerName += (char)('A' + (keyCode - ALLEGRO_KEY_A));
       } else if(keyCode == ALLEGRO_KEY_BACKSPACE && !playerName.empty()){
            playerName.pop_back();
       } else if(keyCode == ALLEGRO_KEY_ENTER && !playerName.empty()){
            nameEntered = true;
            auto *playScene = dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetScene("play"));
            if(playScene){
                playScene->SaveScore(finalScore, playerName);
            }
            Initialize();
       }
    }
}

void WinScene::SetFinalScore(int score) {
    finalScore = score;
}

void WinScene::Draw() const {
    if (!nameEntered) {
        // First: draw background objects (not text box or input labels)
        // This ensures benjamin-sad.png and others are not drawn over the box
        IScene::Draw();

        // Draw the black box
        int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
        int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
        int halfW = w / 2;
        int halfH = h / 2;

        int boxWidth = 1000;
        int boxHeight = 600;
        int boxX = halfW - boxWidth / 2;
        int boxY = halfH / 2 - 50;

        al_draw_filled_rounded_rectangle(
            boxX, boxY, boxX + boxWidth, boxY + boxHeight, 10, 10, al_map_rgb(0, 0, 0)
        );

        al_draw_rounded_rectangle(
            boxX, boxY, boxX + boxWidth, boxY + boxHeight, 10, 10, al_map_rgb(57, 255, 20), 2.0f
        );

        // Draw the input-related labels manually on top
        Engine::Label label1("Enter your name: ", "pirulen.ttf", 36, halfW, halfH, 255, 255, 255, 255, 0.5, 0.5);
        Engine::Label label2(playerName + "_", "pirulen.ttf", 36, halfW, halfH + 50, 255, 255, 255, 255, 0.5, 0.5);
        Engine::Label label3("press ENTER when done", "pirulen.ttf", 24, halfW, halfH + 100, 255, 255, 255, 255, 0.5, 0.5);

        label1.Draw();
        label2.Draw();
        label3.Draw();
    } else {
        // After name is entered, just draw all UI normally
        IScene::Draw();
    }
}


void WinScene::Terminate() {
    IScene::Terminate();
    AudioHelper::StopBGM(bgmId);
}
void WinScene::Update(float deltaTime) {
    ticks += deltaTime;
    if (ticks > 4 && ticks < 100 &&
        dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetScene("play"))->MapId == 2) {
        ticks = 100;
        bgmId = AudioHelper::PlayBGM("happy.ogg");
    }


    if(!nameEntered && ticks > 0.5f){
        ticks = 0;
        Initialize();
    }
}

void WinScene::BackOnClick(int stage) {
    // Change to select scene.
    Engine::GameEngine::GetInstance().ChangeScene("stage-select");
}

void WinScene::SetPlayerName(const std::string &name){
    playerName = name;
}
