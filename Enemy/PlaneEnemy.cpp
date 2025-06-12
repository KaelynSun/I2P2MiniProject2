#include <string>

#include "PlaneEnemy.hpp"

// TODO HACKATHON-3 (1/3): You can imitate the 2 files: 'SoldierEnemy.hpp', 'SoldierEnemy.cpp' to create a new enemy.
PlaneEnemy::PlaneEnemy(int x, int y) : Enemy("play/enemy-2.png", x, y, 10, 50, 5, 5) {
    bulletImage = "play/bullet-9.png"; // Set the bullet image for this enemy
    attackRange = 400; 
    attackDamage = 1; 
    attackCooldown = 0.5f; 
}
