#include <algorithm>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <queue>
#include <deque>
#include <string>
#include <vector>
#include <sstream> // <-- Add this for std::istringstream

#include "Enemy/Enemy.hpp"
#include "Enemy/SoldierEnemy.hpp"
#include "Enemy/PlaneEnemy.hpp"
#include "Enemy/TankEnemy.hpp"
#include "Enemy/SupportEnemy.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/LOG.hpp"
#include "Engine/Resources.hpp"
#include "PlayScene.hpp"
#include "WinScene.hpp"
#include "Turret/LaserTurret.hpp"
#include "Turret/MachineGunTurret.hpp"
#include "Turret/PierceTurret.hpp"
#include "Turret/RocketTurret.hpp"
#include "Turret/TurretButton.hpp"
#include "Turret/Landmine.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "UI/Animation/Plane.hpp"
#include "UI/Component/Label.hpp"
#include <map>
#include "LocalAccount.h"

// TODO HACKATHON-4 (1/3): Trace how the game handles keyboard input. [DONE]
// TODO HACKATHON-4 (2/3): Find the cheat code sequence in this file [DONE line 40]
// TODO HACKATHON-4 (3/3): When the cheat code is entered, a plane should be spawned and added to the scene. [DONE]
// TODO HACKATHON-5 (1/4): There's a bug in this file, which crashes the game when you win. Try to find it. [DONE]
// TODO HACKATHON-5 (2/4): The "LIFE" label are not updated when you lose a life. Try to fix it. [DONE line 280]

bool PlayScene::paused = false;
const float PlayScene::ConstructionTime = 10.0f;  // or whatever value you want
static Engine::Label* turretInfoLabel = nullptr;
static bool turretInfoLabelInUI = false;
static int selectedTurretX = -1;
static int selectedTurretY = -1;
bool PlayScene::DebugMode = false;
const std::vector<Engine::Point> PlayScene::directions = { Engine::Point(-1, 0), Engine::Point(0, -1), Engine::Point(1, 0), Engine::Point(0, 1) };
const int PlayScene::MapWidth = 20, PlayScene::MapHeight = 13;
const int PlayScene::BlockSize = 64;
const float PlayScene::DangerTime = 7.61;
const Engine::Point PlayScene::SpawnGridPoint = Engine::Point(-1, 0);
const Engine::Point PlayScene::EndGridPoint = Engine::Point(MapWidth, MapHeight - 1);
const std::vector<int> PlayScene::code = { //CHEAT CODE SEQUENCE HERE
    ALLEGRO_KEY_UP, ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_DOWN,
    ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT,
    ALLEGRO_KEY_B, ALLEGRO_KEY_A, ALLEGRO_KEY_LSHIFT, ALLEGRO_KEY_ENTER
};

void PlayScene::FreeMapTile(int x, int y) {
    if (x >= 0 && x < MapWidth && y >= 0 && y < MapHeight) {
        mapState[y][x] = TILE_FLOOR;
    }
}
float PlayScene::CalculateDistance(const Engine::Point& p1, const Engine::Point& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt(dx*dx + dy*dy);
}
Engine::Point PlayScene::GetClientSize() {
    return Engine::Point(MapWidth * BlockSize, MapHeight * BlockSize);
}
void PlayScene::Initialize() {
    // Clear all turret info UI elements first
    ClearTurretInfo();
    if (turretInfoLabel) {
        UIGroup->RemoveObject(turretInfoLabel->GetObjectIterator());
        turretInfoLabel = nullptr;
    }
    for (auto& label : turretInfoLabels) {
        if (label) {
            UIGroup->RemoveObject(label->GetObjectIterator());
        }
    }
    turretInfoLabels.clear();
    if (upgradeButton) {
        upgradeButton->SetOnClickCallback(nullptr);
        UIGroup->RemoveObject(upgradeButton->GetObjectIterator());
        upgradeButton = nullptr;
    }
    mapState.clear();
    keyStrokes.clear();
    ticks = 0;
    deathCountDown = -1;
    lives = 10;
    money = 300;
    SpeedMult = 1;
    enemiesKilled = 0;
    gameStarted = false; // Reset gameStarted flag on initialization
    paused = false;

    // Construction
    currentPhase = GamePhase::CONSTRUCTION;
    currentWave = 0;
    waveTimer = 0;
    constructionTimer = ConstructionTime; // Set to 60 seconds
    postWaveDelayTimer = 0.0f; // Initialize post wave delay timer
    // Add groups from bottom to top.
    AddNewObject(TileMapGroup = new Group());
    AddNewObject(GroundEffectGroup = new Group());
    AddNewObject(DebugIndicatorGroup = new Group());
    AddNewObject(TowerGroup = new Group());
    AddNewObject(EnemyGroup = new Group());
    AddNewObject(BulletGroup = new Group());
    AddNewObject(EffectGroup = new Group());
    // Should support buttons.
    AddNewControlObject(UIGroup = new Group());
    ReadMap();
    ReadEnemyWave();
    
    // Clear any existing enemies
    EnemyGroup->Clear();

    // Only put the first round's enemies in the queue
    if (!allEnemyWaves.empty()) {
        enemyWaveData = allEnemyWaves[0];
    } else {
        enemyWaveData.clear();
    }

    mapDistance = CalculateBFSDistance();
    ConstructUI();
    imgTarget = new Engine::Image("play/target.png", 0, 0);
    imgShovel = new Engine::Image("play/shovel.png", 0, 0);
    imgWrench = new Engine::Image("play/wrench.png", 0, 0);
    imgTarget->Visible = false;
    imgShovel->Visible = false;
    imgWrench->Visible = false;
    preview = nullptr;
    UIGroup->AddNewObject(imgTarget);
    UIGroup->AddNewObject(imgShovel);
    UIGroup->AddNewObject(imgWrench); 
    // Preload Lose Scene
    deathBGMInstance = Engine::Resources::GetInstance().GetSampleInstance("astronomia.ogg");
    Engine::Resources::GetInstance().GetBitmap("lose/benjamin-happy.png");
    // Start BGM.
    bgmId = AudioHelper::PlayBGM("play.ogg");
}
void PlayScene::Terminate() {
    AudioHelper::StopBGM(bgmId);
    AudioHelper::StopSample(deathBGMInstance);
    deathBGMInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
    IScene::Terminate();
}

