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
#include "Score.h" // 分数系统

//======================
// 基本设定
//======================
const char kWindowTitle[] = "5106_フレームラッシュ";

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

//======================
// 音量
//======================
constexpr float VOLUME_BGM = 0.45f;
constexpr float VOLUME_UI_MOVE = 0.8f;
constexpr float VOLUME_UI_START = 2.0f;
constexpr float VOLUME_JUMP = 2.5f;
constexpr float VOLUME_LAND = 2.5f;

//======================
// 平台（地面段）参数
//======================
constexpr float kBlockWidth = 128.0f;
constexpr float kBlockHeight = 128.0f;
constexpr int   kNumSegments = 4;

// 每一段平台由多少块128px的方块组成（随机范围）
constexpr int MIN_BLOCKS_PER_SEG = 6;
constexpr int MAX_BLOCKS_PER_SEG = 10;

// 段与段之间的水平间隔（留坑）
constexpr int GAP_BLOCKS = 2;

// 平台允许出现的高度范围（y 越大越靠下）
constexpr float GROUND_Y_MIN = 380.0f;
constexpr float GROUND_Y_MAX = 640.0f;

// 相邻两段平台的最大高度差（像素）
constexpr float MAX_HEIGHT_STEP = 200.0f;

//======================
// 难度阶段（7个阶段）
//======================
// 跑的距离到达这些值就往下一个阶段走
constexpr float STAGE_DISTANCE_ENDS[6] = {
    3000.0f,
    6000.0f,
    10000.0f,
    20000.0f,
    30000.0f,
    40000.0f
};

// 每个阶段的地面卷动速度
constexpr float GROUND_STAGE_SPEED[7] = {
    10.0f,
    13.0f,
    15.0f,
    17.0f,
    19.0f,
    21.0f,
    23.0f
};

// 背景远景层速度（远景=慢）
constexpr float FAR_STAGE_SPEED[7] = {
    5.0f,
    7.0f,
    9.0f,
    11.0f,
    13.0f,
    15.0f,
    17.0f
};

// 背景近景层速度（近景=快）
constexpr float NEAR_STAGE_SPEED[7] = {
    10.0f,
    13.0f,
    15.0f,
    17.0f,
    19.0f,
    21.0f,
    23.0f
};

// 玩家动画速度（跑步/跳跃）3档
constexpr float RUN_TIER_SPF[3] = { 0.08f, 0.05f, 0.035f };
constexpr float JUMP_TIER_SPF[3] = { 0.18f, 0.14f, 0.10f };

//======================
// 小工具函数
//======================
static inline int GetStageIndex(float distanceRun) {
    for (int i = 0; i < 6; i++) {
        if (distanceRun < STAGE_DISTANCE_ENDS[i]) {
            return i;
        }
    }
    return 6;
}

static inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

//======================
// 游戏状态
//======================
enum GameState { READY, PLAY };
enum UIState { MENU, HOWTO };

//======================
// 地面段
//======================
struct Ground {
    float x;          // 这段平台起始X
    float y;          // 这段平台的顶面y（越大越靠下）
    int   blockCount; // 这段平台由多少块128px拼成
    bool  isGround;
};

