#ifndef TankBullet_HPP
#define TankBullet_HPP
#include "Bullet.hpp"

class Turret;
class PlayScene;
class Enemy;
namespace Engine {
    struct Point;
}   // namespace Engine

class TankBullet : public Bullet {
private:
    // Enemy bullets don't need a parent turret reference
    PlayScene* getPlayScene();
    void OnExplode(Turret* turret);

public:
    // Constructor modified for enemy bullets
    TankBullet(std::string img, float speed, float damage, 
               Engine::Point position, Engine::Point forwardDirection, 
               float rotation);

    void Update(float deltaTime) override;
    void Draw() const override;
};

#endif // TankBullet_HPP
