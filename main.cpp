#include <Novice.h>
#include <cmath>
#include <cstring>
#include "bg.h"
#include "button.h"
#include "title.h"
#include "Player.h"

const char kWindowTitle[] = "Run Game (Seamless Title -> Play)";

// 游戏状态（速度/逻辑）
enum GameState { READY, PLAY };
// UI 状态（叠加在 READY 上）
enum UIState { MENU, HOWTO };

static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

//—— Floor 相关 ——//
constexpr int FLOOR_H = 260;
constexpr float GROUND_Y = 720.0f - FLOOR_H;

// 障碍物结构
struct Obstacle {
    Corners pos;
    float w, h;
    bool hitX, hitY;
};

static inline Corners MakeCorners(float x, float y, float w, float h) {
    return { {x,y},{x + w,y},{x,y + h},{x + w,y + h} };
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256]{}, preKeys[256]{};

    // 贴图
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");
    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    int bgFloor = Novice::LoadTexture("./Resources/floor.png");

    // 背景
    Background bg;
    InitBackground(bg, bgFar, bgNear, bgFloor);

    // Title & Buttons
    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);
    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f);
    InitButton(btn[1], texHow, 850.0f, 450.0f);
    InitButton(btn[2], texExit, 850.0f, 500.0f);
    int selected = 1;

    // 玩家
    Player player;
    player.Init();
    player.Reset(225.0f, GROUND_Y - 25.0f);

    // 障碍物
    Obstacle obs{ {}, 50.0f, 100.0f, false, false };
    float obsX = 1000.0f;
    float obsY = GROUND_Y - obs.h;
    obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);

    bool isGameOver = false;

    float playTime = 0.0f;
    float scrollSpeed = 6.0f;
    const float maxScrollSpeed = 64.0f;
    const float baseAccel = 0.05f;
    const float gravity = 1.2f;
    const float jumpPower = -24.0f;

    const float STAGE1_FACTOR = 1.0f;
    const float STAGE2_FACTOR = 1.6f;
    const float STAGE3_FACTOR = 2.2f;
    const float STAGE2_SPEED = 15.0f;
    const float STAGE3_SPEED = 35.0f;

    float bgSpeedFactor = STAGE1_FACTOR;
    float targetBgFactor = STAGE1_FACTOR;
    const float BG_LERP = 0.15f;

    const float FLY_T = 0.18f;

    GameState gameState = READY;
    UIState uiState = MENU;

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        if (gameState == READY && uiState == MENU) {
            if (keys[DIK_W] && !preKeys[DIK_W]) selected = (selected + 2) % 3;
            if (keys[DIK_S] && !preKeys[DIK_S]) selected = (selected + 1) % 3;

            if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
                if (selected == 0) {
                    title.targetOffsetX = -800.0f;
                    for (int i = 0; i < 3; ++i) btn[i].targetOffsetX = +800.0f;
                    gameState = PLAY;
                    isGameOver = false;
                    player.Reset(225.0f, GROUND_Y - player.h / 2.0f);
                    obsX = 1000.0f;
                    obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);
                    playTime = 0.0f;
                    scrollSpeed = 6.0f;
                    targetBgFactor = STAGE1_FACTOR;
                }
                else if (selected == 1) {
                    title.targetOffsetX = -800.0f;
                    for (int i = 0; i < 3; ++i) btn[i].targetOffsetX = +800.0f;
                    uiState = HOWTO;
                }
                else if (selected == 2) {
                    Novice::Finalize();
                    return 0;
                }
            }
        }

        if (gameState == READY && uiState == HOWTO) {
            if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) {
                title.targetOffsetX = 0.0f;
                for (int i = 0; i < 3; ++i) btn[i].targetOffsetX = 0.0f;
                uiState = MENU;
            }
        }

        if (gameState == PLAY) {
            if (!isGameOver) {

                // ✅ 玩家模块化更新
                bool pressJump = keys[DIK_SPACE] && !preKeys[DIK_SPACE];
                player.Update(pressJump, gravity, jumpPower, GROUND_Y);

                // 障碍物移动
                scrollSpeed += baseAccel;
                if (scrollSpeed > maxScrollSpeed) scrollSpeed = maxScrollSpeed;

                obsX -= scrollSpeed;
                if (obsX + obs.w < 0.0f) {
                    obsX = float(1280 + rand() % 300);
                }
                obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);

                // ✅ 碰撞仍然完全使用 corners
                obs.hitX = (player.corners.rb.x >= obs.pos.lt.x && player.corners.lt.x <= obs.pos.rb.x);
                obs.hitY = (player.corners.lb.y >= obs.pos.lt.y && player.corners.lt.y <= obs.pos.lb.y);
                if (obs.hitX && obs.hitY) isGameOver = true;

                // 背景速度分段
                if (scrollSpeed < STAGE2_SPEED) targetBgFactor = STAGE1_FACTOR;
                else if (scrollSpeed < STAGE3_SPEED) targetBgFactor = STAGE2_FACTOR;
                else targetBgFactor = STAGE3_FACTOR;
            }
            else {
                if (keys[DIK_R] && !preKeys[DIK_R]) {
                    gameState = READY;
                    uiState = MENU;
                    title.targetOffsetX = 0.0f;
                    for (int i = 0; i < 3; ++i) btn[i].targetOffsetX = 0.0f;
                }
            }
        }

        bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, BG_LERP);
        UpdateBackground(bg, bgSpeedFactor);

        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);
        for (int i = 0; i < 3; ++i) {
            UpdateButton(btn[i], i == selected);
            btn[i].offsetX = Lerp(btn[i].offsetX, btn[i].targetOffsetX, FLY_T);
        }

        DrawBackground(bg);

        if (gameState == READY) {
            if (uiState == MENU) {
                DrawTitle(title);
                for (int i = 0; i < 3; ++i) {
                    DrawButton(btn[i], (i == selected) ? 0xF2FC32FF : 0xFFFFFFFF);
                }
            }
            else if (uiState == HOWTO) {
                const int panelW = 720;
                const int panelH = 420;
                const int panelX = (1280 - panelW) / 2;
                const int panelY = (720 - panelH) / 2;
                Novice::DrawSprite(panelX, panelY, texHowTo, 720.0f / 1280.0f, 420.0f / 720.0f, 0.0f, 0xFFFFFFFF);
                Novice::ScreenPrintf(40, 40, "ESC: Back to Menu");
            }
        }

        if (gameState == PLAY) {

            // ✅ 玩家动画绘制替代旧矩形
            player.Draw();

            // 障碍仍旧矩形绘制（未确认贴图）
            int white1x1 = Novice::LoadTexture("./NoviceResources/white1x1.png");
            Novice::DrawQuad(
                int(obs.pos.lt.x), int(obs.pos.lt.y),
                int(obs.pos.rt.x), int(obs.pos.rt.y),
                int(obs.pos.lb.x), int(obs.pos.lb.y),
                int(obs.pos.rb.x), int(obs.pos.rb.y),
                0, 0, int(obs.w), int(obs.h), white1x1, 0x000000FF
            );

            Novice::ScreenPrintf(40, 40, "SPACE Jump");
            Novice::ScreenPrintf(40, 60, "Speed: %.1f", scrollSpeed);

            if (isGameOver) {
                Novice::ScreenPrintf(540, 300, "GAME OVER!");
                Novice::ScreenPrintf(480, 340, "Press [R] Return Title");
            }
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
