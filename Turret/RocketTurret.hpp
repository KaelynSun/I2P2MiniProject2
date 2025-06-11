#ifndef ROCKETTURRET_HPP
#define ROCKETTURRET_HPP
#include "Turret.hpp"

class RocketTurret : public Turret {
public:
    static const int Price;
    RocketTurret(float x, float y);
    void CreateBullet() override;
    void Upgrade() override {
        //Turret::Upgrade();
        atk *= 1.5f;
        hp *= 1.5f;
    }
    std::string GetName() const override { return "Rocket Turret"; }
};
#endif   // MACHINEGUNTURRET_HPP
