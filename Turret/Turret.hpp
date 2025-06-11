#ifndef TURRET_HPP
#define TURRET_HPP
#include <allegro5/base.h>
#include <list>
#include <string>

#include "Engine/Sprite.hpp"

class Enemy;
class PlayScene;

class Turret : public Engine::Sprite {
protected:
    int price;
    float coolDown;
    float reload = 0;
    float rotateRadian = 2 * ALLEGRO_PI;
    Sprite imgBase;
    std::list<Turret *>::iterator lockedTurretIterator;
    PlayScene *getPlayScene();
    // Reference: Design Patterns - Factory Method.
    virtual void CreateBullet() = 0;
    float hp, atk;

public:
    // New methods for upgrading
    virtual void SetDamage(float damage) { this->atk = damage; }
    virtual void SetHealth(float health) { this->hp = health; }
    virtual std::string GetName() const = 0;
    float GetDamage() const { return atk; }
    float GetHealth() const { return hp; }
    virtual int GetUpgradeCost() const { return price * 2; }
    virtual void Upgrade() {
        printf("Turret::Upgrade called. Before upgrade atk: %f, hp: %f\n", atk, hp);
        atk *= 1.5f;
        hp *= 1.5f;
        printf("Turret::Upgrade called. After upgrade atk: %f, hp: %f\n", atk, hp);
    }
    bool Enabled = true;
    bool Preview = false;
    Enemy *Target = nullptr;
    Turret(std::string imgBase, std::string imgTurret, float x, float y, float radius, int price, float coolDown);
    void Update(float deltaTime) override;
    void Draw() const override;
    int GetPrice() const;
};
#endif   // TURRET_HPP
