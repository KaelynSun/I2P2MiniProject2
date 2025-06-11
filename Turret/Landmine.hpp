#ifndef LANDMINE_HPP
#define LANDMINE_HPP
#include "Turret/Turret.hpp"

class Landmine : public Turret {
public:
    static const int Price;
    Landmine(float x, float y);

    void Update(float deltaTime) override;
    void CreateBullet() override {} // No bullets needed
    void Upgrade() override {
        atk *= 1.5f;
        hp *= 1.5f;
    }
    std::string GetName() const override { return "Landmine"; }
};
#endif // LANDMINETURRET_HPP
