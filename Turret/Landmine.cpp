#include <allegro5/allegro_primitives.h>
#include <memory>

#include "Landmine.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "Enemy/Enemy.hpp"
#include "Scene/PlayScene.hpp"
#include "UI/Animation/ExplosionEffect.hpp"

const int Landmine::Price = 200;

Landmine::Landmine(float x, float y) :
    Turret("play/landmine.png", "play/landmine.png", x, y, 60, Price, 999) {
    // You can set cooldown very high since we only trigger once.
    // We can also disable reload by overriding Update()
    Target = nullptr;
    CollisionRadius = 1;
}

void Landmine::Update(float deltaTime) {
    // if (Preview) return; // Skip update logic if still in preview mode

    // PlayScene* scene = getPlayScene();

    // // Loop through enemies to check if any enter the radius
    // for (auto& it : scene->EnemyGroup->GetObjects()) {
    //     Engine::Point diff = it->Position - Position;
    //     if (diff.Magnitude() <= CollisionRadius) {
    //         // Deal damage and explode
    //         Enemy* enemy = dynamic_cast<Enemy*>(it);
    //         if (enemy) {
    //             enemy->Hit(100, true); // true = AOE hit
    //         }
    //         // Explosion visual + sound
    //         scene->EffectGroup->AddNewObject(new ExplosionEffect(Position.x, Position.y));
    //         AudioHelper::PlayAudio("explosion.wav");
    //         // Remove landmine from scene
    //         scene->TowerGroup->RemoveObject(objectIterator);
    //         return;
    //     }
    // }
    if (Preview) return; // Skip update logic if still in preview mode

    PlayScene* scene = getPlayScene();

    // Loop through enemies to check if any enter the radius
    for (auto& it : scene->EnemyGroup->GetObjects()) {
        Engine::Point diff = it->Position - Position;
        if (diff.Magnitude() <= CollisionRadius) {
            // Deal damage and explode
            Enemy* enemy = dynamic_cast<Enemy*>(it);
            if (enemy) {
                enemy->Hit(100, true); // true = AOE hit
            }
            
            // Create explosion effect
            scene->EffectGroup->AddNewObject(new ExplosionEffect(Position.x, Position.y));
            AudioHelper::PlayAudio("explosion.wav");
            
            // Store the iterator before removal
            auto it = objectIterator;
            
            // Remove landmine from scene
            scene->TowerGroup->RemoveObject(it);
            
            // Important: Return immediately after removal to prevent accessing deleted object
            return;
        }
    }
}

void Landmine::Draw() const {
    // Only draw the sprite, do not draw health bar.
    Sprite::Draw();
    // Optionally, add debug drawing if needed.
    // if (PlayScene::DebugMode) { ... }
}

