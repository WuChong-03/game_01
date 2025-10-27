#include <Novice.h>
#include <cmath>
#include <cstring>
#include "bg.h"
#include "button.h"
#include "title.h"
#include "Player.h"

const char kWindowTitle[] = "Run Game (Seamless Title -> Play)";

enum GameState { READY, PLAY };
enum UIState { MENU, HOWTO };

static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

constexpr int FLOOR_H = 260;
constexpr float GROUND_Y = 720.0f - FLOOR_H;

//======================
// ✅ 音量常量
//======================
constexpr float VOLUME_BGM = 0.45f;
constexpr float VOLUME_UI_MOVE = 0.80f;
constexpr float VOLUME_UI_START = 1.0f;
constexpr float VOLUME_JUMP = 0.80f;
constexpr float VOLUME_LAND = 0.60f;

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

    //--------------------------------------
    // 音频加载
    //--------------------------------------
    int seBgm = Novice::LoadAudio("./Resources/sound/bgm_gameplay.wav");
    int seJumpUp = Novice::LoadAudio("./Resources/sound/jump_start.wav");
    int seLand = Novice::LoadAudio("./Resources/sound/jump_land.wav");
    int seUiMove = Novice::LoadAudio("./Resources/sound/ui_button.wav");
    int seUiStart = Novice::LoadAudio("./Resources/sound/ui_start.wav");
    int playBgmHandle = -1;

    auto PlayBGM = [&](int audio) {
        if (playBgmHandle != -1) Novice::StopAudio(playBgmHandle);
        playBgmHandle = Novice::PlayAudio(audio, true, VOLUME_BGM);
        };

    //--------------------------------------
    // 纹理加载
    //--------------------------------------
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");

    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    int bgFloor = Novice::LoadTexture("./Resources/floor.png");

    int white1x1 = Novice::LoadTexture("./NoviceResources/white1x1.png");

    //--------------------------------------
    // 背景 & UI 初始化
    //--------------------------------------
    Background bg;
    InitBackground(bg, bgFar, bgNear, bgFloor);

    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);

    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f);
    InitButton(btn[1], texHow, 850.0f, 450.0f);
    InitButton(btn[2], texExit, 850.0f, 500.0f);
    int selected = 1;

    //--------------------------------------
    // 玩家
    //--------------------------------------
    Player player;
    player.Init();
    player.Reset(225.0f, GROUND_Y - 25.0f);

    //--------------------------------------
    // 障碍物初始化
    //--------------------------------------
    Obstacle obs{ {}, 50.0f, 100.0f, false, false };
    float obsX = 1000.0f;
    float obsY = GROUND_Y - obs.h;
    obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);

    //--------------------------------------
    // 游戏变量
    //--------------------------------------
    bool isGameOver = false;
    float scrollSpeed = 6.0f;
    const float maxScrollSpeed = 64.0f;
    const float baseAccel = 0.05f;

    const float gravity = 1.2f;
    const float jumpPower = -24.0f;

    float bgSpeedFactor = 1.0f;
    float targetBgFactor = 1.0f;
    const float BG_LERP = 0.15f;
    const float FLY_T = 0.18f;

    GameState gameState = READY;
    UIState uiState = MENU;

    //--------------------------------------
    // 主循环
    //--------------------------------------
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //--------------------------------------
        // MENU：背景动，但游戏不更新
        //--------------------------------------
        if (gameState == READY && uiState == MENU) {

            if (!Novice::IsPlayingAudio(playBgmHandle)) PlayBGM(seBgm);

            if (keys[DIK_W] && !preKeys[DIK_W]) { selected = (selected + 2) % 3; Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE); }
            if (keys[DIK_S] && !preKeys[DIK_S]) { selected = (selected + 1) % 3; Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE); }

            if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);

                if (selected == 0) {
                    Novice::PlayAudio(seUiStart, false, VOLUME_UI_START);
                    gameState = PLAY;
                    isGameOver = false;
                    title.targetOffsetX = -800.0f;
                    for (int i = 0;i < 3;i++) btn[i].targetOffsetX = +800.0f;
                    PlayBGM(seBgm);

                    player.Reset(225.0f, GROUND_Y - player.h / 2);
                    scrollSpeed = 6.0f;
                    targetBgFactor = 1.0f;
                }
                else if (selected == 1) uiState = HOWTO;
                else if (selected == 2) { Novice::Finalize(); return 0; }
            }
        }

        //--------------------------------------
        // PLAY 游戏进行中
        //--------------------------------------
        if (gameState == PLAY) {

            bool pressJump = (!isGameOver && keys[DIK_SPACE] && !preKeys[DIK_SPACE]);

            if (!isGameOver) {

                // 玩家更新
                if (pressJump && !player.isJumping)
                    Novice::PlayAudio(seJumpUp, false, VOLUME_JUMP);

                bool wasAir = player.isJumping;
                player.Update(pressJump, gravity, jumpPower, GROUND_Y);
                if (wasAir && !player.isJumping)
                    Novice::PlayAudio(seLand, false, VOLUME_LAND);

                // 滚动更新
                scrollSpeed += baseAccel;
                if (scrollSpeed > maxScrollSpeed)scrollSpeed = maxScrollSpeed;

                obsX -= scrollSpeed;
                if (obsX + obs.w < 0) obsX = float(1280 + rand() % 300);
                obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);

                // 碰撞判定
                obs.hitX = (player.corners.rb.x >= obs.pos.lt.x &&
                    player.corners.lt.x <= obs.pos.rb.x);
                obs.hitY = (player.corners.lb.y >= obs.pos.lt.y &&
                    player.corners.lt.y <= obs.pos.lb.y);

                if (obs.hitX && obs.hitY) {
                    Novice::StopAudio(playBgmHandle);
                    playBgmHandle = -1;
                    isGameOver = true;
                }
            }

            //--------------------------------------
            // ✅死亡等待：仅允许按R重开
            //--------------------------------------
            if (isGameOver && keys[DIK_R] && !preKeys[DIK_R]) {
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);

                gameState = READY;
                uiState = MENU;

                title.targetOffsetX = 0;
                for (int i = 0;i < 3;i++) btn[i].targetOffsetX = 0;

                player.Reset(225.0f, GROUND_Y - player.h / 2);
                scrollSpeed = 6.0f;
                bgSpeedFactor = 1.0f;
                targetBgFactor = 1.0f;

                obsX = 1000.0f;
                obs.pos = MakeCorners(obsX, obsY, obs.w, obs.h);

                isGameOver = false;
                PlayBGM(seBgm);
            }
        }

        //--------------------------------------
        // ✅背景更新范围
        //--------------------------------------
        if (!isGameOver) {
            bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, BG_LERP);
            UpdateBackground(bg, bgSpeedFactor);
        }

        //--------------------------------------
        // UI动画
        //--------------------------------------
        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);

        for (int i = 0;i < 3;i++) {
            UpdateButton(btn[i], i == selected);
            btn[i].offsetX = Lerp(btn[i].offsetX, btn[i].targetOffsetX, FLY_T);
        }

        //--------------------------------------
        // 绘制
        //--------------------------------------
        DrawBackground(bg);

        if (gameState == READY) {
            if (uiState == MENU) {
                DrawTitle(title);
                for (int i = 0;i < 3;i++)
                    DrawButton(btn[i], (i == selected) ? 0xF2FC32FF : 0xFFFFFFFF);
            }
            else if (uiState == HOWTO) {
                Novice::DrawSprite(280, 150, texHowTo, 1, 1, 0, 0xFFFFFFFF);
                if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) uiState = MENU;
            }
        }

        if (gameState == PLAY) {
            player.Draw();

            // 障碍绘制
            Novice::DrawQuad(
                int(obs.pos.lt.x), int(obs.pos.lt.y),
                int(obs.pos.rt.x), int(obs.pos.rt.y),
                int(obs.pos.lb.x), int(obs.pos.lb.y),
                int(obs.pos.rb.x), int(obs.pos.rb.y),
                0, 0, int(obs.w), int(obs.h),
                white1x1, 0x000000FF
            );

            if (isGameOver) {
                Novice::ScreenPrintf(540, 300, "GAME OVER!");
                Novice::ScreenPrintf(480, 340, "Press [R]");
            }
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
