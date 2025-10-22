#pragma once
#include <Novice.h>
#include <cmath>

struct Title {
    int texture;
    float x, y;
    float baseY;
    float time;
    float scale;
};

inline void InitTitle(Title& title, int tex, float x, float y) {
    title.texture = tex;
    title.x = x;
    title.y = y;
    title.baseY = y;
    title.time = 0.0f;
    title.scale = 1.0f;
}

inline void UpdateTitle(Title& title) {
    title.time += 0.3f;
    title.scale = 1.0f + 0.04f * sinf(title.time);
    title.y = title.baseY + 3.0f * sinf(title.time + 1.57f);
}

inline void DrawTitle(const Title& title) {
    const float TITLE_W = 600.0f;
    const float TITLE_H = 200.0f;
    int drawX = static_cast<int>(title.x - (TITLE_W * (title.scale - 1) / 2));
    int drawY = static_cast<int>(title.y - (TITLE_H * (title.scale - 1) / 2));

    // 🔹 往左倾斜（-5度）
    const float tiltAngle = -10.0f * (3.14159265f / 180.0f);

    Novice::DrawSprite(
        drawX,
        drawY,
        title.texture,
        title.scale,
        title.scale,
        tiltAngle,  // ← 加上旋转
        0xFFFFFFFF
    );
}



