#include <string>

#include "SoldierEnemy.hpp"

// TODO HACKATHON-3 (1/3): You can imitate the 2 files: 'SoldierEnemy.hpp', 'SoldierEnemy.cpp' to create a new enemy.
SoldierEnemy::SoldierEnemy(int x, int y) : Enemy("play/enemy-1.png", x, y, 10, 50, 5, 5) {
    bulletImage = "play/bullet-8.png"; // Set the bullet image for this enemy
    attackRange = 200; 
    attackDamage = 1; 
    attackCooldown = 1.5f; 
}
