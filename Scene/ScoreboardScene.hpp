#ifndef SCOREBOARDSCENE_HPP
#define SCOREBOARDSCENE_HPP

#include <allegro5/allegro_audio.h>
#include <vector>
#include <string>
#include <memory>
#include "Engine/IScene.hpp"

struct ScoreEntry {
    std::string playerName;
    int score;
    int stage;
    std::string timestamp;
};

class ScoreboardScene final : public Engine::IScene {
private:
    std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> bgmInstance;
    std::vector<ScoreEntry> scores;
    int currentPage = 0;
    const int entriesPerPage = 5;
    const std::string scoreFile = "scores.json";

    void LoadScores();
    void SaveScores();
    void SortScores();

public:
    explicit ScoreboardScene() = default;
    void Initialize() override;
    void Terminate() override;
    void BackOnClick();
    void NextPageOnClick();
    void PrevPageOnClick();
};

#endif // SCOREBOARDSCENE_HPP
