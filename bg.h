#pragma once
#include <Novice.h>

// 背景两层：far / near（连续滚动），速度乘以一个 factor
struct Background {
    int   farTex{ -1 };
    int   nearTex{ -1 };

    float farX{ 0.0f };
    float nearX{ 0.0f };

    // ✅ 初始速度（保留你原先的数值风格）
    float farSpeed{ 5.0f };
    float nearSpeed{ 8.0f };
};

inline void InitBackground(Background& bg, int farTex, int nearTex) {
    bg.farTex = farTex;
    bg.nearTex = nearTex;
    bg.farX = bg.nearX = 0.0f;
}

inline void UpdateBackground(Background& bg, float speedFactor) {
    // 一直滚动：速度 * speedFactor
    bg.farX -= bg.farSpeed * speedFactor;
    bg.nearX -= bg.nearSpeed * speedFactor;

    // 无缝循环（贴图宽=1280）
    if (bg.farX <= -1280) bg.farX += 1280;
    if (bg.nearX <= -1280) bg.nearX += 1280;
}

inline void DrawBackground(const Background& bg) {
    // Far
    Novice::DrawSprite(int(bg.farX), 0, bg.farTex, 1, 1, 0.0f, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.farX + 1280), 0, bg.farTex, 1, 1, 0.0f, 0xFFFFFFFF);

    // Near
    Novice::DrawSprite(int(bg.nearX), 0, bg.nearTex, 1, 1, 0.0f, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.nearX + 1280), 0, bg.nearTex, 1, 1, 0.0f, 0xFFFFFFFF);
}
