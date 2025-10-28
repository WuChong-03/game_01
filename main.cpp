#include <Novice.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <cstdio>

#include "bg.h"
#include "button.h"
#include "title.h"
#include "Player.h"

//======================
// 基本设定
//======================
const char kWindowTitle[] = "5106_ゴ_ヤマジ_ヤマモト";

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

// 音量
constexpr float VOLUME_BGM = 0.45f;
constexpr float VOLUME_UI_MOVE = 0.8f;
constexpr float VOLUME_UI_START = 2.0f;
constexpr float VOLUME_JUMP = 1.8f;
constexpr float VOLUME_LAND = 1.6f;

//----------------------
// 平台（地面段）参数
//----------------------
constexpr float kBlockWidth = 128.0f;
constexpr float kBlockHeight = 128.0f;
constexpr int   kNumSegments = 4;

// 平台每“段”的随机宽度
constexpr int MIN_BLOCKS_PER_SEG = 6;  
constexpr int MAX_BLOCKS_PER_SEG = 10;  

// 段与段之间的最小空隙：用块数（2块=256px）
constexpr int GAP_BLOCKS = 2;

// 平台的允许高度范围（屏幕向下是+）
// 注意：Y小 = 更高、更难跳；Y大 = 更低、更安全
constexpr float GROUND_Y_MIN = 380.0f; // 最高允许平台
constexpr float GROUND_Y_MAX = 640.0f; // 最低允许平台

// 相邻平台之间的最大高度差（像素）
// 让高度不会突然差太多，跳起来更舒服
constexpr float MAX_HEIGHT_STEP = 200.0f;

//======================
// 难度阶段系统（7阶段）
//======================
//
// 阶段1:   0    ~  3000 px
// 阶段2:   3000 ~  6000 px
// 阶段3:   6000 ~ 10000 px
// 阶段4:   10000~ 20000 px
// 阶段5:   20000~ 30000 px
// 阶段6:   30000~ 40000 px
// 阶段7:   40000+
//
// distanceRun = 累计水平滚动距离
//======================

// 每个阶段结束线（决定阶段切换）
constexpr float STAGE_DISTANCE_ENDS[6] = {
    3000.0f,   // 阶段1结束
    6000.0f,   // 阶段2结束
    10000.0f,  // 阶段3结束
    20000.0f,  // 阶段4结束
    30000.0f,  // 阶段5结束
    40000.0f   // 阶段6结束
    // 超过就是阶段7
};

// 每阶段（索引0~6 -> 阶段1~7）具体速度表
// 地板/平台卷动速度（主游戏推进）
constexpr float GROUND_STAGE_SPEED[7] = {
    10.0f,   // stage1
    13.0f,   // stage2
    15.0f,  // stage3
    17.0f,  // stage4
    19.0f,  // stage5
    21.0f,  // stage6
    23.0f   // stage7
};
// 远景背景速度
constexpr float FAR_STAGE_SPEED[7] = {
    5.0f,
    7.0f,
    9.0f,
    11.0f,
    13.0f,
    15.0f,
    17.0f
};
// 近景背景速度
constexpr float NEAR_STAGE_SPEED[7] = {
    10.0f,   // stage1
    13.0f,   // stage2
    15.0f,  // stage3
    17.0f,  // stage4
    19.0f,  // stage5
    21.0f,  // stage6
    23.0f   // stage7
};

// 玩家动画三档（慢/中/快），用“每帧持续时间(秒/帧)”表示，越小越快
constexpr float RUN_TIER_SPF[3] = { 0.08f, 0.05f, 0.035f }; // 跑步动画
constexpr float JUMP_TIER_SPF[3] = { 0.18f, 0.14f, 0.10f };  // 跳跃动画

// 根据累计距离 -> 阶段(0~6)
static inline int GetStageIndex(float distanceRun) {
    for (int i = 0; i < 6; i++) {
        if (distanceRun < STAGE_DISTANCE_ENDS[i]) {
            return i;
        }
    }
    return 6; // 阶段7
}

// Lerp 小工具
static inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

//======================
// 游戏状态
//======================
enum GameState { READY, PLAY };
enum UIState { MENU, HOWTO };

//======================
// 地面段结构
//======================
struct Ground {
    float x;          // 这段平台的起始X
    float y;          // 这段平台顶面（越大越靠下）
    int   blockCount; // 这段平台的宽度(多少块×128像素)
    bool  isGround;
};

