#include <allegro5/base.h>
#include <random>
#include <string>

#include "TankBullet.hpp"
#include <allegro5/allegro_primitives.h>
#include "Engine/LOG.hpp"
#include "Turret/Turret.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Collider.hpp"
#include "Scene/PlayScene.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "Engine/AudioHelper.hpp" // Add this include

TankBullet::TankBullet(std::string img, float speed, float damage,
                         Engine::Point position, Engine::Point forwardDirection,
                         float /*rotation*/) :
    Bullet(img, speed, damage, position, forwardDirection, std::atan2(forwardDirection.y, forwardDirection.x) + ALLEGRO_PI, nullptr, 180, 180) {
    // Make tank bullets even bigger by loading bitmap with larger width and height
    Size = Engine::Point(180, 180); // Set size explicitly
    CollisionRadius = 90;           // Adjust collision radius accordingly (made bigger)
    // Always use the provided forwardDirection for velocity
    float len = std::sqrt(forwardDirection.x * forwardDirection.x + forwardDirection.y * forwardDirection.y);
    Engine::Point normDir = (len > 0.0001f) ? Engine::Point(forwardDirection.x / len, forwardDirection.y / len) : Engine::Point(1, 0);
    Velocity = normDir * speed;
    Tint = al_map_rgba(255, 255, 255, 255); // Ensure full opacity
    Anchor = Engine::Point(0.5, 0.5);

    // Set the rotation so the "up" asset points toward the turret
    Rotation = std::atan2(forwardDirection.y, forwardDirection.x) - ALLEGRO_PI/2;

    // Verify image was loaded
    if (!bmp) {
        Size = Engine::Point(180, 180); // Keep large size even if image fails
        Tint = al_map_rgb(255, 255, 0); // Yellow fallback
    }

    // Play explosion sound when tank shoots bullet
    AudioHelper::PlaySample("explosion.wav");
}

PlayScene* TankBullet::getPlayScene() {
    return dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetActiveScene());
}

void TankBullet::OnExplode(Turret* turret) {
    // Create explosion effect when bullet hits a turret
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(2, 5);

    getPlayScene()->GroundEffectGroup->AddNewObject(
        new DirtyEffect("play/dirty-3.png", dist(rng),
        turret->Position.x, turret->Position.y
    ));

    // Set turret tint to red
    turret->Tint = al_map_rgba(255, 0, 0, 255);
}

void TankBullet::Update(float deltaTime) {
    // Use base Bullet's update but with turret collision instead of enemy
    Sprite::Update(deltaTime);
    PlayScene* scene = getPlayScene();
    
    // // Debug: Draw collision circle
    // if (PlayScene::DebugMode) {
    //     al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(255, 0, 0), 2);
    // }

    // Update position based on velocity
    Position.x += Velocity.x * deltaTime;
    Position.y += Velocity.y * deltaTime;

    // Ensure rotation always matches velocity direction (top points forward)
    Rotation = std::atan2(Velocity.y, Velocity.x) - ALLEGRO_PI / 2;

    // Check for collision with turrets
    for (auto& obj : scene->TowerGroup->GetObjects()) {
        Turret* turret = dynamic_cast<Turret*>(obj);
        if (turret && Engine::Collider::IsCircleOverlap(Position, CollisionRadius, 
                                                      turret->Position, 0.000001f * turret->CollisionRadius)) {
            // Apply damage to turret
            turret->TakeDamage(damage);
            
            // Create explosion effect
            OnExplode(turret);
            
            // Remove the bullet
            getPlayScene()->BulletGroup->RemoveObject(objectIterator);
            return;
        }
    }

    // Remove bullet if it goes out of screen (using scene dimensions instead of GetGameMap)
    int screenWidth = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int screenHeight = Engine::GameEngine::GetInstance().GetScreenSize().y;
    if (Position.x < 0 || Position.x > screenWidth ||
        Position.y < 0 || Position.y > screenHeight) {
        getPlayScene()->BulletGroup->RemoveObject(objectIterator);
    }
}

// Add a Draw override to always draw something visible if bmp is missing
void TankBullet::Draw() const {
    if (bmp) {
        // Debug: Draw a line showing the "up" direction after rotation (draw first so it's not covered)
        float lineLength = 60;
        float dx = std::cos(Rotation) * lineLength;
        float dy = std::sin(Rotation) * lineLength;
        al_draw_line(Position.x, Position.y, Position.x + dx, Position.y + dy, al_map_rgb(255, 0, 255), 4);

        // Draw the bitmap with correct anchor and rotation
        al_draw_tinted_scaled_rotated_bitmap(
            bmp.get(), Tint, 
            Anchor.x * GetBitmapWidth(), Anchor.y * GetBitmapHeight(),
            Position.x, Position.y, 
            Size.x / GetBitmapWidth(), 
            Size.y / GetBitmapHeight(), 
            Rotation, 0);
    } else {
        al_draw_filled_circle(Position.x, Position.y, CollisionRadius, Tint);
        al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(0, 0, 0), 2);
    }
}
