#include "SupportEnemy.hpp"
#include "Engine/LOG.hpp"
#include <cmath>
#include "Scene/PlayScene.hpp"

SupportEnemy::SupportEnemy(int x, int y) : Enemy("play/enemy-4.png", x, y, 50, 80, 20, 50) {
    type = "Support";
    supportRadius = 500;
}

void SupportEnemy::Update(float deltaTime) {
    Enemy::Update(deltaTime);
    BuffNearbyAllies();
}

void SupportEnemy::BuffNearbyAllies() {
    auto scene = getPlayScene();
    if (!scene) return;

    for (auto& enemy : scene->EnemyGroup->GetObjects()) {
        Enemy* ally = dynamic_cast<Enemy*>(enemy);
        if (!ally || ally == this || ally->type == "Support" || ally->buffed)
            continue;

        float dx = ally->Position.x - Position.x;
        float dy = ally->Position.y - Position.y;
        float distance = std::sqrt(dx*dx + dy*dy);

        if (distance <= supportRadius) {
            ally->setHP(ally->getHP() * 10);
            ally->buffed = true;
            Engine::LOG(Engine::INFO) << "Buffed " << ally->type << " to " << ally->getHP() << " HP";
        }
    }
}