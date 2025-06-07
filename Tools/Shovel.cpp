// #include "Shovel.hpp"
// #include "Engine/GameEngine.hpp"
// #include "Engine/Group.hpp"
// #include "Engine/IScene.hpp"
// #include "Engine/Point.hpp"
// #include "Engine/Resources.hpp"
// #include "Scene/PlayScene.hpp"
// #include "Turret/Turret.hpp"
// #include <allegro5/allegro.h>
// #include <allegro5/allegro_image.h>

// Shovel::Shovel(float x, float y) : Engine::IObject(x, y, 0, 0) {
//     Position.x = x;
//     Position.y = y;
//     Anchor = Engine::Point(0.5, 0.5);
// }

// void Shovel::Update(float deltaTime) {
//     // No special update logic needed for the shovel
//     IObject::Update(deltaTime);
// }

// void Shovel::Draw() const {
//     std::shared_ptr<ALLEGRO_BITMAP> bmp = Engine::Resources::GetInstance().GetBitmap("play/shovel.png");
//     al_draw_bitmap(bmp.get(), Position.x - Anchor.x * Size.x, Position.y - Anchor.y * Size.y, 0);
// }

// int Shovel::GetPrice() const {
//     return 0; // Shovel is a tool, doesn't cost money to use
// }