void PlayScene::Update(float deltaTime) {
    // If paused, skip updating game objects (enemies, bullets, etc.)
    if (paused) {
        // Still update UI and preview for responsiveness
        if (preview) {
            preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
            preview->Update(0);
        }
        GroundEffectGroup->Update(0);
        return;
    }

    // Construction
    // Handle game phases
    if (currentPhase == GamePhase::CONSTRUCTION) {
        constructionTimer -= deltaTime;
        if (constructionTimer < 0) constructionTimer = 0;
        // Update label
        int secondsLeft = static_cast<int>(ceil(constructionTimer));
        int minutes = secondsLeft / 60;
        int seconds = secondsLeft % 60;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Construction: %d:%02d", minutes, seconds);
        constructionTimerLabel->Text = buffer;

        // Update animations during construction phase
        EffectGroup->Update(deltaTime);
        GroundEffectGroup->Update(deltaTime);
        
        // Only go to win scene if all rounds are done AND this is the final construction phase (after all waves)
        if (constructionTimer <= 0) {
            if (currentWave >= 4) { // Ensure win only after 4 rounds
                Engine::GameEngine::GetInstance().ChangeScene("win");
                return;
            }
            // Only put the current round's enemies in the queue at the start of each wave
            enemyWaveData = allEnemyWaves[currentWave];
            currentPhase = GamePhase::WAVE;
            constructionTimerLabel->Text = "";
            ticks = 0; // Reset ticks to start enemy spawn timing fresh

            // Remove preview when entering WAVE phase
            if (preview) {
                UIGroup->RemoveObject(preview->GetObjectIterator());
                preview = nullptr;
                imgTarget->Visible = false;
                imgShovel->Visible = false;
                imgWrench->Visible = false;
            }
        }
    }
    else { // WAVE phase
        waveTimer += deltaTime;

        // Check if wave is complete (all enemies spawned and no enemies left)
        if (enemyWaveData.empty() && EnemyGroup->GetObjects().empty()) {
            // Check if there are any active effects (like explosions) still playing
            bool effectsPlaying = false;
            for (auto& obj : EffectGroup->GetObjects()) {
                if (dynamic_cast<DirtyEffect*>(obj)) {
                    effectsPlaying = true;
                    break;
                }
            }
            
            // Only proceed to next phase if no effects are playing
            if(!effectsPlaying) {
                // Clear any remaining enemies (just to be safe)
                for (auto& obj : EnemyGroup->GetObjects()) {
                    EnemyGroup->RemoveObject(obj->GetObjectIterator());
                }
                
                if (currentWave >= 3) {
                    // Update high score before switching scene
                    accountManager.updateHighScore(totalScore);

                    // Pass score and name to WinScene
                    auto* winScene = dynamic_cast<WinScene*>(Engine::GameEngine::GetInstance().GetScene("win"));
                    if (winScene) {
                        winScene->SetFinalScore(totalScore);
                        winScene->SetPlayerName(accountManager.getCurrentUsername());
                    }

                    // Change to win scene
                    Engine::GameEngine::GetInstance().ChangeScene("win");
                    return;
                }
                currentWave++;
                // Always go to construction phase, even after last wave
                currentPhase = GamePhase::CONSTRUCTION;
                constructionTimer = ConstructionTime;
                // Reset construction timer label text for new construction phase
                if (constructionTimerLabel) {
                    int secondsLeft = static_cast<int>(ceil(constructionTimer));
                    int minutes = secondsLeft / 60;
                    int seconds = secondsLeft % 60;
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "Construction: %d:%02d", minutes, seconds);
                    constructionTimerLabel->Text = buffer;
                }
                EnemyGroup->Clear();
            }
        }
    }

    // Update phase indicator UI
    if (currentPhase == GamePhase::CONSTRUCTION && constructionTimerLabel) {
        int secondsLeft = static_cast<int>(ceil(constructionTimer));
        int minutes = secondsLeft / 60;
        int seconds = secondsLeft % 60;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Construction Phase %d:%02d", minutes, seconds);
        constructionTimerLabel->Text = buffer;
    }
    else if (controlsLabel) {
        // Show 1-based round number for UI
        int displayWave = std::min(currentWave + 1, 4); // Show max 4 waves
        controlsLabel->Text = "WAVE " + std::to_string(displayWave) + "/4";
    }

    if (SpeedMult == 0)
        deathCountDown = -1;
    else if (deathCountDown != -1)
        SpeedMult = 1;
    // Calculate danger zone.
    std::vector<float> reachEndTimes;
    for (auto &it : EnemyGroup->GetObjects()) {
        reachEndTimes.push_back(dynamic_cast<Enemy *>(it)->reachEndTime);
    }
    
    if (currentPhase == GamePhase::WAVE) {
        // Can use Heap / Priority-Queue instead. But since we won't have too many enemies, sorting is fast enough.
        std::sort(reachEndTimes.begin(), reachEndTimes.end());
        float newDeathCountDown = -1;
        int danger = lives;
        for (auto &it : reachEndTimes) {
            if (it <= DangerTime) {
                danger--;
                if (danger <= 0) {
                    // Death Countdown
                    float pos = DangerTime - it;
                    if (it > deathCountDown) {
                        // Restart Death Count Down BGM.
                        AudioHelper::StopSample(deathBGMInstance);
                        if (SpeedMult != 0)
                            deathBGMInstance = AudioHelper::PlaySample("astronomia.ogg", false, AudioHelper::BGMVolume, pos);
                    }
                    float alpha = pos / DangerTime;
                    alpha = std::max(0, std::min(255, static_cast<int>(alpha * alpha * 255)));
                    dangerIndicator->Tint = al_map_rgba(255, 255, 255, alpha);
                    newDeathCountDown = it;
                    break;
                }
            }
        }
        deathCountDown = newDeathCountDown;
        if (SpeedMult == 0)
            AudioHelper::StopSample(deathBGMInstance);
        if (deathCountDown == -1 && lives > 0) {
            AudioHelper::StopSample(deathBGMInstance);
            dangerIndicator->Tint.a = 0;
        }
        if (SpeedMult == 0)
            deathCountDown = -1;
        for (int i = 0; i < SpeedMult; i++) {
            IScene::Update(deltaTime);
            // Check if we should create new enemy.
            ticks += deltaTime;
            if (enemyWaveData.empty()) {
                if (EnemyGroup->GetObjects().empty()) {
                    // Only go to win scene if all waves are done
                    if (currentWave >= 4) {
                        // Update high score before switching scene
                        accountManager.updateHighScore(totalScore);

                        // Pass score and name to WinScene
                        auto* winScene = dynamic_cast<WinScene*>(Engine::GameEngine::GetInstance().GetScene("win"));
                        if (winScene) {
                            winScene->SetFinalScore(totalScore);
                            winScene->SetPlayerName(accountManager.getCurrentUsername());
                        }

                        // Change to win scene
                        Engine::GameEngine::GetInstance().ChangeScene("win");
                        return;
                    }

                    // Wait for death animation to end
                    if (deathCountDown != -1) {
                        continue;
                    }

                    // Delay after wave ends before construction phase
                    if (postWaveDelayTimer > 0.0f) {
                        postWaveDelayTimer -= deltaTime;
                        continue;
                    }

                    // Move to construction phase
                    currentWave++;
                    currentPhase = GamePhase::CONSTRUCTION;
                    constructionTimer = ConstructionTime;
                    postWaveDelayTimer = 1.0f; // Reset for next round

                    // Update construction timer label
                    if (constructionTimerLabel) {
                        int secondsLeft = static_cast<int>(ceil(constructionTimer));
                        int minutes = secondsLeft / 60;
                        int seconds = secondsLeft % 60;
                        char buffer[32];
                        snprintf(buffer, sizeof(buffer), "Construction: %d:%02d", minutes, seconds);
                        constructionTimerLabel->Text = buffer;
                    }
                }
                continue;
            }
            auto current = enemyWaveData.front();
            if (ticks < current.second)
                continue;
            ticks -= current.second;
            enemyWaveData.pop_front();
            const Engine::Point SpawnCoordinate = Engine::Point(SpawnGridPoint.x * BlockSize + BlockSize / 2, SpawnGridPoint.y * BlockSize + BlockSize / 2);
            Enemy *enemy;
            switch (current.first) {
                case 1:
                    EnemyGroup->AddNewObject(enemy = new SoldierEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
                    break;
                // TODO HACKATHON-3 (2/3): Add your new enemy here. [DONE]
                case 2:
                    EnemyGroup->AddNewObject(enemy = new PlaneEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
                    break;
                case 3:
                    EnemyGroup->AddNewObject(enemy = new TankEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
                    break;
                case 4:
                    EnemyGroup->AddNewObject(enemy = new SupportEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
                    break;
                default:
                    continue;
            }

            for (auto& obj : EnemyGroup->GetObjects()) {
                Enemy* enemy = dynamic_cast<Enemy*>(obj);
                if (enemy && enemy->type == "Support") {
                    // This is a SupportEnemy, buff nearby allies
                    for (auto& obj2 : EnemyGroup->GetObjects()) {
                        Enemy* ally = dynamic_cast<Enemy*>(obj2);
                        if (ally && ally != enemy && ally->type != "Support") {
                            float distance = CalculateDistance(enemy->Position, ally->Position);
                            if (distance <= 200 && !ally->buffed) { // 200 is buff radius
                                ally->setHP(ally->getHP() * 2); // Double the health
                                ally->buffed = true;
                                // Add visual effect
                                EffectGroup->AddNewObject(new DirtyEffect("play/target.png", 1, 
                                    ally->Position.x, ally->Position.y));
                            }
                        }
                    }
                }
            }

            enemy->UpdatePath(mapDistance);
            // Compensate the time lost.
            enemy->Update(ticks);
        }
    }

    // Clean up destroyed turrets
    std::vector<Turret*> turretsToRemove;
    const auto& towerObjects = TowerGroup->GetObjects();
    for (auto it = towerObjects.begin(); it != towerObjects.end(); ++it) {
        Turret* turret = dynamic_cast<Turret*>(*it);
        if (turret && turret->IsDestroyed()) {
            turretsToRemove.push_back(turret);
        }
    }
    for (Turret* turret : turretsToRemove) {
        TowerGroup->RemoveObject(turret->GetObjectIterator());
    }
    
    if (preview) {
        preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
        // To keep responding when paused.
        preview->Update(deltaTime);
    }
    // Add update call for GroundEffectGroup to update DirtyEffect objects
    GroundEffectGroup->Update(deltaTime);
}
void PlayScene::Draw() const {
    IScene::Draw();
    if (TileMapGroup) {
        TileMapGroup->Draw();
    }
    if (GroundEffectGroup) {
        GroundEffectGroup->Draw();
    }
    if (DebugIndicatorGroup) {
        DebugIndicatorGroup->Draw();
    }
    if (TowerGroup) {
        TowerGroup->Draw();
    }
    if (EnemyGroup) {
        EnemyGroup->Draw();
    }
    if (BulletGroup) {
        BulletGroup->Draw();
    }
    if (EffectGroup) {
        EffectGroup->Draw();
    }
    if (UIGroup) {
        UIGroup->Draw();
    }

    if (turretInfoLabel && turretInfoLabelInUI) {
        int boxX = 1300;
        int boxY = 310;
        int boxW = 280;
        int boxH = 200; // Increased height to fit Effectiveness and Weakness

        al_draw_filled_rounded_rectangle(boxX, boxY, boxX + boxW, boxY + boxH,
                                        10, 10, al_map_rgba(0, 0, 0, 80)); // semi-transparent black
        al_draw_rounded_rectangle(boxX, boxY, boxX + boxW, boxY + boxH,
                                10, 10, al_map_rgb(255, 255, 255), 2.0); // white outline
    }
    if (paused) {
        // Draw semi-transparent black overlay
        al_draw_filled_rectangle(0, 0, 
            MapWidth * BlockSize, 
            MapHeight * BlockSize, 
            al_map_rgba(0, 0, 0, 150));
        
        // Draw pause text
        Engine::Label pauseText("PAUSED", "pirulen.ttf", 96, 
            MapWidth * BlockSize / 2, 
            MapHeight * BlockSize / 2);
        pauseText.Anchor = Engine::Point(0.5, 0.5);
        pauseText.Draw();
    }

    if (hoveredEnemy) {
        // Popup configuration
        float popupWidth = 100.0f;
        float popupHeight = 50.0f;
        float barWidth = 80.0f;
        float barHeight = 10.0f;
        float radius = 5.0f;

        // Popup positioning (below enemy)
        float centerX = hoveredEnemy->Position.x;
        float topY = hoveredEnemy->Position.y + 20;
        float left = centerX - popupWidth / 2;
        float right = centerX + popupWidth / 2;
        float bottom = topY + popupHeight;

        // Draw rounded background
        al_draw_filled_rounded_rectangle(left, topY, right, bottom, radius, radius, al_map_rgba(211, 169, 108, 180));

        // Draw enemy type label
        Engine::Label typeLabel(hoveredEnemy->type, "pirulen.ttf", 12, centerX, topY + 10);
        typeLabel.Anchor = Engine::Point(0.5, 0.5);
        typeLabel.Draw();

        // Health bar logic
        float hp = hoveredEnemy->getHP();
        float baseMaxHP = hoveredEnemy->getMaxHP();
        float maxHP = std::max(hp, baseMaxHP); // To handle overflow
        float normalPercent = std::min(hp, baseMaxHP) / maxHP;

        float barLeft = centerX - barWidth / 2;
        float barTop = topY + 25;

        // Bar background
        al_draw_filled_rectangle(
            barLeft, barTop,
            barLeft + barWidth, barTop + barHeight,
            al_map_rgb(100, 100, 100)
        );

        // Normal HP portion
        ALLEGRO_COLOR normalColor = normalPercent > 0.6 ? al_map_rgb(0, 200, 0) :
                                    normalPercent > 0.3 ? al_map_rgb(255, 165, 0) :
                                                        al_map_rgb(200, 0, 0);

        al_draw_filled_rectangle(
            barLeft, barTop,
            barLeft + barWidth * normalPercent, barTop + barHeight,
            normalColor
        );

        // Overflow (bonus HP)
        if (hp > baseMaxHP) {
            float overflowPercent = (hp - baseMaxHP) / maxHP;
            al_draw_filled_rectangle(
                barLeft + barWidth * normalPercent, barTop,
                barLeft + barWidth * (normalPercent + overflowPercent), barTop + barHeight,
                al_map_rgb(0, 150, 255)
            );
        }

        // Border
        al_draw_rectangle(
            barLeft, barTop,
            barLeft + barWidth, barTop + barHeight,
            al_map_rgb(255, 255, 255), 1
        );
    }
    if (DebugMode) {
        // Draw reverse BFS distance on all reachable blocks.
        for (int i = 0; i < MapHeight; i++) {
            for (int j = 0; j < MapWidth; j++) {
                if (mapDistance[i][j] != -1) {
                    // Not elegant nor efficient, but it's quite enough for debugging.
                    Engine::Label label(std::to_string(mapDistance[i][j]), "pirulen.ttf", 32, (j + 0.5) * BlockSize, (i + 0.5) * BlockSize);
                    label.Anchor = Engine::Point(0.5, 0.5);
                    label.Draw();
                }
            }
        }
    }

    if (wrenchMode) {
        int infoX = Engine::GameEngine::GetInstance().GetScreenSize().x - 200;
        int infoY = 50;
        
        al_draw_filled_rounded_rectangle(infoX, infoY, infoX + 180, infoY + 40, 
                                        5, 5, al_map_rgba(0, 0, 0, 180));
        al_draw_rounded_rectangle(infoX, infoY, infoX + 180, infoY + 40,
                                5, 5, al_map_rgb(255, 255, 255), 2);
        
        Engine::Label upgradeInfo("Upgrade: +" + std::to_string((int)wrenchUpgradeAmount) + "s", 
                                "pirulen.ttf", 16, infoX + 10, infoY + 10);
        upgradeInfo.Draw();
    }
}
void PlayScene::OnMouseDown(int button, int mx, int my) {
    if (paused) return;
    // Right click on turret button: show info
    if ((button & 2)) {
        // First, remove any existing turret info labels
        ClearTurretInfo();
        
        for (auto& it : TowerGroup->GetObjects()) {
            Turret* turret = dynamic_cast<Turret*>(it);
            if (turret) {
                int turretX = static_cast<int>(turret->Position.x);
                int turretY = static_cast<int>(turret->Position.y);
                if (mx >= turretX - BlockSize/2 && mx < turretX + BlockSize/2 &&
                    my >= turretY - BlockSize/2 && my < turretY + BlockSize/2) {
                    // Show turret info with actual turret stats
                    TurretBtnInfo btnInfo;
                    btnInfo.name = turret->GetName();
                    btnInfo.atk = turret->GetDamage();
                    btnInfo.hp = turret->GetHealth();
                    ShowTurretInfo(btnInfo, mx, my, turret);
                    return;
                }
            }
        }

        // Check if mouse is over a turret button
        std::vector<TurretBtnInfo> btns = {
            {1294, 136, 64, 64, "Machine Gun", 15, 800, ""},
            {1370, 136, 64, 64, "Laser Turret", 20, 700, ""},
            {1446, 136, 64, 64, "Pierce Turret", 25, 600, ""},
            {1522, 136, 64, 64, "Rocket Turret", 30, 500, ""},
            {1294, 215, 64, 64, "Shovel", 0, 0, ""}, // swapped with landmine
            // {1294, 215, 64, 64, "Landmine", 40, 1, ""}, // swapped with shovel
            {1370, 215, 64, 64, "Wrench", 0, 0, ""}  // Add this line for the wrench
        };
        for (const auto& btn : btns) {
            if (mx >= btn.x && mx < btn.x + btn.w && my >= btn.y && my < btn.y + btn.h) {
                ClearTurretInfo();
                ShowTurretInfo(btn, mx, my);
                return; // Exit after finding the clicked button
            }
        }
        IScene::OnMouseDown(button, mx, my);
    }
    else if ((button & 1)) {
        IScene::OnMouseDown(button, mx, my);
    }
}
void PlayScene::ClearTurretInfo() {
    if (turretInfoLabel) {
        UIGroup->RemoveObject(turretInfoLabel->GetObjectIterator());
        turretInfoLabel = nullptr;
    }
    // Clear any additional labels if they exist
    for (auto& label : turretInfoLabels) {
        if (label) {
            UIGroup->RemoveObject(label->GetObjectIterator());
        }
    }
    turretInfoLabels.clear();
     // Clear the upgrade button if it exists
    if (upgradeButton) {
        upgradeButton->SetOnClickCallback(nullptr); // Clear callback to prevent dangling references
        UIGroup->RemoveObject(upgradeButton->GetObjectIterator());
        upgradeButton = nullptr;
    }
    turretInfoLabelInUI = false;
}
void PlayScene::ShowTurretInfo(const TurretBtnInfo& btn, int mx, int my, Turret* actualTurret) {
    ClearTurretInfo(); // Clear any existing info first
    // Position the info box
    int boxX = 1310;
    int boxY = 320; // lowered by 10 pixels to match turret button row shift
    int boxW = 280;
    int boxH = 200; // Increased height to fit additional info

    // Move stats box position for shovel and landmine to right side fixed position
    if (btn.name == "Shovel" || btn.name == "Landmine") {
        int screenWidth = Engine::GameEngine::GetInstance().GetScreenSize().x;
        boxX = screenWidth - boxW - 10; // 20 px padding from right edge
        boxY = 320;
    }
    
    // Adjust position if it would go off-screen
    int screenWidth = Engine::GameEngine::GetInstance().GetScreenSize().x;
    if (boxX + boxW > screenWidth) {
        boxX = mx - boxW - 20;
    }

    int localSelectedTurretX = -1;
    int localSelectedTurretY = -1;

    if (actualTurret) {
        localSelectedTurretX = static_cast<int>(actualTurret->Position.x) / BlockSize;
        localSelectedTurretY = static_cast<int>(actualTurret->Position.y) / BlockSize;
    } else {
        // Store the turret grid position for upgrade button callback
        localSelectedTurretX = mx / BlockSize;
        if (localSelectedTurretX >= MapWidth) localSelectedTurretX = MapWidth - 1;
        localSelectedTurretY = my / BlockSize;
        if (localSelectedTurretY >= MapHeight) localSelectedTurretY = MapHeight - 1;
    }

    if (turretInfoLabelInUI && turretInfoLabel && !turretInfoLabels.empty()) {
        // Update existing labels' text
        turretInfoLabel->Text = "";
        turretInfoLabels[0]->Text = btn.name;
        if (btn.name != "Shovel" && btn.name != "Wrench") {
            if (actualTurret) {
                turretInfoLabels[1]->Text = "Attack: " + std::to_string(static_cast<int>(actualTurret->GetDamage()));
                turretInfoLabels[2]->Text = "Health: " + std::to_string(static_cast<int>(actualTurret->GetHealth()));
            } else {
                turretInfoLabels[1]->Text = "Attack: " + std::to_string(static_cast<int>(btn.atk));
                turretInfoLabels[2]->Text = "Health: " + std::to_string(static_cast<int>(btn.hp));
            }
            int cost = 0;
            if (btn.name == "Machine Gun Turret") cost = MachineGunTurret::Price;
            else if (btn.name == "Laser Turret") cost = LaserTurret::Price;
            else if (btn.name == "Pierce Turret") cost = PierceTurret::Price;
            else if (btn.name == "Rocket Turret") cost = RocketTurret::Price;
            else if (btn.name == "Landmine") cost = Landmine::Price;
            turretInfoLabels[3]->Text = "Upgrade cost: $" + std::to_string(cost);

            // Update new labels for Effectiveness and Weakness with actual values
            if (btn.name == "Rocket Turret") {
                turretInfoLabels[4]->Text = "Effectiveness: Plane, Tank";
            } else if (btn.name == "Laser Turret") {
                turretInfoLabels[4]->Text = "Effectiveness: Support";
            } else if (btn.name == "Machine Gun Turret") {
                turretInfoLabels[4]->Text = "Effectiveness: Soldier";
            } else if (btn.name == "Pierce Turret") {
                turretInfoLabels[4]->Text = "Effectiveness: Plane";
            } else if (btn.name == "Landmine") {
                turretInfoLabels[4]->Text = "Effectiveness: Soldier";
            } else {
                turretInfoLabels[4]->Text = "Effectiveness: ";
            }
            // Update Weakness label with actual values
            if (btn.name == "Laser Turret") {
                turretInfoLabels[5]->Text = "Weakness: Plane";
            } else if (btn.name == "Machine Gun Turret") {
                turretInfoLabels[5]->Text = "Weakness: Tank";
            } else if (btn.name == "Pierce Turret") {
                turretInfoLabels[5]->Text = "Weakness: Support";
            } else if (btn.name == "Landmine") {
                turretInfoLabels[5]->Text = "Weakness: Plane";
            } else {
                turretInfoLabels[5]->Text = "Weakness: ";
            }
        }
    } else {
        ClearTurretInfo(); // Clear any existing info first
        
        // Create and position the info box background
        UIGroup->AddNewObject(turretInfoLabel = new Engine::Label("", "pirulen.ttf", 16, boxX + 10, boxY + 10));
        turretInfoLabelInUI = true;
        
        // Add name label
        Engine::Label* nameLabel = new Engine::Label(btn.name, "pirulen.ttf", 18, boxX + 10, boxY + 10, 0, 0, 0);
        UIGroup->AddNewObject(nameLabel);
        turretInfoLabels.push_back(nameLabel);

        if (btn.name == "Wrench") {
            nameLabel->Text = "Potion"; // Change display name here
        }

        // Add stats labels only for turrets (not shovel)
        if (btn.name != "Shovel" && btn.name != "Wrench") {
            if (actualTurret) {
                Engine::Label* atkLabel = new Engine::Label("Attack: " + std::to_string(actualTurret->GetDamage()), "pirulen.ttf", 14, boxX + 10, boxY + 40, 0, 0, 0);
                UIGroup->AddNewObject(atkLabel);
                turretInfoLabels.push_back(atkLabel);
                
                Engine::Label* hpLabel = new Engine::Label("Health: " + std::to_string(actualTurret->GetHealth()), "pirulen.ttf", 14, boxX + 10, boxY + 70, 0, 0, 0);
                UIGroup->AddNewObject(hpLabel);
                turretInfoLabels.push_back(hpLabel);
            } else {
                Engine::Label* atkLabel = new Engine::Label("Attack: " + std::to_string(btn.atk), "pirulen.ttf", 14, boxX + 10, boxY + 40, 0, 0, 0);
                UIGroup->AddNewObject(atkLabel);
                turretInfoLabels.push_back(atkLabel);
                
                Engine::Label* hpLabel = new Engine::Label("Health: " + std::to_string(btn.hp), "pirulen.ttf", 14, boxX + 10, boxY + 70, 0, 0, 0);
                UIGroup->AddNewObject(hpLabel);
                turretInfoLabels.push_back(hpLabel);
            }

            // Add new labels for Effectiveness and Weakness with actual values
            Engine::Label* effectivenessLabel;
            if (btn.name == "Rocket Turret") {
                effectivenessLabel = new Engine::Label("Effectiveness: Tank", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
                nameLabel->Text = "Wriothesley"; 
            } else if (btn.name == "Laser Turret") {
                effectivenessLabel = new Engine::Label("Effectiveness: Support", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
                nameLabel->Text = "Mage"; 
            } else if (btn.name == "Machine Gun") {
                effectivenessLabel = new Engine::Label("Effectiveness: Soldier", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
                nameLabel->Text = "Assassin"; 
            } else if (btn.name == "Pierce Turret") {
                effectivenessLabel = new Engine::Label("Effectiveness: Plane", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
                nameLabel->Text = "Archer"; 
            } else if (btn.name == "Landmine") {
                effectivenessLabel = new Engine::Label("Effectiveness: Soldier", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
            } else {
                effectivenessLabel = new Engine::Label("Effectiveness: ", "pirulen.ttf", 14, boxX + 10, boxY + 100, 0, 0, 0);
            }
            UIGroup->AddNewObject(effectivenessLabel);
            turretInfoLabels.push_back(effectivenessLabel);

            Engine::Label* weaknessLabel;
            if (btn.name == "Laser Turret") {
                weaknessLabel = new Engine::Label("Weakness: Plane", "pirulen.ttf", 14, boxX + 10, boxY + 130, 0, 0, 0);
            } else if (btn.name == "Machine Gun") {
                weaknessLabel = new Engine::Label("Weakness: Tank", "pirulen.ttf", 14, boxX + 10, boxY + 130, 0, 0, 0);
            } else if (btn.name == "Pierce Turret") {
                weaknessLabel = new Engine::Label("Weakness: Support", "pirulen.ttf", 14, boxX + 10, boxY + 130, 0, 0, 0);
            } else if (btn.name == "Landmine") {
                weaknessLabel = new Engine::Label("Weakness: Plane", "pirulen.ttf", 14, boxX + 10, boxY + 130, 0, 0, 0);
            } else {
                weaknessLabel = new Engine::Label("Weakness: ", "pirulen.ttf", 14, boxX + 10, boxY + 130, 0, 0, 0);
            }
            UIGroup->AddNewObject(weaknessLabel);
            turretInfoLabels.push_back(weaknessLabel);
        }
        
        // Add cost label for turrets and landmine
        if (btn.name != "Shovel" && btn.name != "Wrench") {
            int cost = 100;
            if (btn.name == "Machine Gun Turret") cost = 100;
            else if (btn.name == "Laser Turret") cost = 120;
            else if (btn.name == "Pierce Turret") cost = 150;
            else if (btn.name == "Rocket Turret") cost = 200;
            else if (btn.name == "Landmine") cost = Landmine::Price;
            
            Engine::Label* costLabel = new Engine::Label("Cost: $" + std::to_string(cost), "pirulen.ttf", 14, boxX + 10, boxY + 160, 0, 0, 0);
            UIGroup->AddNewObject(costLabel);
            turretInfoLabels.push_back(costLabel);
        }
    }
}
void PlayScene::OnMouseMove(int mx, int my) {
    IScene::OnMouseMove(mx, my);
    const int x = mx / BlockSize;
    const int y = my / BlockSize;

    hoveredEnemy = nullptr;

    for (auto& obj : EnemyGroup->GetObjects()) {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy) {
            float dx = mx - enemy->Position.x;
            float dy = my - enemy->Position.y;
            if (dx*dx + dy*dy < enemy->CollisionRadius * enemy->CollisionRadius) {
                hoveredEnemy = enemy;
                break;
            }
        }
    }
    
    // Don't hide/show target/shovel cursors when showing turret info
    if (!turretInfoLabelInUI) {
        if (!preview && !shovelMode  && !wrenchMode|| x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
            imgTarget->Visible = false;
            imgShovel->Visible = false;
            imgWrench->Visible = false;
            return;
        }

        imgTarget->Visible = false;
        imgShovel->Visible = false;
        imgWrench->Visible = false;
        
        if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
            return;
        }

        if (shovelMode) {
            imgShovel->Visible = true;
        } 
        else if (wrenchMode) {
            imgWrench->Visible = true;
        }
        else if (preview) {
            imgTarget->Visible = true;
        }
        
        if (shovelMode) {
            imgShovel->Position.x = x * BlockSize;
            imgShovel->Position.y = y * BlockSize;
        }
        else if (wrenchMode) {
            imgWrench->Position.x = x * BlockSize;
            imgWrench->Position.y = y * BlockSize;
        }
        else if (preview) {
            imgTarget->Position.x = x * BlockSize;
            imgTarget->Position.y = y * BlockSize;
        }
    }
}
void PlayScene::OnMouseUp(int button, int mx, int my) {
    if (paused) return;
    // Remove construction phase check for wrench
    // if(currentPhase != GamePhase::CONSTRUCTION) return;
    IScene::OnMouseUp(button, mx, my);
    if (!imgTarget->Visible && !imgShovel->Visible && !imgWrench->Visible)
        return;
    const int x = mx / BlockSize;
    const int y = my / BlockSize;
    if (button & 1) {
        if (shovelMode) {
            // Shovel mode - remove turret if one exists at this position
            //iterates through the all the turrets
            for (auto& it : TowerGroup->GetObjects()) {
                Turret* turret = dynamic_cast<Turret*>(it);
                if (turret) {
                    int turretX = static_cast<int>(turret->Position.x) / BlockSize;
                    int turretY = static_cast<int>(turret->Position.y) / BlockSize;
                    if (turretX == x && turretY == y) {
                        // Remove the turret
                        TowerGroup->RemoveObject(it->GetObjectIterator());
                        mapState[y][x] = TILE_FLOOR; // Mark the tile as unoccupied
                    }
                }
            }
            shovelMode = false; // Exit shovel mode if clicked on empty space
        }
        else if (wrenchMode) {
            for (auto& it : TowerGroup->GetObjects()) {
                Turret* turret = dynamic_cast<Turret*>(it);
                if (turret) {
                    int turretX = static_cast<int>(turret->Position.x) / BlockSize;
                    int turretY = static_cast<int>(turret->Position.y) / BlockSize;
                    if (turretX == x && turretY == y) {
                        // Don't allow wrench on destroyed turrets
                        if (turret->IsDestroyed())
                            break;
                        // Check if player has enough money
                        if (money < 100) {
                            break; // Not enough money, do nothing
                        }
                        // If turret is disabled, enable it and upgrade
                        if (!turret->Enabled) {
                            turret->Enabled = true;
                        } 
                        
                        turret->IncreaseLifetime(wrenchUpgradeAmount);

                        // Visual feedback
                        GroundEffectGroup->AddNewObject(
                            new DirtyEffect("play/target.png", 1, 
                            x * BlockSize + BlockSize / 2, 
                            y * BlockSize + BlockSize / 2)
                        );

                        EarnMoney(-100);
                        break;
                    }
                }
            }
            wrenchMode = false; // Exit wrench mode after upgrade
        }
        else if (mapState[y][x] != TILE_OCCUPIED || dynamic_cast<Landmine*>(preview)) {
            // Original turret placement code
            if (!preview)
                return;
            if (!CheckSpaceValid(x, y)) {
                Engine::Sprite* sprite;
                GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
                sprite->Rotation = 0;
                // Remove preview turret and reset preview pointer after invalid placement
                UIGroup->RemoveObject(preview->GetObjectIterator());
                preview = nullptr;
                // Reset target and shovel cursor visibility after invalid placement
                if (imgTarget) imgTarget->Visible = false;
                if (imgShovel) imgShovel->Visible = false;
                return;
            }
            EarnMoney(-preview->GetPrice());
            preview->GetObjectIterator()->first = false;
            UIGroup->RemoveObject(preview->GetObjectIterator());
            preview->Position.x = x * BlockSize + BlockSize / 2;
            preview->Position.y = y * BlockSize + BlockSize / 2;
            preview->Enabled = true;
            preview->Preview = false;
            preview->Tint = al_map_rgba(255, 255, 255, 255);
            TowerGroup->AddNewObject(preview);
            preview->Update(0);
            preview = nullptr;
            mapState[y][x] = TILE_OCCUPIED;
            OnMouseMove(mx, my);
        }
    }
}
void PlayScene::OnKeyDown(int keyCode) {
    IScene::OnKeyDown(keyCode);
    if (keyCode == ALLEGRO_KEY_TAB) {
        DebugMode = !DebugMode;
    } 
    else if (keyCode == ALLEGRO_KEY_P) {
        // Toggle pause
        paused = !paused;
        if (paused) {
            AudioHelper::StopBGM(bgmId); // Pause by stopping
        } else {
            bgmId = AudioHelper::PlayBGM("play.ogg"); // Resume by restarting
        }
    }
    else if (keyCode == ALLEGRO_KEY_O) {
        // Restart stage
        Engine::GameEngine::GetInstance().ChangeScene("play");
        paused = false;
    }
    else if (keyCode == ALLEGRO_KEY_I) {
        // Back to stage select
        Engine::GameEngine::GetInstance().ChangeScene("stage-select");
        paused = false;
    }
    else if (keyCode == ALLEGRO_KEY_S && !gameStarted) {
        // Start the game when S is pressed
        gameStarted = true;
        currentPhase = GamePhase::CONSTRUCTION;

        // Spawn an enemy immediately after pressing 's'
        const Engine::Point SpawnCoordinate = Engine::Point(SpawnGridPoint.x * BlockSize + BlockSize / 2, SpawnGridPoint.y * BlockSize + BlockSize / 2);
        Enemy* enemy = new SoldierEnemy(SpawnCoordinate.x, SpawnCoordinate.y);
        EnemyGroup->AddNewObject(enemy);
        enemy->UpdatePath(mapDistance);
    }
    else if (keyCode == ALLEGRO_KEY_S && currentPhase == GamePhase::CONSTRUCTION) {
        // End construction phase early if 's' is pressed during construction
        constructionTimer = 0;
    }
    else {
        keyStrokes.push_back(keyCode);
        if (keyStrokes.size() > code.size())
            keyStrokes.pop_front();

        if (keyCode == ALLEGRO_KEY_ENTER && keyStrokes.size() == code.size()) { // Last code 
            auto it = keyStrokes.begin();
            for (int c : code) { 
                // Fix shift key comparison: code uses ALLEGRO_KEY_LSHIFT, but keyStrokes may have ALLEGRO_KEY_LSHIFT or ALLEGRO_KEY_RSHIFT
                if (!((*it == c) || 
                    (c == ALLEGRO_KEY_LSHIFT && (*it == ALLEGRO_KEY_LSHIFT || *it == ALLEGRO_KEY_RSHIFT))))
                    return;
                ++it;
            }
            EffectGroup->AddNewObject(new Plane());
            EarnMoney(10000);
        }
    }
    if (keyCode == ALLEGRO_KEY_Q) {
        // Hotkey for MachineGunTurret.
        UIBtnClicked(0);
    } else if (keyCode == ALLEGRO_KEY_W) {
        // Hotkey for LaserTurret.
        UIBtnClicked(1);
    }
    else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
        // Hotkey for Speed up.
        SpeedMult = keyCode - ALLEGRO_KEY_0;
    }
}
void PlayScene::Hit() {
    lives--;
    UILives->Text = std::string("Life ") + std::to_string(lives);
    if (lives <= 0) {
        Engine::GameEngine::GetInstance().ChangeScene("lose");
    }
}
int PlayScene::GetMoney() const {
    return money;
}
void PlayScene::EarnMoney(int money) {
    this->money += money;
    UIMoney->Text = std::string("$") + std::to_string(this->money);
}
void PlayScene::ReadMap() {
    std::string filename = std::string("Resource/map") + std::to_string(MapId) + ".txt";
    // Read map file.
    char c;
    std::vector<bool> mapData;
    std::ifstream fin(filename);
    while (fin >> c) {
        switch (c) {
            case '0': mapData.push_back(false); break;
            case '1': mapData.push_back(true); break;
            case '\n':
            case '\r':
                if (static_cast<int>(mapData.size()) / MapWidth != 0)
                    throw std::ios_base::failure("Map data is corrupted.");
                break;
            default: throw std::ios_base::failure("Map data is corrupted.");
        }
    }
    fin.close();
    // Validate map data.
    if (static_cast<int>(mapData.size()) != MapWidth * MapHeight)
        throw std::ios_base::failure("Map data is corrupted.");
    // Store map in 2d array.
    mapState = std::vector<std::vector<TileType>>(MapHeight, std::vector<TileType>(MapWidth));
    for (int i = 0; i < MapHeight; i++) {
        for (int j = 0; j < MapWidth; j++) {
            const int num = mapData[i * MapWidth + j];
            mapState[i][j] = num ? TILE_FLOOR : TILE_DIRT;
            if (num)
                TileMapGroup->AddNewObject(new Engine::Image("play/floor.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
            else
                TileMapGroup->AddNewObject(new Engine::Image("play/dirt.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
        }
    }
}
void PlayScene::ReadEnemyWave() {
    allEnemyWaves.clear();
    enemyWaveData.clear();
    // Read 4 files: enemy1-1.txt, enemy1-2.txt, enemy1-3.txt, enemy1-4.txt for MapId == 1
    for (int round = 1; round <= 4; ++round) {
        std::string filename = "Resource/enemy" + std::to_string(MapId) + "-" + std::to_string(round) + ".txt";
        std::ifstream fin(filename);
        std::string line;
        std::deque<std::pair<int, float>> currentWave;
        while (std::getline(fin, line)) {
            // Remove leading/trailing whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty()) continue;
            std::istringstream iss(line);
            int type;
            float wait;
            int repeat;
            if (iss >> type >> wait >> repeat) {
                for (int i = 0; i < repeat; i++)
                    currentWave.emplace_back(type, wait);
            }
        }
        fin.close();
        allEnemyWaves.push_back(currentWave);
    }
    
    enemyWaveData.clear();

    // // Only put the first round's enemies in the queue
    // if (!allEnemyWaves.empty()) {
    //     enemyWaveData = allEnemyWaves[0];
    // } else {
    //     enemyWaveData.clear();
    // }
}

void PlayScene::OnEnemyDefeated(Enemy *enemy){
    if(enemy->type == "Soldier"){
        AddScore(50);
    }
    
    if(enemy->type == "Plane"){
        AddScore(100);
    }

    if(enemy->type == "Tank"){
        AddScore(150);
    }

    if(enemy->type == "Support"){
        AddScore(200);
    }
}

void PlayScene::AddScore(int points){
    totalScore += points;
}

void PlayScene::ConstructUI() {
    // Construction Timer Label (top right)
    int screenWidth = Engine::GameEngine::GetInstance().GetScreenSize().x;
    constructionTimerLabel = new Engine::Label(
        "Construction: 60", // Initial text
        "pirulen.ttf", 32,
        10, 10, // Top right corner, adjust as needed
        255, 255, 0, 255, 0
    );
    UIGroup->AddNewObject(constructionTimerLabel);
    
    // Background
    UIGroup->AddNewObject(new Engine::Image("play/sand.png", 1280, 0, 320, 832));
    // Text
    UIGroup->AddNewObject(new Engine::Label(std::string("Stage ") + std::to_string(MapId), "pirulen.ttf", 32, 1294, 0));
    UIGroup->AddNewObject(UIMoney = new Engine::Label(std::string("$") + std::to_string(money), "pirulen.ttf", 24, 1294, 48));
    UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life ") + std::to_string(lives), "pirulen.ttf", 24, 1294, 88));
    TurretButton *btn;
    // Button 1
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/tower-base.png", 1294, 161, 0, 0, 0, 0),
        Engine::Sprite("play/turret-1.png", 1294, 161 - 8, 0, 0, 0, 0), 1294, 161, MachineGunTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 0));
    UIGroup->AddNewControlObject(btn);

    // Button 2
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/tower-base.png", 1370, 161, 0, 0, 0, 0),
        Engine::Sprite("play/turret-2.png", 1370, 161 - 8, 0, 0, 0, 0), 1370, 161, LaserTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 1));
    UIGroup->AddNewControlObject(btn);

    // Button 3
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/tower-base.png", 1446, 161, 0, 0, 0, 0),
        Engine::Sprite("play/turret-3.png", 1446, 161 - 8, 0, 0, 0, 0), 1446, 161, PierceTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 2));
    UIGroup->AddNewControlObject(btn);

    // Button 4
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/tower-base.png", 1522, 161, 0, 0, 0, 0),
        Engine::Sprite("play/turret-6.1.png", 1522, 161 - 8, 0, 0, 0, 0), 1522, 161, RocketTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 3));
    UIGroup->AddNewControlObject(btn);

    // Button 5 (shovel) - swapped position with landmine
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/shovel-base.png", 1294, 235, 0, 0, 0, 0),
        Engine::Sprite("play/shovel.png", 1294, 235, 0, 0, 0, 0), 1294, 235, 0);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 4));
    UIGroup->AddNewControlObject(btn);

    // Button 6 (wrench)
    btn = new TurretButton("play/floor.png", "play/dirt.png",
        Engine::Sprite("play/shovel-base.png", 1370, 235, 0, 0, 0, 0),
        Engine::Sprite("play/wrench.png", 1370, 235, 0, 0, 0, 0), 1370, 235, 0);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 5));
    UIGroup->AddNewControlObject(btn);

    CreateEnemyListUI();

    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;

    controlsLabel = new Engine::Label("P", "pirulen.ttf", 16, w - 300, h - 90, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);
    controlsLabel = new Engine::Label("Pause/continue", "pirulen.ttf", 16, w - 260, h - 90, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);

    controlsLabel = new Engine::Label("O", "pirulen.ttf", 16, w - 300, h - 60, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);
    controlsLabel = new Engine::Label("Restart", "pirulen.ttf", 16, w - 260, h - 60, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);

    controlsLabel = new Engine::Label("I", "pirulen.ttf", 16, w - 300, h - 30, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);
    controlsLabel = new Engine::Label("Return", "pirulen.ttf", 16, w - 260, h - 30, 0, 0, 0);
    UIGroup->AddNewObject(controlsLabel);

    int shift = 135 + 25;
    dangerIndicator = new Engine::Sprite("play/benjamin.png", w - shift, h - shift);
    dangerIndicator->Tint.a = 0;
    UIGroup->AddNewObject(dangerIndicator);

    // Construction phase
    UIGroup->AddNewObject(controlsLabel = new Engine::Label("", "pirulen.ttf", 24, 1294, 128));
}

