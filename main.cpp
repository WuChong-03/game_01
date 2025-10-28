#include <Novice.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include "bg.h"
#include "button.h"
#include "title.h"
#include "Player.h"

const char kWindowTitle[] = "Run Game (段段地面单文件版)";

enum GameState { READY, PLAY };
enum UIState { MENU, HOWTO };

static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

//======================
// 常量设置（保持不变）
//======================
constexpr float VOLUME_BGM = 0.45f;
constexpr float VOLUME_UI_MOVE = 0.8f;
constexpr float VOLUME_UI_START = 2.0f;
constexpr float VOLUME_JUMP = 0.8f;
constexpr float VOLUME_LAND = 0.6f;

//======================
// 段段地面参数（保持不变）
//======================
constexpr float kBlockWidth = 128.0f;
constexpr float kBlockHeight = 128.0f;
constexpr int   kBlocksPerSeg = 5;
constexpr int   kNumSegments = 4;

const float kHeights[] = { 400.0f, 500.0f, 600.0f, 700.0f };
const int   kNumHeights = (int)(sizeof(kHeights) / sizeof(kHeights[0]));

//======================
// 段段地面：内联在本文件
//======================
struct Ground {
    float x;
    float y;
    int   blockCount;
    bool  isGround;
};

// 初始化地面
static inline void InitGrounds(
    Ground* grounds, int num, float /*startY*/,
    float blockW, int blocksPerSeg,
    const float* heights, int nHeights
) {
    for (int i = 0; i < num; i++) {
        grounds[i].x = i * blockW * (blocksPerSeg + 2);
        // 初始高度：循环使用高度表，简单稳定
        grounds[i].y = heights[i % nHeights];
        grounds[i].blockCount = blocksPerSeg;
        grounds[i].isGround = true;
    }
}

// 将一段地面移动到右端并随机高度（避免太接近）
static inline void RecycleOneGround(
    Ground& g, const Ground* grounds, int num,
    float blockW, int blocksPerSeg,
    const float* heights, int nHeights,
    float minDY = 120.0f
) {
    float maxX = grounds[0].x;
    for (int j = 1; j < num; j++) {
        if (grounds[j].x > maxX) maxX = grounds[j].x;
    }
    g.x = maxX + blockW * (blocksPerSeg + 2);

    float newY;
    do {
        newY = heights[std::rand() % nHeights];
    } while (std::fabs(newY - g.y) < minDY);
    g.y = newY;
}

// 更新地面（左移+循环）
static inline void UpdateGrounds(
    Ground* grounds, int num,
    float scrollSpeed,
    float blockW, int blocksPerSeg,
    const float* heights, int nHeights
) {
    for (int i = 0; i < num; i++) {
        grounds[i].x -= scrollSpeed;
        if (grounds[i].x + blockW * blocksPerSeg < 0) {
            RecycleOneGround(grounds[i], grounds, num, blockW, blocksPerSeg, heights, nHeights);
        }
    }
}

// 绘制：从每段顶 y 拉到底部
static inline void DrawGrounds(
    const Ground* grounds, int num,
    int groundTex,
    float blockW, float blockH,
    int screenH
) {
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {
            float tileX = grounds[i].x + j * blockW;
            float tileY = grounds[i].y;

            Novice::DrawQuad(
                (int)tileX, (int)tileY,                 // 左上
                (int)(tileX + blockW), (int)tileY,      // 右上
                (int)tileX, screenH,                    // 左下
                (int)(tileX + blockW), screenH,         // 右下
                0, 0, (int)blockW, (int)blockH,
                groundTex, 0xFFFFFFFF
            );
        }
    }
}

