#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "Bullet/RocketBullet.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "RocketTurret.hpp"
#include "Scene/PlayScene.hpp"

const int RocketTurret::Price = 200;
RocketTurret::RocketTurret(float x, float y)
    : Turret("play/tower-base.png", "play/turret-6.png", x, y, 500, Price, 2.0) {
        maxLifetime = 50.0f;
    // Move center downward, since we the turret head is slightly biased upward.
    Anchor.y += 8.0f / GetBitmapHeight();
    hp = 500.0f;
    maxHP = 500.0f;
    atk = 30; // Attack will be set by specific turret types
}
void RocketTurret::CreateBullet() {
    Engine::Point diff = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
    float rotation = atan2(diff.y, diff.x);
    Engine::Point normalized = diff.Normalize();
    // Change bullet position to the front of the gun barrel.
    getPlayScene()->BulletGroup->AddNewObject(new RocketBullet(Position + normalized * 36, diff, rotation, this));
    AudioHelper::PlayAudio("explosion.wav");
}
