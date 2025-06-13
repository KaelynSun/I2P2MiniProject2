#include <allegro5/base.h>
#include <random>
#include <string>

#include "PlaneBullet.hpp"
#include <allegro5/allegro_primitives.h>
#include "Engine/LOG.hpp"
#include "Turret/Turret.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Collider.hpp"
#include "Scene/PlayScene.hpp"
#include "UI/Animation/DirtyEffect.hpp"

PlaneBullet::PlaneBullet(std::string img, float speed, float damage,
                         Engine::Point position, Engine::Point forwardDirection,
                         float rotation) :
    Bullet(img, speed, damage, position, forwardDirection, rotation, nullptr) {
    // Make plane bullets bigger
    Size = Engine::Point(80, 80); // Larger size for plane bullet
    CollisionRadius = 40;         // Adjust collision radius accordingly

    // Always use the provided forwardDirection for velocity
    float len = std::sqrt(forwardDirection.x * forwardDirection.x + forwardDirection.y * forwardDirection.y);
    Engine::Point normDir = (len > 0.0001f) ? Engine::Point(forwardDirection.x / len, forwardDirection.y / len) : Engine::Point(1, 0);
    Velocity = normDir * speed;

    // To rotate 180 degrees from the current orientation, add ALLEGRO_PI
    Rotation = atan2(normDir.y, normDir.x) + ALLEGRO_PI/2;

    Anchor = Engine::Point(0.5, 0.5);
        
    // Verify image was loaded
    if (!bmp) {
        Size = Engine::Point(40, 40); // Fallback size for missing image
        Tint = al_map_rgb(255, 255, 0); // Yellow fallback
        // Engine::LOG() << "PlaneBullet image not loaded: " << img;
    }
}

PlayScene* PlaneBullet::getPlayScene() {
    return dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetActiveScene());
}

void PlaneBullet::OnExplode(Turret* turret) {
    // Create explosion effect when bullet hits a turret
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(2, 5);
    
    getPlayScene()->GroundEffectGroup->AddNewObject(
        new DirtyEffect("play/dirty-3.png", dist(rng), 
        turret->Position.x, turret->Position.y
    ));
}

void PlaneBullet::Update(float deltaTime) {
    // Use base Bullet's update but with turret collision instead of enemy
    Sprite::Update(deltaTime);
    PlayScene* scene = getPlayScene();

    // Update position based on velocity
    Position.x += Velocity.x * deltaTime;
    Position.y += Velocity.y * deltaTime;

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
void PlaneBullet::Draw() const {
    if (bmp) {
        Sprite::Draw();
    } else {
        al_draw_filled_circle(Position.x, Position.y, CollisionRadius, Tint);
        al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(0, 0, 0), 2);
    }
}
