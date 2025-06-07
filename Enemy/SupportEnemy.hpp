#ifndef SUPPORTENEMY_HPP
#define SUPPORTENEMY_HPP
#include "Enemy.hpp"

class SupportEnemy : public Enemy {
private:
    float supportRadius; //radius that allows this enemy to buff up its surroundings
    std::vector<Enemy*> buffedEnemies;
public:
    SupportEnemy(int x, int y);
    void Update(float deltaTime) override;
    void BuffNearbyAllies();

};
#endif   // SUPPORTENEMY_HPP
