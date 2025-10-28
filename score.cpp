// Score.cpp
#include "Score.h"
#include <cmath>
#include <cstdio>

// 线性插值小工具（和你主代码里的Lerp一样，可重复写一份）
static inline float LerpF(float a, float b, float t) {
    return a + (b - a) * t;
}

void ScoreManager::Init() {
    score = 0;
    shownScore = 0.0f;
    popTimer = 0.0f;

    // 载入位图数字
    // 假设你准备一张 ./Resources/font_digits.png
    // 里面是十个数字横向排列：0 1 2 3 4 5 6 7 8 9
    texDigits = Novice::LoadTexture("./Resources/number/num.png");

    digitW = 44; // 这一格一个数字的宽
    digitH = 80; // 一格的高
    // ↑ 这两个要跟你的字体图实际大小对齐
}

void ScoreManager::Update(float distanceRun) {
    // 计算新的分数：距离 -> 米 -> 分数
    int newScore = (int)(distanceRun / 10.0f); // 10px = 1m = 1分

    if (newScore > score) {
        // 分数提高了
        score = newScore;
        popTimer = 1.0f; // 开始一轮"弹一下"动画
    }

    // 让显示的数字追赶真实分数（柔和）
    shownScore = LerpF(shownScore, (float)score, 0.3f);

    // 衰减pop效果
    if (popTimer > 0.0f) {
        popTimer -= 0.1f;
        if (popTimer < 0.0f) {
            popTimer = 0.0f;
        }
    }
}

void ScoreManager::Draw(int x, int y) {
    // 做缩放弹动画：popTimer越大，scale越大
    // popTimer=1.0f -> scale=1.2
    // popTimer=0.0f -> scale=1.0
    float scale = 1.0f + popTimer * 0.2f;

    // 我们要显示的分数 = 四舍五入后的 shownScore
    int displayScore = (int)std::round(shownScore);

    // 把这个分数转成字符串，一位一位画
    char buf[32];
    sprintf_s(buf, sizeof(buf), "%d", displayScore);

    // 逐字符绘制
    // 数字纹理的切片规则：
    // '0' 在x=0
    // '1' 在x=digitW
    // '2' 在x=digitW*2
    // ...
    for (int i = 0; buf[i] != '\0'; i++) {
        char c = buf[i];
        if (c < '0' || c > '9') { continue; }

        int digit = c - '0';
        int srcX = digit * digitW;
        int srcY = 0;

        int drawX = x + (int)(i * digitW * scale);
        int drawY = y;

        Novice::DrawQuad(
            drawX,
            drawY,
            drawX + (int)(digitW * scale),
            drawY,
            drawX,
            drawY + (int)(digitH * scale),
            drawX + (int)(digitW * scale),
            drawY + (int)(digitH * scale),
            srcX, srcY,
            digitW, digitH,
            texDigits,
            0xFFFFFFFF
        );
    }

    // 你也可以在分数旁边再画一个小文字，比如 "m" 或 "SCORE"
    // 用一张小的位图写"m"或"SCORE"，同样DrawQuad就行
}
