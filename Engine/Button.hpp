#pragma once
#include "IControl.hpp"
#include "UI/Component/Image.hpp"

namespace Engine {
    class Button : public IControl {
    private:
        std::string imgPath;
        std::string hoverImgPath;
        std::function<void()> onClick;
        bool hovered = false;
        Image* image;
        Image* hoverImage;
    public:
        bool Enabled = true;
        Button(std::string img, std::string hoverImg, float x, float y);
        void SetOnClickCallback(std::function<void()> callback);
        void SetEnabled(bool enabled);
        void SetText(const std::string& text);
        void Draw() const;
        void OnMouseDown(int button, int mx, int my) override;
        void OnMouseMove(int mx, int my) override;
    };
}