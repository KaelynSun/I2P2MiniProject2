#include <allegro5/allegro_primitives.h>
#include <allegro5/color.h>
#include <cmath>
#include <utility>

#include "Enemy/Enemy.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/IObject.hpp"
#include "Engine/IScene.hpp"
#include "Engine/Point.hpp"
#include "Scene/PlayScene.hpp"
#include "Turret.hpp"
#include "UI/Animation/ExplosionEffect.hpp"  // For ExplosionEffect
#include "Engine/AudioHelper.hpp"           // For AudioHelper

PlayScene *Turret::getPlayScene() {
    return dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetActiveScene());
}
Turret::Turret(std::string imgBase, std::string imgTurret, float x, float y, float radius, int price, float coolDown) : 
Sprite(imgTurret, x, y), price(price), coolDown(coolDown), imgBase(imgBase, x, y), atk(0), hp(0) {
    CollisionRadius = radius;
    hp = 100.0f;
    maxHP = 100.0f;
    atk = 0; // Attack will be set by specific turret types
}
void Turret::Update(float deltaTime) {
    Sprite::Update(deltaTime);
    PlayScene *scene = getPlayScene();
    imgBase.Position = Position;

    // Reset tint to normal (white) at the start of each update
    Tint = al_map_rgb(255, 255, 255);
    imgBase.Tint = Tint;

    lifetime += deltaTime; // Increase time alive
    if (lifetime >= maxLifetime) {
        Enabled = false; // Disable turret
        Target = nullptr;
        return;
    }
    
    if (!Enabled)
        return;
    if (Target) {
        Engine::Point diff = Target->Position - Position;
        if (diff.Magnitude() > CollisionRadius) {
            Target->lockedTurrets.erase(lockedTurretIterator);
            Target = nullptr;
            lockedTurretIterator = std::list<Turret *>::iterator();
        }
    }
    if (!Target) {
        // Lock first seen target.
        // Can be improved by Spatial Hash, Quad Tree, ...
        // However simply loop through all enemies is enough for this program.
        for (auto &it : scene->EnemyGroup->GetObjects()) {
            Engine::Point diff = it->Position - Position;
            if (diff.Magnitude() <= CollisionRadius) {
                Target = dynamic_cast<Enemy *>(it);
                Target->lockedTurrets.push_back(this);
                lockedTurretIterator = std::prev(Target->lockedTurrets.end());
                break;
            }
        }
    }
    if (Target) {
        Engine::Point originRotation = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
        Engine::Point targetRotation = (Target->Position - Position).Normalize();
        float maxRotateRadian = rotateRadian * deltaTime;
        float cosTheta = originRotation.Dot(targetRotation);
        // Might have floating-point precision error.
        if (cosTheta > 1) cosTheta = 1;
        else if (cosTheta < -1) cosTheta = -1;
        float radian = acos(cosTheta);
        Engine::Point rotation;
        if (abs(radian) <= maxRotateRadian)
            rotation = targetRotation;
        else
            rotation = ((abs(radian) - maxRotateRadian) * originRotation + maxRotateRadian * targetRotation) / radian;
        // Add 90 degrees (PI/2 radian), since we assume the image is oriented upward.
        Rotation = atan2(rotation.y, rotation.x) + ALLEGRO_PI / 2;
        // Shoot reload.
        reload -= deltaTime;
        if (reload <= 0) {
            // shoot.
            reload = coolDown;
            CreateBullet();
        }
    }
}
void Turret::Draw() const {
    if (Preview) {
        al_draw_filled_circle(Position.x, Position.y, CollisionRadius, al_map_rgba(0, 255, 0, 50));
    }

    imgBase.Draw();
    Sprite::Draw();

    // Only draw red "X" if not in preview mode
    if (!Preview && !Enabled) {
        // Draw a red "X" or overlay if broken
        al_draw_line(Position.x - 10, Position.y - 10, Position.x + 10, Position.y + 10, al_map_rgb(255, 0, 0), 3);
        al_draw_line(Position.x - 10, Position.y + 10, Position.x + 10, Position.y - 10, al_map_rgb(255, 0, 0), 3);
    }
    
    // Show health bar only if not destroyed
    if (!Preview && !isDestroyed) {
        // Draw health bar
        float healthRatio = hp / maxHP;
        float barWidth = 40;
        float barHeight = 5;
        
        // Background
        al_draw_filled_rectangle(
            Position.x - barWidth/2, Position.y - 30,
            Position.x + barWidth/2, Position.y - 25,
            al_map_rgb(100, 100, 100)
        );
        
        // Health
        al_draw_filled_rectangle(
            Position.x - barWidth/2, Position.y - 30,
            Position.x - barWidth/2 + barWidth * healthRatio, Position.y - 25,
            healthRatio > 0.6 ? al_map_rgb(0, 255, 0) :
            healthRatio > 0.3 ? al_map_rgb(255, 165, 0) :
                               al_map_rgb(255, 0, 0)
        );
        
        // Border
        al_draw_rectangle(
            Position.x - barWidth/2, Position.y - 30,
            Position.x + barWidth/2, Position.y - 25,
            al_map_rgb(255, 255, 255), 1
        );
    }

    if (PlayScene::DebugMode) {
        // Draw target radius.
        al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(0, 0, 255), 2);
    }
}
int Turret::GetPrice() const {
    return price;
}
void Turret::TakeDamage(float damage) {
    if (isDestroyed || !Enabled) return;
    
    hp -= damage;
    printf("Turret took %f damage, remaining HP: %f/%f\n", damage, hp, maxHP);
    
    // Visual feedback for being hit
    Tint = al_map_rgb(255, 100, 100); // Flash red
    
    if (hp <= 0) {
        DestroyTurret();
    }
}
void Turret::DestroyTurret() {
    if (isDestroyed) return;
    
    isDestroyed = true;
    Enabled = false;
    
    // Clear target references
    if (Target) {
        Target->lockedTurrets.erase(lockedTurretIterator);
        Target = nullptr;
    }
    
    // Explosion effect
    getPlayScene()->EffectGroup->AddNewObject(
        new ExplosionEffect(Position.x, Position.y)
    );
    
    // Free the map tile
    int x = static_cast<int>(Position.x / PlayScene::BlockSize);
    int y = static_cast<int>(Position.y / PlayScene::BlockSize);
    if (x >= 0 && x < PlayScene::MapWidth && y >= 0 && y < PlayScene::MapHeight) {
        getPlayScene()->FreeMapTile(x, y);
    }
    
    // Play sound
    AudioHelper::PlayAudio("explosion.wav");
    
    // Remove from game (handled by PlayScene's update)
}
