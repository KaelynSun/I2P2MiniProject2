#include <functional>
#include <memory>

#include "Engine/Collider.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "Image.hpp"
#include "ImageButton.hpp"

namespace Engine {
    ImageButton::ImageButton(std::string img, std::string imgIn, float x, float y, float w, float h, float anchorX, float anchorY) : Image(img, x, y, w, h, anchorX, anchorY), imgOut(Resources::GetInstance().GetBitmap(img)), imgIn(Resources::GetInstance().GetBitmap(imgIn)) {
        Point mouse = GameEngine::GetInstance().GetMousePosition();
        mouseIn = Collider::IsPointInBitmap(Point((mouse.x - Position.x) * GetBitmapWidth() / Size.x + Anchor.x * GetBitmapWidth(), (mouse.y - Position.y) * GetBitmapHeight() / Size.y + Anchor.y * GetBitmapHeight()), bmp);
        if (!mouseIn || !Enabled) bmp = imgOut;
        else bmp = this->imgIn;
    }
    void ImageButton::SetOnClickCallback(std::function<void(void)> onClickCallback) {
        OnClickCallback = onClickCallback;
    }
    void ImageButton::OnMouseDown(int button, int mx, int my) {
        if ((button & 1) && mouseIn && Enabled) {
            if (OnClickCallback)
                OnClickCallback();
        }
    }
void ImageButton::OnMouseMove(int mx, int my) {
        bool wasHovered = mouseIn;
        bool newMouseIn = Collider::IsPointInBitmap(Point((mx - Position.x) * GetBitmapWidth() / Size.x + Anchor.x * GetBitmapWidth(), 
                                        (my - Position.y) * GetBitmapHeight() / Size.y + Anchor.y * GetBitmapHeight()), bmp);
        if (newMouseIn != wasHovered) {
            mouseIn = newMouseIn;
            if (!mouseIn || !Enabled) {
                bmp = imgOut;
                if (wasHovered && OnOutCallback) {
                    OnOutCallback();
                }
            } else {
                bmp = imgIn;
                if (!wasHovered && OnHoverCallback) {
                    OnHoverCallback();
                }
            }
            isHovered = mouseIn && Enabled;
        }
    }
    void ImageButton::SetOnHoverCallback(std::function<void()> callback) {
        OnHoverCallback = callback;
    }
    void ImageButton::SetOnOutCallback(std::function<void()> callback) {
        OnOutCallback = callback;
    }
    void ImageButton::SetHoverTint(ALLEGRO_COLOR color) {
        hoverTint = color;
    }
    void ImageButton::SetNormalTint(ALLEGRO_COLOR color) {
        normalTint = color;
    }
    void ImageButton::Draw() const {
            if (Visible) {
                al_draw_tinted_scaled_bitmap(bmp.get(), isHovered ? hoverTint : normalTint, 
                                            0, 0, al_get_bitmap_width(bmp.get()), al_get_bitmap_height(bmp.get()),
                                            Position.x - Anchor.x * Size.x, Position.y - Anchor.y * Size.y, 
                                            Size.x, Size.y, 0);
            }
        }
    }
