#include <algorithm>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

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
#include "Turret/LaserTurret.hpp"
#include "Turret/MachineGunTurret.hpp"
#include "Turret/RocketTurret.hpp"
#include "Turret/TurretButton.hpp"
#include "Turret/PierceTurret.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "Scene/WinScene.hpp"
#include "UI/Animation/Plane.hpp"
#include "UI/Component/Label.hpp"
#include "Tools/Shovel.hpp"
#include "Tools/Landmine.hpp"

// TODO HACKATHON-4 (1/3): Trace how the game handles keyboard input.
// TODO HACKATHON-4 (2/3): Find the cheat code sequence in this file.
// TODO HACKATHON-4 (3/3): When the cheat code is entered, a plane should be spawned and added to the scene.
// TODO HACKATHON-5 (1/4): There's a bug in this file, which crashes the game when you win. Try to find it.
// TODO HACKATHON-5 (2/4): The "LIFE" label are not updated when you lose a life. Try to fix it.

bool PlayScene::DebugMode = false;
bool PlayScene::paused = false;
const std::vector<Engine::Point> PlayScene::directions = { Engine::Point(-1, 0), Engine::Point(0, -1), Engine::Point(1, 0), Engine::Point(0, 1) };
const int PlayScene::MapWidth = 20, PlayScene::MapHeight = 13;
const int PlayScene::BlockSize = 64;
const float PlayScene::DangerTime = 7.61;
const Engine::Point PlayScene::SpawnGridPoint = Engine::Point(-1, 0);
const Engine::Point PlayScene::EndGridPoint = Engine::Point(MapWidth, MapHeight - 1);
const std::vector<int> PlayScene::code = { // CHEAT CODE SEQUENCE IS HERE!!!
    ALLEGRO_KEY_UP, ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_DOWN,
    ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT,
    ALLEGRO_KEY_B, ALLEGRO_KEY_A, ALLEGRO_KEYMOD_SHIFT, ALLEGRO_KEY_ENTER
};
Engine::Point PlayScene::GetClientSize() {
    return Engine::Point(MapWidth * BlockSize, MapHeight * BlockSize);
}
void PlayScene::Initialize() {
    mapState.clear();
    keyStrokes.clear();
    ticks = 0;
    deathCountDown = -1;
    lives = 10;
    money = 150;
    SpeedMult = 1;
    enemiesKilled = 0;
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
    mapDistance = CalculateBFSDistance();
    ConstructUI();
    imgTarget = new Engine::Image("play/target.png", 0, 0);
    imgTarget->Visible = false;
    preview = nullptr;
    UIGroup->AddNewObject(imgTarget);
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
    if (paused) return; // Skip update when paused
    // If we use deltaTime directly, then we might have Bullet-through-paper problem.
    // Reference: Bullet-Through-Paper
    if (SpeedMult == 0)
        deathCountDown = -1;
    else if (deathCountDown != -1)
        SpeedMult = 1;
    // Calculate danger zone.
    std::vector<float> reachEndTimes;
    for (auto &it : EnemyGroup->GetObjects()) {
        reachEndTimes.push_back(dynamic_cast<Enemy *>(it)->reachEndTime);
    }
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
        // TODO HACKATHON-5 (1/4)
        //bool changeScenePls = false;
        if (enemyWaveData.empty()) {
            if (EnemyGroup->GetObjects().empty()) {
                // Free resources.
                /*delete TileMapGroup;
                delete GroundEffectGroup;
                delete DebugIndicatorGroup;
                delete TowerGroup;
                delete EnemyGroup;
                delete BulletGroup;
                delete EffectGroup;
                delete UIGroup;
                delete imgTarget;*/
                // Win
                Engine::GameEngine::GetInstance().ChangeScene("win");
                return;
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
            // TODO HACKATHON-3 (2/3): Add your new enemy here.
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
    if (preview) {
        preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
        // To keep responding when paused.
        preview->Update(deltaTime); 
    }
}
void PlayScene::Draw() const {
    IScene::Draw();
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
}
void PlayScene::OnMouseDown(int button, int mx, int my) {
    if (paused) return;
    if ((button & 1) && !imgTarget->Visible && preview) {
        // Cancel turret construct.
        UIGroup->RemoveObject(preview->GetObjectIterator());
        preview = nullptr;
    }
    IScene::OnMouseDown(button, mx, my);
}
void PlayScene::OnMouseMove(int mx, int my) {
    if (paused) return;
    IScene::OnMouseMove(mx, my);
    const int x = mx / BlockSize;
    const int y = my / BlockSize;
    if (!preview || x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
        imgTarget->Visible = false;
        return;
    }
    imgTarget->Visible = true;
    imgTarget->Position.x = x * BlockSize;
    imgTarget->Position.y = y * BlockSize;
}
void PlayScene::OnMouseUp(int button, int mx, int my) {
    if (paused) return;
    IScene::OnMouseUp(button, mx, my);
    if (!imgTarget->Visible)
        return;
    const int x = mx / BlockSize;
    const int y = my / BlockSize;
    if (button & 1) {
        if (dynamic_cast<Shovel*>(preview)) {
            if (mapState[y][x] == TILE_OCCUPIED) {
                for (auto& it : TowerGroup->GetObjects()) {
                    int tx = static_cast<int>(it->Position.x / BlockSize);
                    int ty = static_cast<int>(it->Position.y / BlockSize);
                    if (tx == x && ty == y) {
                        auto itIter = it->GetObjectIterator();  // store iterator
                        itIter->first = false;
                        TowerGroup->RemoveObject(itIter);
                        mapState[y][x] = TILE_FLOOR;
                    }
                }
            }
            UIGroup->RemoveObject(preview->GetObjectIterator());
            preview = nullptr;
        }
        else if (mapState[y][x] != TILE_OCCUPIED || dynamic_cast<Landmine*>(preview)) {
            if (!preview)
                return;
            // Check if valid.
            if (!CheckSpaceValid(x, y)) {
                Engine::Sprite *sprite;
                GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
                sprite->Rotation = 0;
                return;
            }
            // // Purchase.
            // EarnMoney(-preview->GetPrice());
            // // Remove Preview.
            // preview->GetObjectIterator()->first = false;
            // UIGroup->RemoveObject(preview->GetObjectIterator());
            // // Construct real turret.
            // preview->Position.x = x * BlockSize + BlockSize / 2;
            // preview->Position.y = y * BlockSize + BlockSize / 2;
            // preview->Enabled = true;
            // preview->Preview = false;
            // preview->Tint = al_map_rgba(255, 255, 255, 255);
            // TowerGroup->AddNewObject(preview);
            // // To keep responding when paused.
            // preview->Update(0);
            // // Remove Preview.
            // preview = nullptr;

            // Purchase.
            EarnMoney(-preview->GetPrice());
            // Save and remove Preview.
            Turret* turret = preview;
            preview->GetObjectIterator()->first = false;
            UIGroup->RemoveObject(preview->GetObjectIterator());
            preview = nullptr;

            // Construct real turret.
            turret->Position.x = x * BlockSize + BlockSize / 2;
            turret->Position.y = y * BlockSize + BlockSize / 2;
            turret->Enabled = true;
            turret->Preview = false;
            turret->Tint = al_map_rgba(255, 255, 255, 255);
            TowerGroup->AddNewObject(turret);
            // To keep responding when paused.
            turret->Update(0);

            // Don't mark the tile as occupied for landmines
            if (!dynamic_cast<Landmine*>(turret)) {
                mapState[y][x] = TILE_OCCUPIED;
            }
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
    else {
        keyStrokes.push_back(keyCode);
        if (keyStrokes.size() > code.size())
            keyStrokes.pop_front();
            // Hackathon-4
            if (keyCode == ALLEGRO_KEY_ENTER && keyStrokes.size() == code.size()) { // Last code 
                auto it = keyStrokes.begin();
                for (int c : code) { 
                    if (!((*it == c) || // Compare
                        (c == ALLEGRO_KEYMOD_SHIFT &&
                        (*it == ALLEGRO_KEY_LSHIFT || *it == ALLEGRO_KEY_RSHIFT))))
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
    } 
    else if (keyCode == ALLEGRO_KEY_W) {
        // Hotkey for LaserTurret.
        UIBtnClicked(1);
    }
    else if (keyCode == ALLEGRO_KEY_E) {
        // Hotkey for RocketTurret.
        UIBtnClicked(2);
    }
    else if (keyCode == ALLEGRO_KEY_R) {
        // Hotkey for RocketTurret.
        UIBtnClicked(3);
    }
    else if (keyCode == ALLEGRO_KEY_T) {
        // Hotkey for Landmine
        UIBtnClicked(4);
    }
    else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
        // Hotkey for Speed up.
        SpeedMult = keyCode - ALLEGRO_KEY_0;
    }
}
void PlayScene::Hit() { // TODO HACKATHON-5 (2/4)
    lives--;
    UILives->Text = std::string("Life: ") + std::to_string(lives);
    if (lives <= 0) {
        Engine::GameEngine::GetInstance().ChangeScene("lose");
    }
}
int PlayScene::GetMoney() const {
    return money;
}
int PlayScene::GetLives() const {
    return lives;
}
int PlayScene::GetEnemiesKilled() const {
    return enemiesKilled;
}
void PlayScene::EarnMoney(int money) {
    this->money += money;
    UIMoney->Text = std::string("$") + std::to_string(this->money);
    UIMoney->Draw();
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
    std::string filename = std::string("Resource/enemy") + std::to_string(MapId) + ".txt";
    // Read enemy file.
    float type, wait, repeat;
    enemyWaveData.clear();
    std::ifstream fin(filename);
    while (fin >> type && fin >> wait && fin >> repeat) {
        for (int i = 0; i < repeat; i++)
            enemyWaveData.emplace_back(type, wait);
    }
    fin.close();
}
void PlayScene::ConstructUI() {
    // Background
    UIGroup->AddNewObject(new Engine::Image("play/sand.png", 1280, 0, 320, 832));
    // Text
    UIGroup->AddNewObject(new Engine::Label(std::string("Stage ") + std::to_string(MapId), "pirulen.ttf", 32, 1294, 0));
    UIGroup->AddNewObject(UIMoney = new Engine::Label(std::string("$") + std::to_string(money), "pirulen.ttf", 24, 1294, 48));
    UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life ") + std::to_string(lives), "pirulen.ttf", 24, 1294, 88));
    TurretButton *btn;
    // Button 1
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1294, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-1.png", 1294, 136 - 8, 0, 0, 0, 0), 1294, 136, MachineGunTurret::Price);
    // Reference: Class Member Function Pointer and std::bind.
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 0));
    UIGroup->AddNewControlObject(btn);
    // Button 2
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1370, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-2.png", 1370, 136 - 8, 0, 0, 0, 0), 1370, 136, LaserTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 1));
    UIGroup->AddNewControlObject(btn);
    // Button 3
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1446, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-6.png", 1446, 136 - 8, 0, 0, 0, 0), 1446, 136, RocketTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 2));
    UIGroup->AddNewControlObject(btn);
    //Button 4
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1522, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-3.png", 1522, 136 - 8, 0, 0, 0, 0), 1522, 136, PierceTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 5));
    UIGroup->AddNewControlObject(btn);
    // Shovel button
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/towerbase.png", 1294, 215, 0, 0, 0, 0),
                           Engine::Sprite("play/shovel.png", 1294, 215 - 8, 0, 0, 0, 0), 1294, 215, 0);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 3));
    UIGroup->AddNewControlObject(btn);
    // Landmine button
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                        Engine::Sprite("play/landmine.png", 1370, 215, 0, 0, 0, 0),
                        Engine::Sprite("play/landmine.png", 1370, 215, 0, 0, 0, 0), 1370, 215, Landmine::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 4));
    UIGroup->AddNewControlObject(btn);

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
}
// Add turret
void PlayScene::UIBtnClicked(int id) {
    Engine::Point mousePos = Engine::GameEngine::GetInstance().GetMousePosition();
    if (preview)
        UIGroup->RemoveObject(preview->GetObjectIterator());
    if (id == 0 && money >= MachineGunTurret::Price)
        preview = new MachineGunTurret(0, 0);
    else if (id == 1 && money >= LaserTurret::Price)
        preview = new LaserTurret(0, 0);
    else if (id == 2 && money >= RocketTurret::Price)
        preview = new RocketTurret(0, 0);
    else if (id == 3)
        preview = new Shovel(0, 0);
    else if (id == 4 && money >= Landmine::Price)
        preview = new Landmine(1402, 247);
    else if (id == 5 && money >= PierceTurret::Price)
        preview = new PierceTurret(0, 0);
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

     // Allow landmines to be placed on enemy paths (TILE_DIRT)
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
    
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    
    while (!que.empty()) {
        Engine::Point p = que.front();
        que.pop();
        // TODO PROJECT-1 (1/1): Implement a BFS starting from the most right-bottom block in the map.
        //               For each step you should assign the corresponding distance to the most right-bottom block.
        //               mapState[y][x] is TILE_DIRT if it is empty.
        int currDist = map[p.y][p.x];
        for (int i = 0; i < 4; ++i) { // 4 direction
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];
            if (nx >= 0 && nx < MapWidth && ny >= 0 && ny < MapHeight) {
                if (map[ny][nx] == -1 && mapState[ny][nx] == TILE_DIRT) {
                    map[ny][nx] = currDist + 1;
                    que.push(Engine::Point(nx, ny));
                }
            }
        }
    }
    return map;
}
float PlayScene::CalculateDistance(const Engine::Point& p1, const Engine::Point& p2) {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}