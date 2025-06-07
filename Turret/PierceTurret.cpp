#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "Bullet/PierceBullet.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "PierceTurret.hpp"
#include "Scene/PlayScene.hpp"

const int PierceTurret::Price = 200;

PierceTurret::PierceTurret(float x, float y)
    : Turret("play/tower-base.png", "play/turret-3.png", x, y, 350, Price, 0.8) {
    Anchor.y += 8.0f / GetBitmapHeight();
}

void PierceTurret::CreateBullet() {
    Engine::Point dir = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
    float rotation = atan2(dir.y, dir.x);
    Engine::Point offset = dir.Normalize() * 36;

    getPlayScene()->BulletGroup->AddNewObject(new PierceBullet(Position + offset, dir, rotation, this, 3)); // pierces 3 enemies
    AudioHelper::PlayAudio("gun.wav");
}