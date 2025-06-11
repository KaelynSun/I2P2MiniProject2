#ifndef LASERTURRET_HPP
#define LASERTURRET_HPP
#include "Turret.hpp"

class LaserTurret : public Turret {
public:
    static const int Price;
    LaserTurret(float x, float y);
    void CreateBullet() override;
    void Upgrade() override {
        //Turret::Upgrade();
        atk *= 1.5f;
        hp *= 1.5f;
    }
    std::string GetName() const override { return "Laser Turret"; }
};
#endif   // LASERTURRET_HPP
