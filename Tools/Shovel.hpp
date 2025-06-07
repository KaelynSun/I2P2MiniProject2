// // Shovel.hpp
// #ifndef SHOVEL_HPP
// #define SHOVEL_HPP
// #include "Engine/IObject.hpp"
// #include "Engine/Point.hpp"
// #include "Engine/Sprite.hpp"

// class Shovel : public Engine::IObject {
// public:
//     explicit Shovel(float x, float y);
//     void Update(float deltaTime) override;
//     void Draw() const override;
//     int GetPrice() const;
// };
// #endif // SHOVEL_HPP
#ifndef SHOVEL_HPP
#define SHOVEL_HPP

#include "Turret/Turret.hpp"

class Shovel : public Turret {
public:
    explicit Shovel(float x, float y)
        : Turret("play/towerbase.png", "play/shovel.png", x, y, /*radius=*/16, /*price=*/0, /*coolDown=*/0) {
        // Shovel specific initialization if needed
        Enabled = true;
        Preview = false;
    }

    void Update(float deltaTime) override {
        // Implement shovel-specific update logic if needed
    }

    void Draw() const override {
        Turret::Draw();  // Use base draw, or override with shovel-specific drawing
    }

    int GetPrice() const override {
        return 0;  // Shovel is free or set your price here
    }

protected:
    void CreateBullet() override {
        // Shovel doesn't shoot bullets, so leave empty or no-op
    }
};

#endif  // SHOVEL_HPP
