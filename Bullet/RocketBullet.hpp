#ifndef ROCKETBULLET_HPP
#define ROCKETBULLET_HPP
#include "Bullet.hpp"

class Enemy;
class PlayScene;
class Turret;

namespace Engine {
    struct Point;
}   // namespace Engine

class RocketBullet : public Bullet {
public:
    explicit RocketBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret *parent);
    void OnExplode(Enemy *enemy) override;
};
#endif   // FIREBULLET_HPP
