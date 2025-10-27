#pragma once
#include <Novice.h>

struct Vector2 {
    float x, y;
};

struct Corners {
    Vector2 lt, rt, lb, rb;
};

struct Player {

    // ---- 玩家核心数据 ----
    Vector2 center;
    Corners corners;

    float vy;
    float w, h;
    bool isJumping;

    // ---- 动画控制 ----
    int runTex[6];
    int jumpTex[4];

    int animFrame;
    float animTimer;
    const float runFrameTime = 0.05f;   // 跑步 10fps
    const float jumpFrameTime = 0.16f; // 跳跃 6fps

    // ---- 方法 ----
    void Init();
    void Reset(float startX, float startY);
    void Update(bool spacePressed, float gravity, float jumpPower, float groundY);
    void Draw();
};
