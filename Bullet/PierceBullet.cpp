#include <allegro5/base.h>
#include <random>
#include <string>

#include "Enemy/Enemy.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "Engine/Collider.hpp"
#include "PierceBullet.hpp"
#include "Scene/PlayScene.hpp"
#include "UI/Animation/DirtyEffect.hpp"

class Turret;

PierceBullet::PierceBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret* parent, int maxPierce)
    : Bullet("play/bullet-3.png", 800, 2, position, forwardDirection, rotation + ALLEGRO_PI/2, parent), pierceCount(maxPierce) {
}

void PierceBullet::OnExplode(Enemy* enemy) {
    pierceCount--;
    // visual effect on hit
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(1, 5);
    getPlayScene()->GroundEffectGroup->AddNewObject(
        new DirtyEffect("play/dirty-1.png", dist(rng), enemy->Position.x, enemy->Position.y));
}

void PierceBullet::Update(float deltaTime) {
    Sprite::Update(deltaTime);

    PlayScene* scene = getPlayScene();

    for (auto& it : scene->EnemyGroup->GetObjects()) {
        Enemy* enemy = dynamic_cast<Enemy*>(it);
        if (!enemy->Visible)
            continue;

        if (Engine::Collider::IsCircleOverlap(Position, CollisionRadius, enemy->Position, enemy->CollisionRadius)) {
            OnExplode(enemy);
            enemy->Hit(damage, false);

            if (pierceCount <= 0) {
                scene->BulletGroup->RemoveObject(objectIterator);
                return;
            }
            // If piercing remains, bullet continues without removal
        }
    }

    // Remove bullet if it goes out of bounds
    if (!Engine::Collider::IsRectOverlap(Position - Size / 2, Position + Size / 2,
                                         Engine::Point(0, 0), PlayScene::GetClientSize())) {
        scene->BulletGroup->RemoveObject(objectIterator);
    }
}
