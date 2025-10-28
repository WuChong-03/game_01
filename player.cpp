#include "Player.h"
#include <cmath>
#include <cstdio>

// 计算矩形四角（碰撞调试/预留）
static inline Corners MakeCorners(float x, float y, float w, float h) {
    Corners c;
    c.lt = { x,         y };
    c.rt = { x + w,     y };
    c.lb = { x,     y + h };
    c.rb = { x + w, y + h };
    return c;
}

void Player::Init() {

    // 玩家碰撞体大小（逻辑碰撞盒）
    w = 50.0f;
    h = 50.0f;

    vy = 0.0f;
    isJumping = false;

    // 载入跑步帧
    for (int i = 0; i < 6; i++) {
        char file[128];
        sprintf_s(file, "./Resources/player/run_%02d.png", i);
        runTex[i] = Novice::LoadTexture(file);
    }

    // 载入跳跃帧
    for (int i = 0; i < 4; i++) {
        char file[128];
        sprintf_s(file, "./Resources/player/jump_%02d.png", i);
        jumpTex[i] = Novice::LoadTexture(file);
    }

    animFrame = 0;
    animTimer = 0.0f;

    // ★ 默认动画速度（可以被主循环覆盖）
    // 跑步 ~20fps，跳跃 ~6fps
    runFrameTime = 0.05f;
    jumpFrameTime = 0.16f;

    // 初始中心
    center = { 225.0f, 600.0f };
    corners = MakeCorners(center.x - w / 2.0f, center.y - h / 2.0f, w, h);
}

void Player::Reset(float startX, float startY) {
    center.x = startX;
    center.y = startY;
    vy = 0.0f;
    isJumping = false;

    animFrame = 0;
    animTimer = 0.0f;

    corners = MakeCorners(center.x - w / 2.0f, center.y - h / 2.0f, w, h);
}

void Player::Update(bool spacePressed, float gravity, float jumpPower, float groundY) {

    // 跳跃输入（只有在地面上才允许起跳）
    if (!isJumping && spacePressed) {
        vy = jumpPower;     // 通常是负的，往上冲
        isJumping = true;
    }

    // 重力
    vy += gravity;

    // 垂直位移
    center.y += vy;

    // 如果传进了有效的地面高度 (比如主循环给你真实地面y)：
    // 玩家脚 = center.y + h/2
    // 把玩家吸到地面并认为落地
    float footY = center.y + h * 0.5f;
    if (footY > groundY) {
        center.y = groundY - h * 0.5f;
        vy = 0.0f;
        isJumping = false;
    }

    // 更新碰撞盒
    corners = MakeCorners(center.x - w / 2.0f, center.y - h / 2.0f, w, h);

    // ---------------------------------
    // 动画更新
    // ---------------------------------
    // 固定帧率假设：60fps -> 每帧大约1/60秒
    const float dt = 1.0f / 60.0f;
    animTimer += dt;

    if (isJumping) {
        // 跳跃动画
        if (animTimer >= jumpFrameTime) {
            animTimer = 0.0f;
            animFrame++;
            if (animFrame >= 4) {
                animFrame = 3; // 跳跃就停在最后一帧，不循环
            }
        }
    }
    else {
        // 跑步动画
        if (animTimer >= runFrameTime) {
            animTimer = 0.0f;
            animFrame++;
            if (animFrame >= 6) {
                animFrame = 0;
            }
        }
    }
}

void Player::Draw() {

    // 我们绘制用的sprite是64x64像素风格
    // （如果你的素材不是64x64，可以改成合适的渲染尺寸）
    float drawX = center.x - 32.0f;
    float drawY = center.y - 32.0f;

    int tex = 0;
    if (isJumping) {
        int idx = animFrame;
        if (idx < 0) idx = 0;
        if (idx > 3) idx = 3;
        tex = jumpTex[idx];
    }
    else {
        int idx = animFrame % 6;
        if (idx < 0) idx = 0;
        tex = runTex[idx];
    }

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
