#ifndef PIERCETURRET_HPP
#define PIERCETURRET_HPP
#include "Turret.hpp"

class PierceTurret : public Turret {
public:
    static const int Price;
    PierceTurret(float x, float y);
    void CreateBullet() override;
    void Upgrade() override {
        //Turret::Upgrade();
        atk *= 1.5f;
        hp *= 1.5f;
    }
    std::string GetName() const override { return "Pierce Turret"; }
};
#endif   // PIERCETURRET_HPP
