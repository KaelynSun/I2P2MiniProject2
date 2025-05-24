#ifndef PIERCETURRET_HPP
#define PIERCETURRET_HPP
#include "Turret.hpp"

class PierceTurret : public Turret {
public:
    static const int Price;
    PierceTurret(float x, float y);
    void CreateBullet() override;
};
#endif   // PIERCETURRET_HPP