// 查询玩家脚下地面的 y 值（找不到则返回很大值→继续下落）
static inline float QueryGroundYAtX(
    const Ground* grounds, int num,
    float px, float blockW
) {
    for (int i = 0; i < num; i++) {
        float segL = grounds[i].x;
        float segR = segL + grounds[i].blockCount * blockW;
        if (px >= segL && px <= segR) {
            return grounds[i].y;
        }
    }
    return 2000.0f;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, SCREEN_W, SCREEN_H);
    char keys[256]{}, preKeys[256]{};
    std::srand(unsigned(std::time(nullptr)));

    //--------------------------------------
    // 音频（保持不变）
    //--------------------------------------
    int seBgm = Novice::LoadAudio("./Resources/sound/bgm_gameplay.wav");
    int seJump = Novice::LoadAudio("./Resources/sound/jump_start.wav");
    int seLand = Novice::LoadAudio("./Resources/sound/jump_land.wav");
    int seUiMove = Novice::LoadAudio("./Resources/sound/ui_button.wav");
    int seUiStart = Novice::LoadAudio("./Resources/sound/ui_start.wav");

    int playBgmHandle = -1;
    auto PlayBGM = [&](int audio) {
        if (playBgmHandle != -1) Novice::StopAudio(playBgmHandle);
        playBgmHandle = Novice::PlayAudio(audio, true, VOLUME_BGM);
        };

    //--------------------------------------
    // 纹理（保持不变）
    //--------------------------------------
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");

    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    // ⚠️ 删除旧的 bgFloor（不再需要）

    int groundTex = Novice::LoadTexture("./Resources/floor/floor.png");

    //--------------------------------------
    // 背景与UI初始化（保持不变，改为2纹理）
    //--------------------------------------
    Background bg;
    InitBackground(bg, bgFar, bgNear);  // ✅ 改为 2 参数版本

    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);

    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f);
    InitButton(btn[1], texHow, 850.0f, 450.0f);
    InitButton(btn[2], texExit, 850.0f, 500.0f);
    int selected = 1;

    //--------------------------------------
    // 玩家（保持你工程内 Player 的逻辑与数值）
    //--------------------------------------
    Player player;
    player.Init();
    player.Reset(225.0f, 600.0f);

    //--------------------------------------
    // 段段地面初始化
    //--------------------------------------
    Ground grounds[kNumSegments];
    InitGrounds(grounds, kNumSegments, 600.0f, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);

    //--------------------------------------
    // 状态与控制变量（保持不变）
    //--------------------------------------
    bool  isGameOver = false;
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
    UIState   uiState = MENU;

    //--------------------------------------
    // 主循环
    //--------------------------------------
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        std::memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //--------------------------------------
        // 标题菜单状态
        //--------------------------------------
        if (gameState == READY && uiState == MENU) {

            if (!Novice::IsPlayingAudio(playBgmHandle)) PlayBGM(seBgm);

            if (keys[DIK_W] && !preKeys[DIK_W]) { selected = (selected + 2) % 3; Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE); }
            if (keys[DIK_S] && !preKeys[DIK_S]) { selected = (selected + 1) % 3; Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE); }

            if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
                Novice::PlayAudio(seUiStart, false, VOLUME_UI_START);

                if (selected == 0) {
                    gameState = PLAY;
                    isGameOver = false;
                    title.targetOffsetX = -800.0f;
                    for (int i = 0; i < 3; i++) btn[i].targetOffsetX = +800.0f;
                    player.Reset(225.0f, 600.0f);
                    InitGrounds(grounds, kNumSegments, 600.0f, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);
                    scrollSpeed = 6.0f;
                }
                else if (selected == 1) {
                    uiState = HOWTO;
                }
                else if (selected == 2) {
                    Novice::Finalize();
                    return 0;
                }
            }
        }

        //--------------------------------------
        // 游戏进行中（段段地面 & 落地逻辑）
        //--------------------------------------
        if (gameState == PLAY) {
            bool pressJump = (keys[DIK_SPACE] && !preKeys[DIK_SPACE]);

            if (!isGameOver) {
                // 查询玩家脚下地面高度
                float groundY = QueryGroundYAtX(grounds, kNumSegments, player.center.x, kBlockWidth);

                if (pressJump && !player.isJumping)
                    Novice::PlayAudio(seJump, false, VOLUME_JUMP);

                bool wasAir = player.isJumping;
                player.Update(pressJump, gravity, jumpPower, groundY);
                if (wasAir && !player.isJumping)
                    Novice::PlayAudio(seLand, false, VOLUME_LAND);

                // 地面左移加速（同事风格）
                scrollSpeed += baseAccel;
                if (scrollSpeed > maxScrollSpeed) scrollSpeed = maxScrollSpeed;

                UpdateGrounds(grounds, kNumSegments, scrollSpeed, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);
            }

            // R 键回到菜单
            if (keys[DIK_R] && !preKeys[DIK_R]) {
                gameState = READY;
                uiState = MENU;
                title.targetOffsetX = 0;
                for (int i = 0; i < 3; i++) btn[i].targetOffsetX = 0;
                player.Reset(225.0f, 600.0f);
                InitGrounds(grounds, kNumSegments, 600.0f, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);
                scrollSpeed = 6.0f;
            }
        }

        //--------------------------------------
        // 背景更新
        //--------------------------------------
        if (!isGameOver) {
            bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, BG_LERP);
            UpdateBackground(bg, bgSpeedFactor);
        }

        //--------------------------------------
        // UI 动画
        //--------------------------------------
        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);
        for (int i = 0; i < 3; i++) {
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
                for (int i = 0; i < 3; i++)
                    DrawButton(btn[i], (i == selected) ? 0xF2FC32FF : 0xFFFFFFFF);
            }
            else if (uiState == HOWTO) {
                Novice::DrawSprite(280, 150, texHowTo, 1, 1, 0, 0xFFFFFFFF);
                if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) uiState = MENU;
            }
        }

        if (gameState == PLAY) {
            DrawGrounds(grounds, kNumSegments, groundTex, kBlockWidth, kBlockHeight, SCREEN_H);
            player.Draw();
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
