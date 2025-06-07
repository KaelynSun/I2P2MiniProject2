#include <allegro5/allegro_audio.h>
#include <functional>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#define turun 100

#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "PlayScene.hpp"
#include "ScoreBoardScene2.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include "UI/Component/Slider.hpp"

void ScoreBoardScene2::Initialize() {
    LoadLeaderboardData("Resource/scoreboard2.txt");

    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;
    Engine::ImageButton *btn;

    // Back button
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW - 200, halfH * 3 / 2 - 50, 400, 100);
    btn->SetOnClickCallback(std::bind(&ScoreBoardScene2::BackOnClick, this));
    AddNewControlObject(btn);

    // Prev
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW - 700, halfH * 3 / 2 - 50, 400, 100);
    btn->SetOnClickCallback(std::bind(&ScoreBoardScene2::PrevOnClick, this));
    AddNewControlObject(btn);

    // Next
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW + 300, halfH * 3 / 2 - 50, 400, 100);
    btn->SetOnClickCallback(std::bind(&ScoreBoardScene2::NextOnClick, this));
    AddNewControlObject(btn);

    AddNewObject(new Engine::Label("Back", "pirulen.ttf", 48, halfW, halfH * 3 / 2, 0, 0, 0, 255, 0.5, 0.5));
    AddNewObject(new Engine::Label("Prev Page", "pirulen.ttf", 48, halfW-500, halfH * 3 / 2, 0, 0, 0, 255, 0.5, 0.5));
    AddNewObject(new Engine::Label("Next page", "pirulen.ttf", 48, halfW+500, halfH * 3 / 2, 0, 0, 0, 255, 0.5, 0.5));
    AddNewObject(new Engine::Label("STAGE 2", "pirulen.ttf", 60, halfW, halfH / 3 - 50, 10, 255, 255, 255, 0.5, 0.5));
    AddNewObject(new Engine::Label("Leaderboard", "pirulen.ttf", 48, halfW, 100+turun, 255, 255, 0, 255, 0.5, 0.5));

    AddNewObject(new Engine::Label("Name", "pirulen.ttf", 24, 360, 180+turun, 255, 255, 255, 255, 0.0, 0.5)); // Name header
    AddNewObject(new Engine::Label("Score", "pirulen.ttf", 24, w - 805, 180+turun, 255, 255, 255, 255, 0.5, 0.5)); // Score header
    AddNewObject(new Engine::Label("Timestamp", "pirulen.ttf", 24, w-500, 180+turun, 255, 255, 255, 255, 0.5, 0.5));

    // Leaderboard data
    for (size_t i = 0; i < 5; ++i) {
        const auto& [name, score, timestamp] = leaderboardData[i];
        float nameX = 360;
        float scoreX = w - 805;
        float timeX = w-500;
        float space = 40;
        float top = 250 + turun;
        float y = top + i * space;

        AddNewObject(new Engine::Label(name, "pirulen.ttf", 24, nameX, y, 255, 255, 255, 180, 0.0, 0.5));
        AddNewObject(new Engine::Label(std::to_string(score), "pirulen.ttf", 24, scoreX, y, 255, 255, 255, 180, 0.5, 0.5));
        AddNewObject(new Engine::Label(timestamp, "pirulen.ttf", 24, timeX, y, 255, 255, 255, 180, 0.5, 0.5));
    }

    // Not safe if release resource while playing, however we only free while change scene, so it's fine.
    bgmInstance = AudioHelper::PlaySample("select.ogg", true, AudioHelper::BGMVolume);
}
void ScoreBoardScene2::Terminate() {
    AudioHelper::StopSample(bgmInstance);
    bgmInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
    IScene::Terminate();
}
void ScoreBoardScene2::BackOnClick() {
    Engine::GameEngine::GetInstance().ChangeScene("start");
}
void ScoreBoardScene2::PrevOnClick() {
    Engine::GameEngine::GetInstance().ChangeScene("scoreboard-scene1");
}
void ScoreBoardScene2::NextOnClick() {
    Engine::GameEngine::GetInstance().ChangeScene("scoreboard-scene1");
}
void ScoreBoardScene2::LoadLeaderboardData(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    leaderboardData.clear(); // Clear any existing data
    
    std::vector<std::tuple<std::string, int, std::string>> tempData;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string name;
        int score;
        std::string timestamp;

        if (iss >> name >> score) {
            std::getline(iss, timestamp);
            // Remove leading space if exists
            if (!timestamp.empty() && timestamp[0] == ' ') {
                timestamp.erase(0, 1);
            }
            tempData.emplace_back(name, score, timestamp);
        }
    }
    
    // Reverse the data so newest entries are first
    std::reverse(tempData.begin(), tempData.end());
    
    // Now sort by score, but keep the original order (newest first) for equal scores
    std::stable_sort(tempData.begin(), tempData.end(), [](const auto& a, const auto& b) {
        return std::get<1>(a) > std::get<1>(b);
    });
    
    // Copy to leaderboardData
    leaderboardData = std::move(tempData);
    
    // Keep only top 5
    if (leaderboardData.size() > 5) {
        leaderboardData.resize(5);
    }
}
void ScoreBoardScene2::BGMSlideOnValueChanged(float value) {
    AudioHelper::ChangeSampleVolume(bgmInstance, value);
    AudioHelper::BGMVolume = value;
}
void ScoreBoardScene2::SFXSlideOnValueChanged(float value) {
    AudioHelper::SFXVolume = value;
}

