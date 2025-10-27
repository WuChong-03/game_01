#pragma once
#include <Novice.h>

// 背景三层：far / near / floor（一直滚动），速度乘以一个 factor
struct Background {
    int farTex{ -1 };
    int nearTex{ -1 };
    int floorTex{ -1 };

    float farX{ 0.0f };
    float nearX{ 0.0f };
    float floorX{ 0.0f };

    // ✅ 初始速度（锁定你要求的）
    float farSpeed{ 5.0f };
    float nearSpeed{ 8.0f };
    float floorSpeed{ 10.0f };
};

inline void InitBackground(Background& bg, int farTex, int nearTex, int floorTex) {
    bg.farTex = farTex;
    bg.nearTex = nearTex;
    bg.floorTex = floorTex;
    bg.farX = bg.nearX = bg.floorX = 0.0f;
}

inline void UpdateBackground(Background& bg, float speedFactor) {
    // 一直滚动：速度 * speedFactor（不改基础速度）
    bg.farX -= bg.farSpeed * speedFactor;
    bg.nearX -= bg.nearSpeed * speedFactor;
    bg.floorX -= bg.floorSpeed * speedFactor;

    // 无缝循环（贴图宽=1280）
    if (bg.farX <= -1280) bg.farX += 1280;
    if (bg.nearX <= -1280) bg.nearX += 1280;
    if (bg.floorX <= -1280) bg.floorX += 1280;
}

inline void DrawBackground(const Background& bg) {
    // Far
    Novice::DrawSprite(int(bg.farX), 0, bg.farTex, 1, 1, 0.0f, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.farX + 1280), 0, bg.farTex, 1, 1, 0.0f, 0xFFFFFFFF);

    // Near
    Novice::DrawSprite(int(bg.nearX), 0, bg.nearTex, 1, 1, 0.0f, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.nearX + 1280), 0, bg.nearTex, 1, 1, 0.0f, 0xFFFFFFFF);

    // Floor（固定Y=720-260）
    const int floorY = 720 - 260;
    Novice::DrawSprite(int(bg.floorX), floorY, bg.floorTex, 1, 1, 0.0f, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.floorX + 1280), floorY, bg.floorTex, 1, 1, 0.0f, 0xFFFFFFFF);
}
