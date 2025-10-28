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

    // ---- 动画贴图 ----
    // 假设跑步6帧 run_00.png ~ run_05.png
    // 假设跳跃4帧 jump_00.png ~ jump_03.png
    int runTex[6];
    int jumpTex[4];

    int   animFrame;     // 当前播放到第几帧
    float animTimer;     // 计时器(秒)

    // ★ 动画帧间隔（秒/帧），可变
    float runFrameTime;   // 跑步动画一帧持续多久
    float jumpFrameTime;  // 跳跃动画一帧持续多久

    // ---- 方法 ----
    void Init();
    void Reset(float startX, float startY);

    // spacePressed: 本帧是否按下跳跃键
    // gravity:      重力加速度/每帧
    // jumpPower:    跳跃初速度(负值向上)
    // groundY:      如果传进实际地面的y，就会自动贴地
    //               如果传很大数(2000)，就不会自动吸到地面
    void Update(bool spacePressed, float gravity, float jumpPower, float groundY);

    void Draw();

    // ★ 由主循环调用，按阶段切换玩家动画速度
    inline void SetAnimSpeeds(float runSecPerFrame, float jumpSecPerFrame) {
        runFrameTime = runSecPerFrame;
        jumpFrameTime = jumpSecPerFrame;
    }
};