//======================
// 工具：随机/Clamp
//======================
static inline float RandRangeF(float minVal, float maxVal) {
    float t = (float)std::rand() / (float)RAND_MAX;
    return minVal + (maxVal - minVal) * t;
}
static inline float ClampF(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// 计算“下一段平台”的起始x
static inline float ComputeNextSegmentX(const Ground* grounds, int numExisting) {
    float rightMostEnd = grounds[0].x + grounds[0].blockCount * kBlockWidth;
    for (int i = 1; i < numExisting; i++) {
        float endX = grounds[i].x + grounds[i].blockCount * kBlockWidth;
        if (endX > rightMostEnd) {
            rightMostEnd = endX;
        }
    }
    // 新段从最右段的末尾再加 GAP_BLOCKS 的空隙
    return rightMostEnd + GAP_BLOCKS * kBlockWidth;
}

//======================
// 初始化一组平台（开局）
//======================
static inline void InitGrounds(Ground* grounds, int num) {

    // 第一段
    int firstBlocks = MIN_BLOCKS_PER_SEG +
        (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

    grounds[0].blockCount = firstBlocks;
    grounds[0].x = 200.0f; // 起始段稍微靠右一点，给角色出生空间
    grounds[0].y = RandRangeF(GROUND_Y_MIN, GROUND_Y_MAX);
    grounds[0].isGround = true;

    // 后续段
    for (int i = 1; i < num; i++) {

        int blocks = MIN_BLOCKS_PER_SEG +
            (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

        float prevY = grounds[i - 1].y;
        float minY = ClampF(prevY - MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
        float maxY = ClampF(prevY + MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
        float newY = RandRangeF(minY, maxY);

        grounds[i].blockCount = blocks;
        grounds[i].y = newY;
        grounds[i].isGround = true;

        grounds[i].x = ComputeNextSegmentX(grounds, i);
    }
}

//======================
// 回收飞出左边的段 → 丢到最右边重新当新段
//======================
static inline void RecycleOneGround(
    Ground& g,
    const Ground* grounds,
    int num
) {
    // 找目前最右端
    float rightMostEnd = grounds[0].x + grounds[0].blockCount * kBlockWidth;
    float refY = grounds[0].y;
    for (int i = 1; i < num; i++) {
        float endX = grounds[i].x + grounds[i].blockCount * kBlockWidth;
        if (endX > rightMostEnd) {
            rightMostEnd = endX;
            refY = grounds[i].y;
        }
    }

    // 新段的宽度
    int blocks = MIN_BLOCKS_PER_SEG +
        (std::rand() % (MAX_BLOCKS_PER_SEG - MIN_BLOCKS_PER_SEG + 1));

    // 新段的高度，别跳变太离谱
    float minY = ClampF(refY - MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
    float maxY = ClampF(refY + MAX_HEIGHT_STEP, GROUND_Y_MIN, GROUND_Y_MAX);
    float newY = RandRangeF(minY, maxY);

    float newX = rightMostEnd + GAP_BLOCKS * kBlockWidth;

    g.blockCount = blocks;
    g.y = newY;
    g.x = newX;
    g.isGround = true;
}

//======================
// 每帧让平台往左滚 & 回收出界的
//======================
static inline void UpdateGrounds(
    Ground* grounds,
    int num,
    float scrollSpeed
) {
    for (int i = 0; i < num; i++) {
        grounds[i].x -= scrollSpeed;

        // 这一段完全飞出屏幕后，扔到最右边
        if (grounds[i].x + kBlockWidth * grounds[i].blockCount < 0.0f) {
            RecycleOneGround(grounds[i], grounds, num);
        }
    }
}

//======================
// 画平台
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
            float topY = grounds[i].y - blockH;

            // 顶面一层（floor_top.png）——像草皮/路面
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

            // 柱身往下垂（floor.png）——像土块
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
// 给定玩家X，找到他脚下那一段平台的Y
// 找不到就返回2000（超大值=表示“没有地面”）
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
// 玩家 vs 平台 的AABB碰撞修正
//======================
static void ResolvePlayerCollision(
    Player& player, const Ground* grounds, int num,
    float blockW, float blockH
) {
    // 先拿玩家当前矩形
    float px = player.center.x - player.w / 2.0f;
    float py = player.center.y - player.h / 2.0f;
    float pw = player.w;
    float ph = player.h;

    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {

            float gx = grounds[i].x + j * blockW;
            float tileTop = grounds[i].y - blockH;
            float tileBottom = (float)SCREEN_H; // 地面柱身一直垂到屏幕底部

            // 轴对齐矩形重叠判定
            bool overlapX = (px + pw > gx) && (px < gx + blockW);
            bool overlapY = (py + ph > tileTop) && (py < tileBottom);

            if (overlapX && overlapY) {
                float overlapLeft = (px + pw) - gx;
                float overlapRight = (gx + blockW) - px;
                float overlapTop = (py + ph) - tileTop;
                float overlapBottom = (tileBottom)-py;

                float m = overlapLeft;
                int   dir = 0; // 0=left,1=right,2=top(脚踩到),3=bottom(头撞上)

                if (overlapRight < m) { m = overlapRight;  dir = 1; }
                if (overlapTop < m) { m = overlapTop;    dir = 2; }
                if (overlapBottom < m) { m = overlapBottom; dir = 3; }

                if (dir == 0) {
                    // 撞到平台块的右边 → 往左推回玩家
                    player.center.x -= overlapLeft;
                }
                else if (dir == 1) {
                    // 撞到平台块的左边 → 往右推回玩家
                    player.center.x += overlapRight;
                }
                else if (dir == 2) {
                    // 从上方踩到平台
                    player.center.y -= overlapTop;
                    player.vy = 0.0f;
                    player.isJumping = false;
                }
                else { // dir == 3
                    // 玩家头撞到平台底部
                    player.center.y += overlapBottom;
                    // vy保持向下(被弹回)，不强制归零
                }

                // 修正后更新px/py，防止一帧多块重叠时偏移错误
                px = player.center.x - player.w / 2.0f;
                py = player.center.y - player.h / 2.0f;
            }
        }
    }
}

//======================
// 出生/重开时把玩家安放到当前平台的顶面
//======================
static inline void PlacePlayerOnGround(Player& player, Ground* grounds) {
    // 玩家Reset，给个大致起点
    player.Reset(225.0f, 600.0f);

    float groundY = QueryGroundYAtX(
        grounds, kNumSegments,
        player.center.x,
        kBlockWidth
    );
    float tileTop = groundY - kBlockHeight;

    // 把玩家脚正好贴在平台面上
    player.center.y = tileTop - player.h * 0.5f;
}

//======================
// 回到菜单 / 开始游戏
//======================
// 注意：我们现在把 ScoreManager& score 也传进来，
// 这样重开/回菜单的时候也能把分数清0。
static void ResetToMenu(
    GameState& gameState,
    UIState& uiState,
    Title& title,
    Button btn[3],
    Player& player,
    Ground grounds[kNumSegments],
    float& scrollSpeed,
    float& distanceRun,
    bool& isGameOver,
    // ▼ 分数UI位置 (飞过去又飞回来)
    float& scoreDrawX,
    float& scoreDrawY,
    float& scoreTargetX,
    float& scoreTargetY,
    // ▼ 分数管理器（要清零）
    ScoreManager& score
) {
    gameState = READY;
    uiState = MENU;
    distanceRun = 0.0f;

    // 回阶段1的速度
    scrollSpeed = GROUND_STAGE_SPEED[0];

    // 不算GameOver了（回标题就相当于准备下一局）
    isGameOver = false;

    // 菜单UI飞回来
    title.targetOffsetX = 0.0f;
    for (int i = 0; i < 3; i++) {
        btn[i].targetOffsetX = 0.0f;
    }

    // 重新生成地面、放玩家
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 玩家动画速度恢复到最慢档（阶段1的感觉）
    player.SetAnimSpeeds(RUN_TIER_SPF[0], JUMP_TIER_SPF[0]);

    // 分数UI回右上角
    scoreDrawX = 1000.0f;
    scoreDrawY = 20.0f;
    scoreTargetX = 1000.0f;
    scoreTargetY = 20.0f;

    // !!! 分数清零 !!!
    score.Init();
}

static void StartGameplay(
    GameState& gameState,
    Title& title,
    Button btn[3],
    bool& isGameOver,
    Player& player,
    Ground grounds[kNumSegments],
    float& scrollSpeed,
    float& distanceRun,
    // ▼ 分数UI位置
    float& scoreDrawX,
    float& scoreDrawY,
    float& scoreTargetX,
    float& scoreTargetY,
    // ▼ 分数管理器
    ScoreManager& score
) {
    gameState = PLAY;
    isGameOver = false;
    distanceRun = 0.0f;
    scrollSpeed = GROUND_STAGE_SPEED[0];

    // 标题飞左，按钮飞右（开场演出）
    title.targetOffsetX = -800.0f;
    for (int i = 0; i < 3; i++) {
        btn[i].targetOffsetX = +800.0f;
    }

    // 地面重置 / 玩家站好
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 玩家动画速度回到最慢档
    player.SetAnimSpeeds(RUN_TIER_SPF[0], JUMP_TIER_SPF[0]);

    // 分数UI开局也放回右上角
    scoreDrawX = 1000.0f;
    scoreDrawY = 20.0f;
    scoreTargetX = 1000.0f;
    scoreTargetY = 20.0f;

    // !!! 新开一局 → 分数也必须归零 !!!
    score.Init();
}

//======================
// 主函数
//======================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    Novice::Initialize(kWindowTitle, SCREEN_W, SCREEN_H);
    std::srand(unsigned(std::time(nullptr)));

    char keys[256]{};
    char preKeys[256]{};

    //----------------------
    // 声音资源
    //----------------------
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

    //----------------------
    // 贴图资源
    //----------------------
    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");

    int bgFarTex = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNearTex = Novice::LoadTexture("./Resources/bg_near.png");
    int groundTex = Novice::LoadTexture("./Resources/floor/floor.png");
    int groundTopTex = Novice::LoadTexture("./Resources/floor/floor_top.png");

    // 提示“按R重开”的图
    int texPressR = Novice::LoadTexture("./Resources/pressR.png");

    //----------------------
    // 场景对象初始化
    //----------------------
    Background bg;
    InitBackground(bg, bgFarTex, bgNearTex);

    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);

    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f); // Start
    InitButton(btn[1], texHow, 850.0f, 450.0f); // HowTo
    InitButton(btn[2], texExit, 850.0f, 500.0f); // Exit
    int selected = 1; // 默认选中中间项

    Player player;
    player.Init();

    Ground grounds[kNumSegments];
    InitGrounds(grounds, kNumSegments);
    PlacePlayerOnGround(player, grounds);

    // 分数管理器（0~9的位图数字）
    ScoreManager score;
    score.Init();

    //----------------------
    // 分数UI的Lerp位置
    //----------------------
    // 分数字当前绘制位置
    float scoreDrawX = 1000.0f;
    float scoreDrawY = 20.0f;
    // 分数字目标位置
    float scoreTargetX = 1000.0f;
    float scoreTargetY = 20.0f;
    // Lerp速率
    const float SCORE_LERP_T = 0.15f;

    //----------------------
    // 运行时变量
    //----------------------
    bool  isGameOver = false;

    float scrollSpeed = GROUND_STAGE_SPEED[0];
    float distanceRun = 0.0f; // 以像素当米来算分

    const float gravity = 1.1f;
    const float jumpPower = -25.0f;

    // 背景滚动的平滑系数
    float bgSpeedFactor = 1.0f;
    float targetBgFactor = 1.0f;
    const float FLY_T = 0.18f; // UI飞动的Lerp速率

    GameState gameState = READY;
    UIState   uiState = MENU;

    //----------------------
    // 游戏主循环
    //----------------------
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        // 输入
        std::memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //======================
        // READY & MENU
        //======================
        if (gameState == READY && uiState == MENU) {

            // 确保BGM在播
            if (!Novice::IsPlayingAudio(playBgmHandle)) {
                PlayBGM(seBgm);
            }

            // W / S 选择菜单
            if (keys[DIK_W] && !preKeys[DIK_W]) {
                selected = (selected + 2) % 3;
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);
            }
            if (keys[DIK_S] && !preKeys[DIK_S]) {
                selected = (selected + 1) % 3;
                Novice::PlayAudio(seUiMove, false, VOLUME_UI_MOVE);
            }

            // 回车确定
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
                        distanceRun,
                        // 分数UI位置
                        scoreDrawX, scoreDrawY,
                        scoreTargetX, scoreTargetY,
                        // 分数管理器
                        score
                    );
                }
                else if (selected == 1) {
                    // How to play 页面
                    uiState = HOWTO;
                }
                else if (selected == 2) {
                    // Exit
                    Novice::Finalize();
                    return 0;
                }
            }
        }

        //======================
        // READY & HOWTO
        //======================
        if (gameState == READY && uiState == HOWTO) {
            if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) {
                uiState = MENU;
            }
        }

        //======================
        // PLAY
        //======================
        if (gameState == PLAY) {

            bool pressJump = (keys[DIK_SPACE] && !preKeys[DIK_SPACE]);

            if (!isGameOver) {
                // 根据距离决定当前阶段
                int stageIdx = GetStageIndex(distanceRun);

                // 根据阶段更新游戏速度
                scrollSpeed = GROUND_STAGE_SPEED[stageIdx];
                bg.farSpeed = FAR_STAGE_SPEED[stageIdx];
                bg.nearSpeed = NEAR_STAGE_SPEED[stageIdx];

                // 玩家动画播放速度：3档
                int animTier = (stageIdx <= 1) ? 0
                    : (stageIdx <= 4) ? 1
                    : 2;
                player.SetAnimSpeeds(
                    RUN_TIER_SPF[animTier],
                    JUMP_TIER_SPF[animTier]
                );

                // 玩家物理
                bool wasAir = player.isJumping;
                if (pressJump && !player.isJumping) {
                    Novice::PlayAudio(seJump, false, VOLUME_JUMP);
                }

                // 自己的player.Update，暂时给一个“没有地面”的大值，
                // 具体落地由我们后面的碰撞来修正
                player.Update(pressJump, gravity, jumpPower, 2000.0f);

                // 玩家 vs 平台 的碰撞修正
                ResolvePlayerCollision(
                    player, grounds, kNumSegments,
                    kBlockWidth, kBlockHeight
                );

                // 落地音效
                if (wasAir && !player.isJumping) {
                    Novice::PlayAudio(seLand, false, VOLUME_LAND);
                }

                // 平台往左卷动
                UpdateGrounds(
                    grounds, kNumSegments,
                    scrollSpeed
                );

                // 判断是否掉下去（超过屏幕+200就算死）
                if (player.center.y - player.h / 2.0f > SCREEN_H + 200.0f) {
                    isGameOver = true;
                }

                // 记录跑的距离，用来当分数
                distanceRun += scrollSpeed;
            }

            // R键：回标题，相当于“按R重开”
            if (isGameOver && keys[DIK_R] && !preKeys[DIK_R]) {
                ResetToMenu(
                    gameState, uiState,
                    title, btn,
                    player, grounds,
                    scrollSpeed,
                    distanceRun,
                    isGameOver,
                    // 分数UI位置
                    scoreDrawX, scoreDrawY,
                    scoreTargetX, scoreTargetY,
                    // 分数管理器
                    score
                );
            }
        }

        //======================
        // 分数系统更新
        //======================
        // 这里我们用累计距离distanceRun去更新score
        score.Update(distanceRun);

        //======================
        // 分数UI位置的目标（决定Lerp）
        //======================
        if (gameState == PLAY) {
            if (isGameOver) {
                // 死亡画面：把分数字飞到屏幕中间上方
                scoreTargetX = (float)(SCREEN_W * 0.5f);
                scoreTargetY = (float)(SCREEN_H * 0.4f);
            }
            else {
                // 正常游戏：分数在右上角
                scoreTargetX = 1000.0f;
                scoreTargetY = 20.0f;
            }
        }

        // Lerp分数的位置
        scoreDrawX = Lerp(scoreDrawX, scoreTargetX, SCORE_LERP_T);
        scoreDrawY = Lerp(scoreDrawY, scoreTargetY, SCORE_LERP_T);

        //======================
        // 背景滚动更新
        //======================
        if (!isGameOver) {
            // (背景的speedFactor -> targetBgFactor的Lerp，保留你的平滑控制)
            bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, 0.15f);
            UpdateBackground(bg, bgSpeedFactor);
        }

        //======================
        // UI动效 (标题/按钮飞进飞出)
        //======================
        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);

        for (int i = 0; i < 3; i++) {
            UpdateButton(btn[i], i == selected);
            btn[i].offsetX = Lerp(btn[i].offsetX, btn[i].targetOffsetX, FLY_T);
        }

        //======================
        // 绘制
        //======================
        DrawBackground(bg); // 背景两层

        if (gameState == READY) {
            if (uiState == MENU) {
                DrawTitle(title);
                for (int i = 0; i < 3; i++) {
                    unsigned int col = (i == selected)
                        ? 0xF2FC32FF  // 亮黄色（当前选中的）
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
            // 地面
            DrawGrounds(
                grounds, kNumSegments,
                groundTex,
                groundTopTex,
                kBlockWidth, kBlockHeight,
                SCREEN_H
            );

            // 玩家
            player.Draw();

            // 分数字（用Lerp后的位置）
            score.Draw((int)scoreDrawX, (int)scoreDrawY);

            // GameOver时，提示“按R重开”
            if (isGameOver) {
                int rX = (int)(SCREEN_W * 0.5f - 200.0f); // 让它大概居中
                int rY = (int)(scoreDrawY + 80.0f);       // 分数下面一点
                Novice::DrawSprite(
                    rX, rY,
                    texPressR,
                    1.0f, 1.0f,
                    0.0f,
                    0xFFFFFFFF
                );
            }
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
