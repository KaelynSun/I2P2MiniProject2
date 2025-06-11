#ifndef MACHINEGUNTURRET_HPP
#define MACHINEGUNTURRET_HPP
#include "Turret.hpp"

class MachineGunTurret : public Turret {
public:
    static const int Price;
    MachineGunTurret(float x, float y);
    void CreateBullet() override;
    void Upgrade() override {
        //Turret::Upgrade();
        atk *= 1.5f;
        hp *= 1.5f;
    }
    std::string GetName() const override { return "Machine Gun Turret"; }
};
#endif   // MACHINEGUNTURRET_HPP
