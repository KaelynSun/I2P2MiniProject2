#include <cmath>
#include <string>

#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/IScene.hpp"
#include "Engine/Resources.hpp"
#include "BigExplosionEffect.hpp"
#include "Scene/PlayScene.hpp"
#include "Enemy/Enemy.hpp"
#include <allegro5/allegro_primitives.h>

PlayScene *BigExplosionEffect::getPlayScene() {
    return dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetActiveScene());
}
BigExplosionEffect::BigExplosionEffect(float x, float y, float radius, float damage) : Sprite("play/explosion-1.png", x, y), timeTicks(0), timeSpan(0.7f),
    damageRadius(radius), damageValue(damage), damageApplied(false) {
    for (int i = 1; i <= 5; i++) {
        bmps.push_back(Engine::Resources::GetInstance().GetBitmap("play/explosion-" + std::to_string(i) + ".png"));
    }

    // Set initial size (twice as big as regular explosion)
    Size.x = 60;
    Size.y = 60;
}
void BigExplosionEffect::Update(float deltaTime) {
    timeTicks += deltaTime;

    // Apply damage once at the start of the explosion
    if (!damageApplied) {
        for (auto& it : getPlayScene()->EnemyGroup->GetObjects()) {
            Enemy* enemy = dynamic_cast<Enemy*>(it);
            if (!enemy) continue;
            
            Engine::Point diff = enemy->Position - Position;
            if (diff.Magnitude() <= damageRadius) {
                // Scale damage based on distance (more damage closer to center)
                float distanceFactor = 1.0f - (diff.Magnitude() / damageRadius);
                float scaledDamage = damageValue * (0.5f + 0.5f * distanceFactor);
                enemy->Hit(scaledDamage, true); // true indicates AOE damage
            }
        }
        damageApplied = true;
    }

    if (timeTicks >= timeSpan) {
        getPlayScene()->EffectGroup->RemoveObject(objectIterator);
        return;
    }
    int phase = floor(timeTicks / timeSpan * bmps.size());
    bmp = bmps[std::min(phase, static_cast<int>(bmps.size()) - 1)];
    
    // Scale effect based on time (starts big, grows slightly, then shrinks)
    float t = timeTicks / timeSpan;
    t = std::min(t, 1.0f);
    float easeOut = 1 - (1 - t) * (1 - t);  // Smoothly ease to 1

    float maxSize = damageRadius * 2; // Grow to cover full radius visually
    Size.x = maxSize * easeOut;
    Size.y = maxSize * easeOut;
    Sprite::Update(deltaTime);
}
void BigExplosionEffect::Draw() const {
    if (bmp) {
        // Draw with scaling and transparency effect
        float t = timeTicks / timeSpan;
        t = std::min(t, 1.0f);
        float alpha = 255 * (1 - t * t); // Fade out faster at the end
        al_draw_tinted_scaled_bitmap(
            bmp.get(), 
            al_map_rgba(255, 255, 255, alpha),
            0, 0, 
            60, 
            60,
            Position.x - Size.x / 2, 
            Position.y - Size.y / 2,
            Size.x, Size.y, 
            0
        );
        
        // Draw damage radius in debug mode
        if (PlayScene::DebugMode) {
            al_draw_circle(Position.x, Position.y, damageRadius, al_map_rgba(255, 100, 0, 100), 3);
        }
    }
}