// In PlayScene.cpp when player wins:
void PlayScene::SaveScore(int score, const std::string& playerName) {
    // Get current time
    time_t now = time(0); // asks for the time, asks the seconds since Jan 1, 1970 and stores in now
    struct tm tstruct; //creates a container struct called tstruct
    char buf[80]; //creates a character array who's length is 80 character
    tstruct = *localtime(&now); // takes the giant number stored in now and put it in the struct
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct); //strftime means stringformattime

    // Append the new score to the file
    std::ofstream file("scores.json", std::ios::app);
    if (file.is_open()) {
        file << playerName << ", " << score << ", " << MapId << ", " << buf << "\n";
        file.close();
    }
}

const std::vector<PlayScene::TurretBtnInfo> PlayScene::turretBtnInfos = {
    {1294, 136, 64, 64, "Machine Gun", 20, 100, ""},
    {1370, 136, 64, 64, "Laser", 40, 80, ""},
    {1446, 136, 64, 64, "Rocket", 60, 120, ""},
    {1522, 136, 64, 64, "Pierce", 30, 90, ""},
    {1294, 215, 64, 64, "Shovel", 0, 0, ""},
    {1446, 215, 64, 64, "Wrench", 0, 0, ""}
};

void PlayScene::UIBtnClicked(int id) {
    // Only restrict turret placement and shovel to construction phase,
    // but allow wrench to be used in any phase.
    if (id != 6 && currentPhase != GamePhase::CONSTRUCTION) return;
    
    Engine::Point mousePos = Engine::GameEngine::GetInstance().GetMousePosition();
    
    if (preview) {
        ClearTurretInfo();
        UIGroup->RemoveObject(preview->GetObjectIterator());
        preview = nullptr;
    }
    if (id == 0 && money >= MachineGunTurret::Price){
        preview = new MachineGunTurret(0, 0);
        shovelMode = false; // Exit shovel mode if a turret is selected 
    }else if (id == 1 && money >= LaserTurret::Price){
        preview = new LaserTurret(0, 0);
        shovelMode = false; // Exit shovel mode if a turret is selected
    }else if (id == 2 && money >= PierceTurret::Price){
        preview = new PierceTurret(0, 0);
        shovelMode = false; // Exit shovel mode if a turret is selected
    }else if (id == 3 && money >= RocketTurret::Price){
        preview = new RocketTurret(0, 0);
        shovelMode = false; // Exit shovel mode if a turret is selected
    }
    else if (id == 4) { // swapped with landmine
        preview = nullptr;
        shovelMode = true;
        // Update shovel cursor visibility and position immediately after selecting shovel
        OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
        if (imgShovel) {
            imgShovel->Visible = true;
            imgShovel->Position.x = (Engine::GameEngine::GetInstance().GetMousePosition().x / BlockSize) * BlockSize;
            imgShovel->Position.y = (Engine::GameEngine::GetInstance().GetMousePosition().y / BlockSize) * BlockSize;
        }
        ClearTurretInfo();
        return;
    }
    else if (id == 5) {  // Wrench button
        preview = nullptr;
        wrenchMode = true;
        shovelMode = false;
        OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, 
                   Engine::GameEngine::GetInstance().GetMousePosition().y);
        if (imgWrench) {
            imgWrench->Visible = true;
            imgWrench->Position.x = (Engine::GameEngine::GetInstance().GetMousePosition().x / BlockSize) * BlockSize;
            imgWrench->Position.y = (Engine::GameEngine::GetInstance().GetMousePosition().y / BlockSize) * BlockSize;
        }
        ClearTurretInfo();
        return;
    }
    if (!preview)
        return;
    preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
    preview->Tint = al_map_rgba(255, 255, 255, 200);
    preview->Enabled = false;
    preview->Preview = true;
    UIGroup->AddNewObject(preview);
    OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
    
}

