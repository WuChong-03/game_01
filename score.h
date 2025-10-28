// Score.h
#pragma once
#include <Novice.h>

struct ScoreManager {
    // 实际分数（整数）
    int score;

    // 用来显示的分数（float，做平滑/弹跳）
    float shownScore;

    // 用来控制缩放动画的计时器
    float popTimer;

    // 位图数字纹理
    // 假设我们做一张包含0~9排成一行的图，比如 digits.png
    int texDigits;

    // 每个数字的宽高（像素）
    int digitW;
    int digitH;

    // 初始化
    void Init();

    // 根据 distanceRun 更新分数
    void Update(float distanceRun);

    // 绘制分数（屏幕坐标 x,y）
    void Draw(int x, int y);
};
