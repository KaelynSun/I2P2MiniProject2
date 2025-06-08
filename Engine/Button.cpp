#include "Button.hpp"
#include "Point.hpp"
#include "Resources.hpp"

namespace Engine {
    Button::Button(std::string img, std::string hoverImg, float x, float y) 
        : imgPath(std::move(img)), hoverImgPath(std::move(hover)) {
        Position = Point(x, y);
        image = new Image(imgPath, x, y);
        hoverImage = new Image(hoverImgPath, x, y);
    }

    void Button::SetOnClickCallback(std::function<void()> callback) {
        onClick = std::move(callback);
    }

    void Button::SetEnabled(bool enabled) {
        this->enabled = enabled;
    }

    void Button::SetText(const std::string& text) {
        // Optional: Add text to button if needed
    }

    void Button::Draw() const override {
        if (hovered && enabled) 
            hoverImage->Draw();
        else 
            image->Draw();
    }

    void Button::OnMouseDown(int button, int mx, int my) override {
        if (button == 1 && enabled && hovered && onClick) {
            onClick();
        }
    }

    void Button::OnMouseMove(int mx, int my) override {
        hovered = image->Visible && image->GetImageBitmap() && 
                 mx >= Position.x && mx <= Position.x + image->GetBitmapWidth() &&
                 my >= Position.y && my <= Position.y + image->GetBitmapHeight();
    }
}