bool PlayScene::CheckSpaceValid(int x, int y) {
    if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight)
        return false;

    if (dynamic_cast<Landmine*>(preview)) {
        return mapState[y][x] == TILE_DIRT;
    }
    
    auto map00 = mapState[y][x];
    mapState[y][x] = TILE_OCCUPIED;
    std::vector<std::vector<int>> map = CalculateBFSDistance();
    mapState[y][x] = map00;
    if (map[0][0] == -1)
        return false;
    for (auto &it : EnemyGroup->GetObjects()) {
        Engine::Point pnt;
        pnt.x = floor(it->Position.x / BlockSize);
        pnt.y = floor(it->Position.y / BlockSize);
        if (pnt.x < 0) pnt.x = 0;
        if (pnt.x >= MapWidth) pnt.x = MapWidth - 1;
        if (pnt.y < 0) pnt.y = 0;
        if (pnt.y >= MapHeight) pnt.y = MapHeight - 1;
        if (map[pnt.y][pnt.x] == -1)
            return false;
    }
    // All enemy have path to exit.
    mapState[y][x] = TILE_OCCUPIED;
    mapDistance = map;
    for (auto &it : EnemyGroup->GetObjects())
        dynamic_cast<Enemy *>(it)->UpdatePath(mapDistance);
    return true;
}

