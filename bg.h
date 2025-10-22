#pragma once
#include <Novice.h>

struct Background {
    int farTex;
    int nearTex;
    int floorTex;     // 🆕 新增地板层
    float farX;
    float nearX;
    float floorX;     // 🆕 floor 的滚动位置
    float farSpeed;
    float nearSpeed;
    float floorSpeed; // 🆕 floor 的滚动速度
};

// 初始化背景
inline void InitBackground(Background& bg, int farTex, int nearTex, int floorTex = -1) {
    bg.farTex = farTex;
    bg.nearTex = nearTex;
    bg.floorTex = floorTex;
    bg.farX = bg.nearX = bg.floorX = 0.0f;
    bg.farSpeed = 0.5f;
    bg.nearSpeed = 6.0f;
    bg.floorSpeed = 10.0f; 
}

// 更新滚动
inline void UpdateBackground(Background& bg) {
    bg.farX -= bg.farSpeed;
    bg.nearX -= bg.nearSpeed;
    bg.floorX -= bg.floorSpeed;

    if (bg.farX <= -1280)   bg.farX = 0;
    if (bg.nearX <= -1280)  bg.nearX = 0;
    if (bg.floorX <= -1280) bg.floorX = 0;
}

// 绘制背景（含 floor）
inline void DrawBackground(const Background& bg) {
    // 远景层
    Novice::DrawSprite(int(bg.farX), 0, bg.farTex, 1, 1, 0, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.farX + 1280), 0, bg.farTex, 1, 1, 0, 0xFFFFFFFF);

    // 近景层
    Novice::DrawSprite(int(bg.nearX), 0, bg.nearTex, 1, 1, 0, 0xFFFFFFFF);
    Novice::DrawSprite(int(bg.nearX + 1280), 0, bg.nearTex, 1, 1, 0, 0xFFFFFFFF);

    // 🆕 地板层（Y固定在720-260）
    if (bg.floorTex != -1) {
        const int floorY = 720 - 260;
        Novice::DrawSprite(int(bg.floorX), floorY, bg.floorTex, 1, 1, 0, 0xFFFFFFFF);
        Novice::DrawSprite(int(bg.floorX + 1280), floorY, bg.floorTex, 1, 1, 0, 0xFFFFFFFF);
    }
}
