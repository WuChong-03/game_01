#pragma once
#include <Novice.h>

struct Background {
    int farTex;
    int nearTex;
    float farX;
    float nearX;
    float farSpeed;
    float nearSpeed;
};

// 初始化背景
inline void InitBackground(Background& bg, int farTex, int nearTex) {
    bg.farTex = farTex;
    bg.nearTex = nearTex;
    bg.farX = 0.0f;
    bg.nearX = 0.0f;
    bg.farSpeed = 0.5f;
    bg.nearSpeed = 10.0f;
}

// 更新滚动
inline void UpdateBackground(Background& bg) {
    bg.farX -= bg.farSpeed;
    bg.nearX -= bg.nearSpeed;
    if (bg.farX <= -1280) bg.farX = 0;
    if (bg.nearX <= -1280) bg.nearX = 0;
}

// 绘制背景
inline void DrawBackground(const Background& bg) {
    Novice::DrawSprite(int(bg.farX), 0, bg.farTex, 1, 1, 0, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.farX + 1280), 0, bg.farTex, 1, 1, 0, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.nearX), 0, bg.nearTex, 1, 1, 0, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.nearX + 1280), 0, bg.nearTex, 1, 1, 0, 0xFFFFFFFF);
}