void PlayScene::CreateEnemyListUI() {
    // Position below the turret buttons
    int startX = 1294;
    int startY = 500;
    
    // Compact layout settings
    int columnWidth = 120;    // Width of each column
    int columnGap = 10;       // Gap between columns
    int iconSize = 20;        // Icon size
    int rowHeight = 15;       // Height of each enemy row
    int waveTitleHeight = 120; // Vertical space for each wave block
    int wavesPerColumn = 2;   // Number of waves per column

    // Enemy data
    std::map<int, std::string> enemyNames = {
        {1, "Soldier"}, {2, "Plane"}, {3, "Tank"}, {4, "Support"}
    };
    std::map<int, std::string> enemyImages = {
        {1, "play/enemy-1.png"}, {2, "play/enemy-2.png"},
        {3, "play/enemy-3.png"}, {4, "play/enemy-4.png"}
    };

    // Add title label
    UIGroup->AddNewObject(new Engine::Label("Enemies in Wave:", "pirulen.ttf", 18, startX, startY));

    // Create enemy icons and counts for each wave
    for (int wave = 0; wave < allEnemyWaves.size(); wave++) {
        // Count enemies for this wave
        std::map<int, int> enemyCounts;
        for (const auto& enemyData : allEnemyWaves[wave]) {
            enemyCounts[enemyData.first]++;
        }

        if (enemyCounts.empty()) continue;
        
        // Calculate position - waves 1-2 in left column, 3-4 in right column
        int column = wave / wavesPerColumn;
        int waveInColumn = wave % wavesPerColumn;
        
        int waveX = startX + column * (columnWidth + columnGap + 20); // +20 for extra margin
        int waveY = startY + 30 + waveInColumn * waveTitleHeight;
        
        // Create wave label
        UIGroup->AddNewObject(new Engine::Label("Wave " + std::to_string(wave+1) + ":", 
                             "pirulen.ttf", 16, waveX, waveY));

        // Create enemy icons and counts
        int iconY = waveY + 20;
        
        for (const auto& pair : enemyCounts) {
            int enemyType = pair.first;
            std::string enemyName = enemyNames[enemyType];
            std::string enemyImage = enemyImages[enemyType];
            
            // Add enemy icon
            UIGroup->AddNewObject(new Engine::Image(enemyImage, waveX, iconY, iconSize, iconSize));
            
            // Add enemy count and name
            std::string infoText = std::to_string(pair.second) + "x " + enemyName;
            UIGroup->AddNewObject(new Engine::Label(infoText, "pirulen.ttf", 12, 
                                                  waveX + iconSize + 5, iconY + 6));
            
            // Move to next row
            iconY += rowHeight;
        }
    }
}


