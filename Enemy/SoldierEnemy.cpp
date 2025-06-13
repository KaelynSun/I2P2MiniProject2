#include <string>

#include "SoldierEnemy.hpp"

// TODO HACKATHON-3 (1/3): You can imitate the 2 files: 'SoldierEnemy.hpp', 'SoldierEnemy.cpp' to create a new enemy.
SoldierEnemy::SoldierEnemy(int x, int y) : Enemy("play/enemy-1.png", x, y, 10, 50, 50, 20) {
    bulletImage = "play/bullet-8.png"; // Set the bullet image for this enemy
    attackRange = 200; 
    attackDamage = 5; 
    attackCooldown = 1.0f; 
}
