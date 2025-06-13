#include <allegro5/base.h>
#include <random>
#include <string>

#include "Engine/Point.hpp"
#include "TankEnemy.hpp"
#include "Turret/Turret.hpp" // Add this include for full Turret definition

TankEnemy::TankEnemy(int x, int y)
    : Enemy("play/enemy-3.png", x, y, 20, 20, 100, 50),
      head("play/enemy-3-head.png", x, y), targetRotation(0) {
    bulletImage = "play/bullet-4.png"; // Set the bullet image for this enemy
    attackRange = 200; 
    attackDamage = 25; 
    attackCooldown = 3.0f; 
}
void TankEnemy::Draw() const {
    Enemy::Draw();
    head.Draw();
}
void TankEnemy::Update(float deltaTime) {
    Enemy::Update(deltaTime);
    head.Position = Position;

    // Make tank head point to the attack target if available
    if (attackTarget) {
        Engine::Point diff = attackTarget->Position - Position;
        head.Rotation = atan2(diff.y, diff.x);
    } else {
        // Idle/random rotation as fallback
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_real_distribution<> dist(0.0f, 4.0f);
        float rnd = dist(rng);
        if (rnd < deltaTime) {
            std::uniform_real_distribution<> distRadian(-ALLEGRO_PI, ALLEGRO_PI);
            targetRotation = distRadian(rng);
        }
        head.Rotation = (head.Rotation + deltaTime * targetRotation) / (1 + deltaTime);
    }
}
