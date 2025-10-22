#include <Novice.h>
#include <cmath>
#include <cstring> // memcpy 需要这个头文件

const char kWindowTitle[] = "タイトル付きメニュー（タイトル中心拡大 + ボタン右拡大 + 選択色変更）";

static inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

struct Button {
    float x, y;          // 左对齐位置
    float w, h;          // 原始宽高
    float scale;         // 当前缩放
    float targetScale;   // 目标缩放
    int texture;         // 贴图句柄
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256]{}, preKeys[256]{};

    //--------------------------------
    // 背景加载
    //--------------------------------
    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    float bgFarX = 0.0f;
    float bgNearX = 0.0f;
    const float FAR_SPEED = 0.5f;
    const float NEAR_SPEED = 2.0f;

    //--------------------------------
    // 按钮参数
    //--------------------------------
    const float BASE_W = 280.0f;
    const float BASE_H = 63.0f;

    //--------------------------------
    // 贴图加载
    //--------------------------------
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHowTo = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");

    //--------------------------------
    // 初始化按钮
    //--------------------------------
    Button btn[3] = {
        {850.0f, 400.0f, BASE_W, BASE_H, 0.7f, 0.7f, texStart},
        {850.0f, 450.0f, BASE_W, BASE_H, 1.0f, 1.0f, texHowTo},
        {850.0f, 500.0f, BASE_W, BASE_H, 0.7f, 0.7f, texExit}
    };

    int selected = 1;
    float time = 0.0f;

    //--------------------------------
    // 主循环
    //--------------------------------
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //--------------------------------
        // 输入处理
        //--------------------------------
        if (keys[DIK_W] && !preKeys[DIK_W]) selected = (selected + 2) % 3;
        if (keys[DIK_S] && !preKeys[DIK_S]) selected = (selected + 1) % 3;

        //--------------------------------
        // 背景更新（循环滚动）
        //--------------------------------
        bgFarX -= FAR_SPEED;
        bgNearX -= NEAR_SPEED;
        if (bgFarX <= -1280) bgFarX = 0;
        if (bgNearX <= -1280) bgNearX = 0;

        //--------------------------------
        // 按钮缩放更新
        //--------------------------------
        for (int i = 0; i < 3; i++) {
            btn[i].targetScale = (i == selected) ? 1.0f : 0.7f;
            btn[i].scale = Lerp(btn[i].scale, btn[i].targetScale, 0.25f);
        }

        //--------------------------------
        // 标题波动动画
        //--------------------------------
        time += 0.30f;
        const float AMP_SCALE = 0.04f;
        const float AMP_FLOAT = 3.0f;
        float titleScale = 1.0f + AMP_SCALE * sinf(time);
        float titleY = 100.0f + AMP_FLOAT * sinf(time + 1.57f);
        int titleX = 120;

        //--------------------------------
        // 背景绘制
        //--------------------------------
        Novice::DrawSprite(int(bgFarX), 0, bgFar, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
        Novice::DrawSprite(int(bgFarX + 1280), 0, bgFar, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
        Novice::DrawSprite(int(bgNearX), 0, bgNear, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
        Novice::DrawSprite(int(bgNearX + 1280), 0, bgNear, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);

        //--------------------------------
        // 绘制标题（中心放大）
        //--------------------------------
        const float TITLE_W = 600.0f;
        const float TITLE_H = 200.0f;
        int titleDrawX = int(titleX - (TITLE_W * (titleScale - 1) / 2));
        int titleDrawY = int(titleY - (TITLE_H * (titleScale - 1) / 2));
        Novice::DrawSprite(titleDrawX, titleDrawY, texTitle, titleScale, titleScale, 0.0f, 0xFFFFFFFF);

        //--------------------------------
        // 绘制按钮（选中变色）
        //--------------------------------
        for (int i = 0; i < 3; i++) {
            float scaledH = btn[i].h * btn[i].scale;
            int btnDrawX = int(btn[i].x);
            int btnDrawY = int(btn[i].y - scaledH / 2);

            // 🎨 颜色定义
            unsigned int unselectedColor = 0xFFF2FC32; 
            unsigned int selectedColor = 0xF2FC32FF;

            unsigned int color = (i == selected) ? selectedColor : unselectedColor;

            Novice::DrawSprite(
                btnDrawX,
                btnDrawY,
                btn[i].texture,
                btn[i].scale,
                btn[i].scale,
                0.0f,
                color
            );
        }

        //--------------------------------
        // 调试显示
        //--------------------------------
        Novice::ScreenPrintf(40, 50, "選択中: %d", selected);
        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
