#include "Player.h"
#include <cmath>
#include <cstdio>

static inline Corners MakeCorners(float x, float y, float w, float h) {
    return { {x,y},{x + w,y},{x,y + h},{x + w,y + h} };
}

void Player::Init() {

    w = 50.0f;
    h = 50.0f;
    vy = 0.0f;
    isJumping = false;

    // 加载跑步贴图
    for (int i = 0; i < 6; i++) {
        char file[128];
        sprintf_s(file, "./Resources/player/run_%02d.png", i);
        runTex[i] = Novice::LoadTexture(file);
    }
    // 加载跳跃贴图
    for (int i = 0; i < 4; i++) {
        char file[128];
        sprintf_s(file, "./Resources/player/jump_%02d.png", i);
        jumpTex[i] = Novice::LoadTexture(file);
    }

    animFrame = 0;
    animTimer = 0.0f;
}

void Player::Reset(float startX, float startY) {
    center = { startX, startY };
    vy = 0.0f;
    isJumping = false;
    animFrame = 0;
    animTimer = 0.0f;
    corners = MakeCorners(center.x - w / 2, center.y - h / 2, w, h);
}

void Player::Update(bool spacePressed, float gravity, float jumpPower, float groundY) {

    // 起跳
    if (spacePressed && !isJumping) {
        vy = jumpPower;
        isJumping = true;
        animFrame = 0;
    }

    // 重力
    vy += gravity;
    center.y += vy;

    // 落地
    if (center.y + h / 2 >= groundY) {
        center.y = groundY - h / 2;
        vy = 0.0f;
        if (isJumping) {
            isJumping = false;
            animFrame = 0;
        }
    }

    // 更新外接矩形
    corners = MakeCorners(center.x - w / 2, center.y - h / 2, w, h);

    // ---- 动画 ----
    animTimer += 1.0f / 60.0f;

    if (!isJumping) {
        if (animTimer >= runFrameTime) {
            animTimer = 0.0f;
            animFrame = (animFrame + 1) % 6;
        }
    }
    else {
        if (animTimer >= jumpFrameTime) {
            animTimer = 0.0f;
            if (animFrame < 3) animFrame++;
        }
    }
}

void Player::Draw() {

    int tex = isJumping ? jumpTex[animFrame] : runTex[animFrame];

    // ✅ 按中心绘制，使用原始贴图尺寸128x128
    float drawX = center.x - 64.0f / 2;
    float drawY = center.y - 64.0f / 2;

    Novice::DrawSprite(
        int(drawX),
        int(drawY),
        tex,
        1.0f,
        1.0f,
        0.0f,
        0xFFFFFFFF
    );
}