std::vector<std::vector<int>> PlayScene::CalculateBFSDistance() {
    // Reverse BFS to find path.
    std::vector<std::vector<int>> map(MapHeight, std::vector<int>(std::vector<int>(MapWidth, -1)));
    std::queue<Engine::Point> que;
    // Push end point.
    // BFS from end point.
    if (mapState[MapHeight - 1][MapWidth - 1] != TILE_DIRT)
        return map;
    que.push(Engine::Point(MapWidth - 1, MapHeight - 1));
    map[MapHeight - 1][MapWidth - 1] = 0;
    while (!que.empty()) {
        Engine::Point p = que.front();
        que.pop();
        
        // Check all 4 directions
        for (auto &dir : directions) {
            Engine::Point next(p.x + dir.x, p.y + dir.y);
            
            // Check if next point is within bounds
            if (next.x >= 0 && next.x < MapWidth && next.y >= 0 && next.y < MapHeight) {
                // Check if the tile is walkable (TILE_DIRT) and not yet visited
                if (mapState[next.y][next.x] == TILE_DIRT && map[next.y][next.x] == -1) {
                    // Set distance (current distance + 1)
                    map[next.y][next.x] = map[p.y][p.x] + 1;
                    // Add to queue for further exploration
                    que.push(next);
                }
            }
        }
        // TODO PROJECT-1 (1/1): Implement a BFS starting from the most right-bottom block in the map.
        //               For each step you should assign the corresponding distance to the most right-bottom block.
        //               mapState[y][x] is TILE_DIRT if it is empty. [DONE]
    }
    return map;
}
