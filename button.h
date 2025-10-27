#pragma once
#include <Novice.h>
#include <cmath>

struct Button {
    float x{ 850.0f }, y{ 400.0f };
    float w{ 280.0f }, h{ 63.0f };
    float scale{ 0.7f };
    float targetScale{ 0.7f };
    int texture{ -1 };

    // 水平飞行动画
    float offsetX{ 0.0f };
    float targetOffsetX{ 0.0f };
};

static inline float Lerp01(float a, float b, float t) { return a + (b - a) * t; }

inline void InitButton(Button& b, int tex, float x, float y) {
    b.texture = tex;
    b.x = x; b.y = y;
    b.w = 280.0f; b.h = 63.0f;
    b.scale = b.targetScale = 0.7f;  // ✅ 未选中 0.7
    b.offsetX = b.targetOffsetX = 0.0f;
}

inline void UpdateButton(Button& b, bool selected) {
    b.targetScale = selected ? 1.0f : 0.7f; // ✅ 选中 1.0
    b.scale = Lerp01(b.scale, b.targetScale, 0.25f); // ✅ Lerp 0.25
}

inline void DrawButton(const Button& b, unsigned int colorRGBA) {
    float scaledH = b.h * b.scale;
    int drawX = int(b.x + b.offsetX);
    int drawY = int(b.y - scaledH / 2.0f);

    Novice::DrawSprite(drawX, drawY, b.texture, b.scale, b.scale, 0.0f, colorRGBA);
}
