#include "Bullet.hpp"
#include "Enemy/Enemy.hpp"
#include "Enemy/PlaneEnemy.hpp"
#include "Enemy/TankEnemy.hpp"
#include "Enemy/SoldierEnemy.hpp"
#include "Enemy/SupportEnemy.hpp"
#include "Turret/Turret.hpp"
#include "Turret/RocketTurret.hpp"
#include "Turret/LaserTurret.hpp"
#include "Turret/MachineGunTurret.hpp"
#include "Turret/PierceTurret.hpp"
#include "Turret/Landmine.hpp"
#include "Engine/Collider.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/IObject.hpp"
#include "Engine/IScene.hpp"
#include "Engine/Point.hpp"
#include "Engine/Sprite.hpp"
#include "Scene/PlayScene.hpp"

PlayScene *Bullet::getPlayScene() {
    return dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetActiveScene());
}
void Bullet::OnExplode(Enemy *enemy) {
}
Bullet::Bullet(std::string img, float speed, float damage, Engine::Point position, Engine::Point forwardDirection, float rotation, Turret *parent, float width, float height)
    : Sprite(img, position.x, position.y, width, height), speed(speed), damage(damage), parent(parent) {
    Velocity = forwardDirection.Normalize() * speed;
    Rotation = rotation;
    CollisionRadius = 4;
    // Remove or comment out any line that sets Size here, so derived classes can control Size.
    // Size = Engine::Point(32, 32); // <-- REMOVE or COMMENT OUT this line if present
}
void Bullet::Update(float deltaTime) {
    Position = Position + Velocity * deltaTime; // Move bullet by velocity
    Sprite::Update(deltaTime);
    PlayScene *scene = getPlayScene();
    // Can be improved by Spatial Hash, Quad Tree, ...
    // However simply loop through all enemies is enough for this program.
    for (auto &it : scene->EnemyGroup->GetObjects()) {
        Enemy *enemy = dynamic_cast<Enemy *>(it);
        if (!enemy->Visible)
            continue;
        if (Engine::Collider::IsCircleOverlap(Position, CollisionRadius, enemy->Position, enemy->CollisionRadius)) {
            OnExplode(enemy);
            float appliedDamage = damage;

            // Apply damage multiplier based on enemy and turret types
            if (parent) {
                // Fallback: use turret GetName() string for immunity check
                std::string turretName = parent->GetName();

                if ((dynamic_cast<PlaneEnemy*>(enemy) && turretName == "Laser Turret") ||
                    (dynamic_cast<TankEnemy*>(enemy) && turretName == "Machine Gun Turret") ||
                    (dynamic_cast<SupportEnemy*>(enemy) && turretName == "Pierce Turret") ||
                    ((dynamic_cast<PlaneEnemy*>(enemy) && turretName == "Landmine"))) {
                    appliedDamage = 0.0f;
                }
                // Damage multiplier cases
                else if (dynamic_cast<TankEnemy*>(enemy) && turretName == "Rocket Turret") {
                    appliedDamage *= 1.5f;
                } else if (dynamic_cast<SupportEnemy*>(enemy) && turretName == "Laser Turret") {
                    appliedDamage *= 1.5f;
                } else if (dynamic_cast<SoldierEnemy*>(enemy) && turretName == "Machine Gun Turret") {
                    appliedDamage *= 1.5f;
                } else if (dynamic_cast<PlaneEnemy*>(enemy) && turretName == "Pierce Turret") {
                    appliedDamage *= 1.5f;
                } else if (dynamic_cast<SoldierEnemy*>(enemy) && turretName == "Landmine") {
                    appliedDamage *= 1.5f;
                }
            }

            enemy->Hit(appliedDamage, false);
            getPlayScene()->BulletGroup->RemoveObject(objectIterator);
            return;
        }
    }
    // Check if out of boundary.
    if (!Engine::Collider::IsRectOverlap(Position - Size / 2, Position + Size / 2, Engine::Point(0, 0), PlayScene::GetClientSize()))
        getPlayScene()->BulletGroup->RemoveObject(objectIterator);
}

void Bullet::Draw() const {
    Sprite::Draw();
}
