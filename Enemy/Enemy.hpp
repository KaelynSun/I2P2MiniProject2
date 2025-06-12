#ifndef ENEMY_HPP
#define ENEMY_HPP
#include <list>
#include <string>
#include <vector>

#include "Engine/Point.hpp"
#include "Engine/Sprite.hpp"

class Bullet;
class PlayScene;
class Turret;

class Enemy : public Engine::Sprite {
protected:
    std::vector<Engine::Point> path;
    float speed;
    float maxHP;
    float hp;
    int money;
    PlayScene *getPlayScene();
    virtual void OnExplode();
    // Atk logic
    float attackRange;
    float attackDamage;
    float attackCooldown;
    float reloadTime = 0;
    Turret* attackTarget = nullptr;
    std::string bulletImage; // Add this line

public:
    std::string type;
    bool buffed = false;
    float reachEndTime;
    std::list<Turret *> lockedTurrets;
    std::list<Bullet *> lockedBullets;
    Enemy(std::string img, float x, float y, float radius, float speed, float hp, int money);
    float GetHealthPercentage() const { return hp / maxHP; }
    std::string GetHealthText() const { return std::to_string((int)hp) + "/" + std::to_string((int)maxHP); }
    float getMaxHP() const { return maxHP; }
    float getHP() const { return hp; }
    void setHP(float value) { hp = value; }
    void Hit(float damage, bool isAOE);
    void Attack();
    void UpdatePath(const std::vector<std::vector<int>> &mapDistance);
    void Update(float deltaTime) override;
    void Draw() const override;
};
#endif   // ENEMY_HPP
