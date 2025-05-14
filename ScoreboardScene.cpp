#include "ScoreboardScene.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include <fstream>
#include <algorithm>
#include <ctime>

void ScoreboardScene::Initialize() {
    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;

    // Load existing scores
    LoadScores();
    SortScores();

    // Title
    AddNewObject(new Engine::Label("scoreboard-scene", "pirulen.ttf", 72, halfW, 60, 255, 255, 255, 255, 0.5, 0.5));

    // Display current page scores
    int startIdx = currentPage * entriesPerPage;
    for (int i = 0; i < entriesPerPage && startIdx + i < scores.size(); i++) {
        const auto& entry = scores[startIdx + i];
        int yPos = 150 + i * 60;
        
        // Rank
        AddNewObject(new Engine::Label(
            std::to_string(startIdx + i + 1) + ".", 
            "pirulen.ttf", 36, halfW - 250, yPos, 255, 255, 255, 255
        ));
        
        // Player name and details
        std::string details = entry.playerName + " - Stage " + std::to_string(entry.stage) + 
                             " - " + entry.timestamp;
        AddNewObject(new Engine::Label(
            details, "pirulen.ttf", 24, halfW - 100, yPos, 255, 255, 255, 255
        ));
        
        // Score
        AddNewObject(new Engine::Label(
            std::to_string(entry.score), "pirulen.ttf", 36, halfW + 250, yPos, 255, 215, 0, 255
        ));
    }

    // Navigation buttons
    Engine::ImageButton* btn;

    // Previous Page
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW - 300, h - 100, 150, 50);
    btn->SetOnClickCallback(std::bind(&ScoreboardScene::PrevPageOnClick, this));
    AddNewControlObject(btn);
    AddNewObject(new Engine::Label("PREV PAGE", "pirulen.ttf", 24, halfW - 225, h - 75, 0, 0, 0, 255, 0.5, 0.5));

    // Next Page
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW + 150, h - 100, 150, 50);
    btn->SetOnClickCallback(std::bind(&ScoreboardScene::NextPageOnClick, this));
    AddNewControlObject(btn);
    AddNewObject(new Engine::Label("NEXT PAGE", "pirulen.ttf", 24, halfW + 225, h - 75, 0, 0, 0, 255, 0.5, 0.5));

    // Back Button
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", 50, h - 70, 100, 50);
    btn->SetOnClickCallback(std::bind(&ScoreboardScene::BackOnClick, this));
    AddNewControlObject(btn);
    AddNewObject(new Engine::Label("BACK", "pirulen.ttf", 24, 100, h - 45, 0, 0, 0, 255, 0.5, 0.5));

    bgmInstance = AudioHelper::PlaySample("select.ogg", true, AudioHelper::BGMVolume);
}

void ScoreboardScene::Terminate() {
    AudioHelper::StopSample(bgmInstance);
    bgmInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
    IScene::Terminate();
}

void ScoreboardScene::BackOnClick() {
    Engine::GameEngine::GetInstance().ChangeScene("stage-select");
}

void ScoreboardScene::NextPageOnClick() {
    if ((currentPage + 1) * entriesPerPage < scores.size()) {
        currentPage++;
        Initialize(); // Re-initialize to refresh display
    }
}

void ScoreboardScene::PrevPageOnClick() {
    if (currentPage > 0) {
        currentPage--;
        Initialize(); // Re-initialize to refresh display
    }
}

void ScoreboardScene::LoadScores() {
    scores.clear();
    std::ifstream file(scoreFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos1 = line.find(',');
            size_t pos2 = line.find(',', pos1 + 1);
            size_t pos3 = line.find(',', pos2 + 1);
            
            if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                ScoreEntry entry;
                entry.playerName = line.substr(0, pos1);
                entry.score = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                entry.stage = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                entry.timestamp = line.substr(pos3 + 1);
                scores.push_back(entry);
            }
        }
        file.close();
    }
}

void ScoreboardScene::SaveScores() {
    std::ofstream file(scoreFile);
    if (file.is_open()) {
        for (const auto& entry : scores) {
            file << entry.playerName << "," << entry.score << "," 
                 << entry.stage << "," << entry.timestamp << "\n";
        }
        file.close();
    }
}

void ScoreboardScene::SortScores() {
    std::sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.score > b.score; // Descending order
    });
}
