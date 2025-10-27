#pragma once
#include <Novice.h>
#include <cmath>

struct Title {
    int tex;
    float x, y;
    float baseX, baseY;    // 原始坐标
    float scale;
    float time;
    float offsetX;
    float targetOffsetX;
    float tiltDeg;         // 倾斜角（度）
};

inline void InitTitle(Title& t, int tex, float x, float y) {
    t.tex = tex;
    t.baseX = t.x = x;
    t.baseY = t.y = y;
    t.scale = 1.0f;
    t.time = 0.0f;
    t.offsetX = 0.0f;
    t.targetOffsetX = 0.0f;
    t.tiltDeg = -5.0f; // ✅ 微微左倾（保持你的设定）
}

inline void UpdateTitle(Title& t) {
    // ✅ 呼吸 + 浮动（保持你的设定）
    t.time += 0.20f;
    t.scale = 1.0f + 0.05f * sinf(t.time);
    t.y = t.baseY + 5.0f * sinf(t.time);
}

inline void DrawTitle(const Title& t) {
    // 以中心放大：用逻辑尺寸修正绘制起点
    const float TITLE_W = 600.0f; // 你根据贴图实际宽高调
    const float TITLE_H = 200.0f;

    int drawX = (int)(t.x + t.offsetX - (TITLE_W * (t.scale - 1) / 2));
    int drawY = (int)(t.y - (TITLE_H * (t.scale - 1) / 2));

    // 角度（弧度）
    float rad = t.tiltDeg * (3.1415926f / 180.0f);

    Novice::DrawSprite(drawX, drawY, t.tex, t.scale, t.scale, rad, 0xFFFFFFFF);
}
