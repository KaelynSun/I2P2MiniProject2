#ifndef PLAYSCENE_HPP
#define PLAYSCENE_HPP
#include <allegro5/allegro_audio.h>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include <deque>

#include "Engine/IScene.hpp"
#include "Engine/Point.hpp"
#include "Enemy/Enemy.hpp"

class Turret;
namespace Engine {
    class Group;
    class Image;
    class Label;
    class Sprite;
    class ImageButton;
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
    int totalScore = 0;
    bool shovelMode = false;
    bool wrenchMode = false;
    float wrenchUpgradeAmount = 10.0f;
    float waveEndDelayTimer;
    Enemy* hoveredEnemy = nullptr;
    void showAccountDialog();
    void showLoginDialog();
    void savePlayerProgress();

protected:
    int lives;
    int money;
    int SpeedMult;
    Turret* selectedTurret = nullptr;

public:
    void FreeMapTile(int x, int y);
    // Construction phase
    bool effectsPlaying = false;
    enum class GamePhase { CONSTRUCTION, WAVE };
    bool gameStarted = false;
    GamePhase currentPhase;
    int currentWave;
    float waveTimer;
    float constructionTimer;
    static const float WaveEndDelay;
    float postWaveDelayTimer; // New timer for delay after wave ends
    static const float ConstructionTime; // 10 seconds construction phase
    // const float WaveInterval = 5.0f; // 5 seconds between waves
    
    // Enemy wave data
    std::vector<std::deque<std::pair<int, float>>> allEnemyWaves;

    // Turret stats
    struct TurretBtnInfo {
        int x, y, w, h;
        std::string name;
        int atk, hp;
        std::string description;
    };
    void ShowUpgradeUI(int turretType);
    static int machineGunUpgradeLevel;
    static int laserUpgradeLevel;
    static int rocketUpgradeLevel;
    static int pierceUpgradeLevel;
    static float machineGunDamageMultiplier;
    static float laserDamageMultiplier;
    static float rocketDamageMultiplier;
    static float pierceDamageMultiplier;
    void ClearTurretInfo();
    void ShowTurretInfo(const TurretBtnInfo& btn, int mx, int my, Turret* actualTurret = nullptr);
    std::vector<Engine::Label*> turretInfoLabels; // To keep track of all info labels

    static const std::vector<TurretBtnInfo> turretBtnInfos; // Turret button info for UI buttons

    int enemiesKilled;
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
    Engine::Image *imgShovel;
    Engine::Image *imgWrench;
    Engine::Sprite *dangerIndicator;
    Turret *preview;
    std::vector<std::vector<TileType>> mapState;
    std::vector<std::vector<int>> mapDistance;
    std::deque<std::pair<int, float>> enemyWaveData;
    std::list<int> keyStrokes;

    Engine::ImageButton* upgradeButton = nullptr;  // Add upgrade button image pointer

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
    void EarnMoney(int money);
    void AddScore(int points);
    void OnEnemyDefeated(Enemy* enemy);
    void ReadMap();
    void ReadEnemyWave();
    void ConstructUI();
    void SaveScore(int score, const std::string& playerName);
    void BuffNearbyAllies(Enemy* supportEnemy);
    void UIBtnClicked(int id);
    bool CheckSpaceValid(int x, int y);
    std::vector<std::vector<int>> CalculateBFSDistance();
    static float CalculateDistance(const Engine::Point& p1, const Engine::Point& p2);
    // void ModifyReadMapTiles();
};
#endif   // PLAYSCENE_HPP
