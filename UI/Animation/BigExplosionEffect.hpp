#ifndef BIGEXPLOSIONEFFECT_HPP
#define BIGEXPLOSIONEFFECT_HPP
#include <allegro5/bitmap.h>
#include <memory>
#include <vector>

#include "Engine/Sprite.hpp"
#include "Engine/Point.hpp"

class PlayScene;

class BigExplosionEffect : public Engine::Sprite {
protected:
    PlayScene *getPlayScene();
    float timeTicks;
    std::vector<std::shared_ptr<ALLEGRO_BITMAP>> bmps;
    float timeSpan;
    float damageRadius;
    float damageValue;
    bool damageApplied;

public:
    BigExplosionEffect(float x, float y, float radius = 300.0f, float damage = 100.0f);
    void Update(float deltaTime) override;
    void Draw() const override;
};
#endif   // EXPLOSIONEFFECT_HPP