//======================
// 随机/Clamp 工具
//======================
static inline float RandRangeF(float minVal, float maxVal) {
    float t = (float)std::rand() / (float)RAND_MAX; // 0~1
    return minVal + (maxVal - minVal) * t;
}
static inline float ClampF(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// 找到目前最右边平台的“末端X”，然后计算新平台起点X
static inline float ComputeNextSegmentX(const Ground* grounds, int numExisting) {
    // numExisting: 我们只看 [0, numExisting] 之前已经正确放置的段
    float rightMostEnd = grounds[0].x + grounds[0].blockCount * kBlockWidth;
    for (int i = 1; i < numExisting; i++) {
        float endX = grounds[i].x + grounds[i].blockCount * kBlockWidth;
        if (endX > rightMostEnd) {
            rightMostEnd = endX;
        }
    }
    // 新段起点 = 最右末端 + 间隔
    return rightMostEnd + GAP_BLOCKS * kBlockWidth;
}

//======================
// 初始化地面组（开局）
//======================
static inline void InitGrounds(Ground* grounds, int num) {

    // 第一段：手动给一个起点
    int firstBlocks = MIN_BLOCKS_PER_SEG +
        (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

    grounds[0].blockCount = firstBlocks;
    grounds[0].x = 200.0f; // 你可以改成0.0f之类，看你感觉
    grounds[0].y = RandRangeF(GROUND_Y_MIN, GROUND_Y_MAX);
    grounds[0].isGround = true;

    // 后面段依次往右接
    for (int i = 1; i < num; i++) {

        // 随机这个段的宽度
        int blocks = MIN_BLOCKS_PER_SEG +
            (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

        // 高度 = 上一段附近的高度 ± MAX_HEIGHT_STEP
        float prevY = grounds[i - 1].y;
        float minY = ClampF(prevY - MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
        float maxY = ClampF(prevY + MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
        float newY = RandRangeF(minY, maxY);

        grounds[i].blockCount = blocks;
        grounds[i].y = newY;
        grounds[i].isGround = true;

        // 用前面已经放好的段算最右端，再把当前段放过去
        grounds[i].x = ComputeNextSegmentX(grounds, i);
    }
}

//======================
// 把飞出屏幕左侧的段丢到最右边，并随机新宽度/高度
//======================
static inline void RecycleOneGround(
    Ground& g,
    const Ground* grounds,
    int num
) {
    // 找最右端的末尾，以及参考它的高度
    float rightMostEnd = grounds[0].x + grounds[0].blockCount * kBlockWidth;
    float refY = grounds[0].y;

    for (int i = 1; i < num; i++) {
        float endX = grounds[i].x + grounds[i].blockCount * kBlockWidth;
        if (endX > rightMostEnd) {
            rightMostEnd = endX;
            refY = grounds[i].y;
        }
    }

    // 新宽度
    int blocks = MIN_BLOCKS_PER_SEG +
        (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

    // 新高度：参照最右段的y，限制最大高度差
    float minY = ClampF(refY - MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
    float maxY = ClampF(refY + MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
    float newY = RandRangeF(minY, maxY);

    // 新X：接在最右之后 + 间隔
    float newX = rightMostEnd + GAP_BLOCKS * kBlockWidth;

    g.blockCount = blocks;
    g.y = newY;
    g.x = newX;
    g.isGround = true;
}

//======================
// 每帧更新地面（左移并回收）
//======================
static inline void UpdateGrounds(
    Ground* grounds,
    int num,
    float scrollSpeed
) {
    for (int i = 0; i < num; i++) {
        // 左移
        grounds[i].x -= scrollSpeed;

        // 如果整段都离开屏幕左侧，就回收
        if (grounds[i].x + kBlockWidth * grounds[i].blockCount < 0.0f) {
            RecycleOneGround(grounds[i], grounds, num);
        }
    }
}

//======================
// 绘制地面段
//======================
static inline void DrawGrounds(
    const Ground* grounds, int num,
    int groundBodyTex,
    int groundTopTex,
    float blockW, float blockH,
    int screenH
) {
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {

            float topX = grounds[i].x + j * blockW;
            float topY = grounds[i].y - blockH; // 平台顶面（角色踩的那层方块的上边缘）

            // --- 1. 画平台最上层（floor_top.png） ---
            // 这块就是一个 128x128 的贴图，放在平台表面
            Novice::DrawQuad(
                (int)topX, (int)topY,
                (int)(topX + blockW), (int)topY,
                (int)topX, (int)(topY + blockH),
                (int)(topX + blockW), (int)(topY + blockH),
                0, 0,
                (int)blockW, (int)blockH,
                groundTopTex,
                0xFFFFFFFF
            );

            // --- 2. 画平台下面的柱身（floor.png） ---
            // 这一段从 topY+blockH 一直到屏幕底，这里我们继续像以前一样拉伸到底
            float bodyTopY = topY + blockH;
            if (bodyTopY < screenH) {
                Novice::DrawQuad(
                    (int)topX, (int)bodyTopY,
                    (int)(topX + blockW), (int)bodyTopY,
                    (int)topX, screenH,
                    (int)(topX + blockW), screenH,
                    0, 0,
                    (int)blockW, (int)blockH,
                    groundBodyTex,
                    0xFFFFFFFF
                );
            }
        }
    }
}


//======================
// 根据玩家X找到脚下平台的y（找不到就返回大值=掉下去）
//======================
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

//======================
// 玩家 vs 平台 碰撞修正（AABB对AABB，四方向推开）
//======================
static void ResolvePlayerCollision(
    Player& player, const Ground* grounds, int num,
    float blockW, float blockH
) {
    float px = player.center.x - player.w / 2.0f;
    float py = player.center.y - player.h / 2.0f;
    float pw = player.w;
    float ph = player.h;

    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {

            float gx = grounds[i].x + j * blockW;
            float tileTop = grounds[i].y - blockH;
            float tileBottom = (float)SCREEN_H;

            bool overlapX = (px + pw > gx) && (px < gx + blockW);
            bool overlapY = (py + ph > tileTop) && (py < tileBottom);

            if (overlapX && overlapY) {
                float overlapLeft = (px + pw) - gx;
                float overlapRight = (gx + blockW) - px;
                float overlapTop = (py + ph) - tileTop;
                float overlapBottom = (tileBottom)-py;

                // 找最小重叠方向
                float m = overlapLeft;
                int   dir = 0; // 0=left,1=right,2=top(脚踩),3=bottom(头撞)

                if (overlapRight < m) { m = overlapRight;  dir = 1; }
                if (overlapTop < m) { m = overlapTop;    dir = 2; }
                if (overlapBottom < m) { m = overlapBottom; dir = 3; }

                if (dir == 0) {
                    player.center.x -= overlapLeft;
                }
                else if (dir == 1) {
                    player.center.x += overlapRight;
                }
                else if (dir == 2) {
                    // 从上踩到平台
                    player.center.y -= overlapTop;
                    player.vy = 0.0f;
                    player.isJumping = false;
                }
                else { // 头撞
                    player.center.y += overlapBottom;
                    // vy 不清零，保持“被弹回”
                }

                // 更新AABB给后面block继续用
                px = player.center.x - player.w / 2.0f;
                py = player.center.y - player.h / 2.0f;
            }
        }
    }
}

//======================
// 把玩家出生/重开时摆到平台上
//======================
static inline void PlacePlayerOnGround(Player& player, Ground* grounds) {

    // 基础位置（跟你以前保持类似）
    player.Reset(225.0f, 600.0f);

    float groundY = QueryGroundYAtX(
        grounds, kNumSegments,
        player.center.x,
        kBlockWidth
    );
    float tileTop = groundY - kBlockHeight;

    // 让玩家脚正好踩在平台面上
    player.center.y = tileTop - player.h * 0.5f;
}

//======================
// HUD (调试信息左上角)
//======================
static inline void DrawHUD(
    int x, int y,
    float distanceRun,
    int stageIdx,
    float groundSpeed,
    float nearSpd,
    float farSpd
) {
    Novice::ScreenPrintf(x, y + 0, "Stage     : %d / 7", stageIdx + 1);
    Novice::ScreenPrintf(x, y + 16, "Distance  : %.0f px", distanceRun);
    Novice::ScreenPrintf(x, y + 32, "Ground Spd: %.1f px/f", groundSpeed);
    Novice::ScreenPrintf(x, y + 48, "Near  Spd : %.1f px/f", nearSpd);
    Novice::ScreenPrintf(x, y + 64, "Far   Spd : %.1f px/f", farSpd);
}

//======================
// 回到菜单 / 开始游戏
//======================
static void ResetToMenu(
    GameState& gameState,
    UIState& uiState,
    Title& title,
    Button btn[3],
    Player& player,
    Ground grounds[kNumSegments],
    float& scrollSpeed,
    float& distanceRun
) {
    gameState = READY;
    uiState = MENU;
    distanceRun = 0.0f;

    // 回阶段1速度
    scrollSpeed = GROUND_STAGE_SPEED[0];

    // UI飞回来
    title.targetOffsetX = 0.0f;
    for (int i = 0; i < 3; i++) {
        btn[i].targetOffsetX = 0.0f;
    }

    // 重新生成地面
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 玩家动画速度也回到最慢档
    player.SetAnimSpeeds(RUN_TIER_SPF[0], JUMP_TIER_SPF[0]);
}

static void StartGameplay(
    GameState& gameState,
    Title& title,
    Button btn[3],
    bool& isGameOver,
    Player& player,
    Ground grounds[kNumSegments],
    float& scrollSpeed,
    float& distanceRun
) {
    gameState = PLAY;
    isGameOver = false;
    distanceRun = 0.0f;
    scrollSpeed = GROUND_STAGE_SPEED[0];

    // UI飞走（标题飞左，按钮飞右）
    title.targetOffsetX = -800.0f;
    for (int i = 0; i < 3; i++) {
        btn[i].targetOffsetX = +800.0f;
    }

    // 地面重置/摆玩家到平台上
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 玩家动画速度回到最慢档
    player.SetAnimSpeeds(RUN_TIER_SPF[0], JUMP_TIER_SPF[0]);
}

//======================
// 主函数
//======================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    Novice::Initialize(kWindowTitle, SCREEN_W, SCREEN_H);
    std::srand(unsigned(std::time(nullptr)));

    char keys[256]{};
    char preKeys[256]{};

    // 声音资源
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

    // 贴图资源
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");

    int bgFarTex = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNearTex = Novice::LoadTexture("./Resources/bg_near.png");
    int groundTex = Novice::LoadTexture("./Resources/floor/floor.png");
    int groundTopTex = Novice::LoadTexture("./Resources/floor/floor_top.png");

    // 场景对象
    Background bg;
    InitBackground(bg, bgFarTex, bgNearTex);

    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);

    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f); // Start
    InitButton(btn[1], texHow, 850.0f, 450.0f); // HowTo
    InitButton(btn[2], texExit, 850.0f, 500.0f); // Exit
    int selected = 1; // 默认选中中间

    Player player;
    player.Init();

    Ground grounds[kNumSegments];
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 运行时变量
    bool  isGameOver = false;

    float scrollSpeed = GROUND_STAGE_SPEED[0]; // 由阶段表决定
    float distanceRun = 0.0f;                  // 跑了多远

    const float gravity = 1.1f;
    const float jumpPower = -25.0f;

    // 背景滚动平滑用
    float bgSpeedFactor = 1.0f;  // 当前滚动系数
    float targetBgFactor = 1.0f;  // 目标滚动系数
    const float FLY_T = 0.18f; // UI 飞入飞出Lerp

    GameState gameState = READY;
    UIState   uiState = MENU;

    //======================
    // 主循环
    //======================
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        // 输入
        std::memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //-------------------------
        // READY & MENU
        //-------------------------
        if (gameState == READY && uiState == MENU) {

            // 确保BGM在播
            if (!Novice::IsPlayingAudio(playBgmHandle)) {
                PlayBGM(seBgm);
            }

            // 上下移动菜单
            if (keys[DIK_W] && !preKeys[DIK_W]) {
                selected = (selected + 2) % 3;
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);
            }
            if (keys[DIK_S] && !preKeys[DIK_S]) {
                selected = (selected + 1) % 3;
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);
            }

            // 回车确认
            if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
                Novice::PlayAudio(seUiStart, false, VOLUME_UI_START);

                if (selected == 0) {
                    // Start Game
                    StartGameplay(
                        gameState,
                        title,
                        btn,
                        isGameOver,
                        player,
                        grounds,
                        scrollSpeed,
                        distanceRun
                    );
                }
                else if (selected == 1) {
                    // How to play
                    uiState = HOWTO;
                }
                else if (selected == 2) {
                    // Exit
                    Novice::Finalize();
                    return 0;
                }
            }
        }

        //-------------------------
        // READY & HOWTO
        //-------------------------
        if (gameState == READY && uiState == HOWTO) {
            if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) {
                uiState = MENU;
            }
        }

        //-------------------------
        // PLAY
        //-------------------------
        if (gameState == PLAY) {

            bool pressJump = (keys[DIK_SPACE] && !preKeys[DIK_SPACE]);

            if (!isGameOver) {

                // 1) 根据距离决定阶段
                int stageIdx = GetStageIndex(distanceRun);

                // 各层卷动速度
                scrollSpeed = GROUND_STAGE_SPEED[stageIdx];
                bg.farSpeed = FAR_STAGE_SPEED[stageIdx];
                bg.nearSpeed = NEAR_STAGE_SPEED[stageIdx];

                // 2) 根据阶段设置玩家动画速度（3档）
                // 阶段1-2 => animTier 0
                // 阶段3-5 => animTier 1
                // 阶段6-7 => animTier 2
                int animTier = (stageIdx <= 1) ? 0
                    : (stageIdx <= 4) ? 1
                    : 2;
                player.SetAnimSpeeds(
                    RUN_TIER_SPF[animTier],
                    JUMP_TIER_SPF[animTier]
                );

                // 3) 玩家物理
                if (pressJump && !player.isJumping) {
                    Novice::PlayAudio(seJump, false, VOLUME_JUMP);
                }
                bool wasAir = player.isJumping;

                // 我们传入一个很大的groundY=2000，这样player.Update不会自动吸地
                player.Update(pressJump, gravity, jumpPower, 2000.0f);

                // 由我们来做平台碰撞
                ResolvePlayerCollision(
                    player, grounds, kNumSegments,
                    kBlockWidth, kBlockHeight
                );

                if (wasAir && !player.isJumping) {
                    Novice::PlayAudio(seLand, false, VOLUME_LAND);
                }

                // 4) 地面滚动 + 回收
                UpdateGrounds(
                    grounds, kNumSegments,
                    scrollSpeed
                );

                // 5) 掉出屏幕底部 => GameOver
                if (player.center.y - player.h / 2.0f > SCREEN_H + 200.0f) {
                    isGameOver = true;
                }

                // 6) 累计路程（决定之后的阶段）
                distanceRun += scrollSpeed;
            }

            // R键回菜单（难度/速度/距离重置）
            if (keys[DIK_R] && !preKeys[DIK_R]) {
                ResetToMenu(
                    gameState, uiState,
                    title, btn,
                    player, grounds,
                    scrollSpeed,
                    distanceRun
                );
            }
        }

        //-------------------------
        // 背景滚动更新
        //-------------------------
        if (!isGameOver) {
            // Lerp 背景滚动系数 (0.15f 直接写，不再用 BG_LERP 常量)
            bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, 0.15f);
            UpdateBackground(bg, bgSpeedFactor);
        }

        //-------------------------
        // UI动效(标题呼吸/按钮飞入飞出)
        //-------------------------
        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);

        for (int i = 0; i < 3; i++) {
            UpdateButton(btn[i], i == selected);
            btn[i].offsetX = Lerp(btn[i].offsetX, btn[i].targetOffsetX, FLY_T);
        }

        //-------------------------
        // 绘制阶段
        //-------------------------
        DrawBackground(bg); // 背景两层

        if (gameState == READY) {
            if (uiState == MENU) {
                DrawTitle(title);
                for (int i = 0; i < 3; i++) {
                    unsigned int col = (i == selected)
                        ? 0xF2FC32FF  // 高亮黄
                        : 0xFFFFFFFF; // 普通白
                    DrawButton(btn[i], col);
                }
            }
            else if (uiState == HOWTO) {
                Novice::DrawSprite(
                    280, 150,
                    texHowTo,
                    1.0f, 1.0f,
                    0.0f,
                    0xFFFFFFFF
                );
            }
        }

        if (gameState == PLAY) {

            DrawGrounds(
                grounds, kNumSegments,
                groundTex,       // 柱身用 floor.png
                groundTopTex,    // 顶面用 floor_top.png
                kBlockWidth, kBlockHeight,
                SCREEN_H
            );

            player.Draw();


            // HUD 调试显示
            int stageIdxForHud = GetStageIndex(distanceRun);
            DrawHUD(
                20, 20,
                distanceRun,
                stageIdxForHud,
                scrollSpeed,
                bg.nearSpeed,
                bg.farSpeed
            );
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
