#ifndef PIERCEBULLET_HPP
#define PIERCEBULLET_HPP
#include "Bullet.hpp"

class Enemy;
class Turret;
namespace Engine {
    struct Point;
}   // namespace Engine

class PierceBullet : public Bullet {
private:
    int pierceCount; // How many enemies it can still hit
public:
    PierceBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret* parent, int maxPierce);
    void OnExplode(Enemy* enemy) override;
    void Update(float deltaTime) override;
};

#endif // PIERCEBULLET_HPP