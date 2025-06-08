#ifndef PLAYSCENE_HPP
#define PLAYSCENE_HPP
#include <allegro5/allegro_audio.h>
#include <deque>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "Engine/IScene.hpp"
#include "Engine/Point.hpp"

class Turret;
namespace Engine {
    class Group;
    class Image;
    class Label;
    class Sprite;
}   // namespace Engine

class PlayScene final : public Engine::IScene {
private:
    enum TileType {
        TILE_DIRT,
        TILE_FLOOR,
        TILE_OCCUPIED,
    };
    ALLEGRO_SAMPLE_ID bgmId;
    std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> deathBGMInstance;
    bool changeToWinScene = false;
    bool shovelActive = false;
    //Shovel* shovel = nullptr;

protected:
    int lives;
    int money;
    int SpeedMult;

public:
    // Construction phase
    enum class GamePhase { CONSTRUCTION, WAVE };
    bool gameStarted = false;
    GamePhase currentPhase;
    int currentWave;
    float waveTimer;
    float constructionTimer;
    float postWaveDelayTimer; // New timer for delay after wave ends
    static const float ConstructionTime; // 10 seconds construction phase
    // const float WaveInterval = 5.0f; // 5 seconds between waves
    
    // Enemy wave data
    std::vector<std::deque<std::pair<int, float>>> allEnemyWaves;
    std::deque<std::pair<int, float>> enemyWaveData;

    int enemiesKilled = 0;
    static bool DebugMode;
    static bool paused;
    static const std::vector<Engine::Point> directions;
    static const int MapWidth, MapHeight;
    static const int BlockSize;
    static const float DangerTime;
    static const Engine::Point SpawnGridPoint;
    static const Engine::Point EndGridPoint;
    static const std::vector<int> code;
    int MapId;
    float ticks;
    float deathCountDown;
    // Map tiles.
    Group *TileMapGroup;
    Group *GroundEffectGroup;
    Group *DebugIndicatorGroup;
    Group *BulletGroup;
    Group *TowerGroup;
    Group *EnemyGroup;
    Group *EffectGroup;
    Group *UIGroup;
    Engine::Label *UIMoney;
    Engine::Label *UILives;
    Engine::Label* controlsLabel;
    Engine::Label* constructionTimerLabel; // Label for construction timer countdown
    Engine::Image *imgTarget;
    Engine::Sprite *dangerIndicator;
    Turret *preview; // Pointer to turret
    std::vector<std::vector<TileType>> mapState;
    std::vector<std::vector<int>> mapDistance;
    std::list<int> keyStrokes;
    static Engine::Point GetClientSize();
    explicit PlayScene() = default;
    void Initialize() override;
    void Terminate() override;
    void Update(float deltaTime) override;
    void Draw() const override;
    void OnMouseDown(int button, int mx, int my) override;
    void OnMouseMove(int mx, int my) override;
    void OnMouseUp(int button, int mx, int my) override;
    void OnKeyDown(int keyCode) override;
    void Hit();
    int GetMoney() const;
    int GetLives() const;
    int GetEnemiesKilled() const;
    void EarnMoney(int money);
    void ReadMap();
    void ReadEnemyWave();
    void ConstructUI();
    void UIBtnClicked(int id);
    bool CheckSpaceValid(int x, int y);
    std::vector<std::vector<int>> CalculateBFSDistance();
    void SetShovelActive(bool active) { shovelActive = active; }
    static float CalculateDistance(const Engine::Point& p1, const Engine::Point& p2);
    // void ModifyReadMapTiles();
};
#endif   // PLAYSCENE_HPP
