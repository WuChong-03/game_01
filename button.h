#pragma once
#include <Novice.h>
#include <cmath>

struct Button {
    float x, y;
    float w, h;
    float scale;
    float targetScale;
    int texture;
};

// 初始化
inline void InitButton(Button& btn, float x, float y, float w, float h, int tex) {
    btn = { x, y, w, h, 0.7f, 0.7f, tex };
}

// 更新缩放
inline void UpdateButton(Button& btn, bool isSelected) {
    btn.targetScale = isSelected ? 1.0f : 0.7f;
    btn.scale += (btn.targetScale - btn.scale) * 0.25f;
}

// 绘制按钮（支持颜色）
inline void DrawButton(const Button& btn, unsigned int color) {
    float scaledH = btn.h * btn.scale;
    int drawX = static_cast<int>(btn.x);
    int drawY = static_cast<int>(btn.y - scaledH / 2);
    Novice::DrawSprite(drawX, drawY, btn.texture, btn.scale, btn.scale, 0.0f, color);
}
