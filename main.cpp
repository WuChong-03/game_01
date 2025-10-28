#include <Novice.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include "bg.h"
#include "button.h"
#include "title.h"
#include "Player.h"
#include <algorithm>

const char kWindowTitle[] = "Run Game (段段地面单文件版)";

enum GameState { READY, PLAY };
enum UIState { MENU, HOWTO };

static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

//======================
// 常量设置
//======================
constexpr float VOLUME_BGM = 0.45f;
constexpr float VOLUME_UI_MOVE = 0.8f;
constexpr float VOLUME_UI_START = 2.0f;
constexpr float VOLUME_JUMP = 1.8f;
constexpr float VOLUME_LAND = 1.6f;

//======================
// 段段地面参数
//======================
constexpr float kBlockWidth = 128.0f;
constexpr float kBlockHeight = 128.0f;
constexpr int   kBlocksPerSeg = 5;
constexpr int   kNumSegments = 4;

const float kHeights[] = { 400.0f, 500.0f, 600.0f, 700.0f };
const int   kNumHeights = (int)(sizeof(kHeights) / sizeof(kHeights[0]));

struct Ground {
    float x;
    float y;
    int   blockCount;
    bool  isGround;
};

static inline void InitGrounds(
    Ground* grounds, int num, float /*startY*/,
    float blockW, int blocksPerSeg,
    const float* heights, int nHeights
) {
    for (int i = 0; i < num; i++) {
        grounds[i].x = i * blockW * (blocksPerSeg + 2);
        grounds[i].y = heights[i % nHeights];
        grounds[i].blockCount = blocksPerSeg;
        grounds[i].isGround = true;
    }
}

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

//=================================================
// 检查玩家矩形与地面块的AABB碰撞
//=================================================
//=================================================
// ✅ 改进版：只检测脚下的地面
//=================================================
//------------------------------------------------------
// 完整AABB碰撞检测（区分水平与垂直）
//------------------------------------------------------
//------------------------------------------------------
// 完整AABB碰撞：先左右、再上下（无未使用变量）
//------------------------------------------------------
static void ResolvePlayerCollision(
    Player& player, const Ground* grounds, int num,
    float blockW, float blockH
) {
    // 玩家AABB
    float px = player.center.x - player.w / 2.0f;
    float py = player.center.y - player.h / 2.0f;
    float pw = player.w;
    float ph = player.h;

    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {

            // 这一列地面的X
            float gx = grounds[i].x + j * blockW;

            // 这一列地面的“可见顶部”
            float tileTop = grounds[i].y - blockH;

            // 我们希望碰撞盒一直到屏幕底部
            float tileBottom = (float)SCREEN_H;

            // 也就是说，这个地块的碰撞矩形是：
            // x: gx ~ gx+blockW
            // y: tileTop ~ SCREEN_H
            bool overlapX = (px + pw > gx) && (px < gx + blockW);
            bool overlapY = (py + ph > tileTop) && (py < tileBottom);

            if (overlapX && overlapY) {

                // 计算重叠量（跟你现在的一样的思路）
                float overlapLeft = (px + pw) - gx;
                float overlapRight = (gx + blockW) - px;
                float overlapTop = (py + ph) - tileTop;
                float overlapBottom = (tileBottom)-py;

                // 选最小的那个方向
                float m = overlapLeft;
                int dir = 0; // 0=left,1=right,2=top,3=bottom

                if (overlapRight < m) { m = overlapRight;  dir = 1; }
                if (overlapTop < m) { m = overlapTop;    dir = 2; }
                if (overlapBottom < m) { m = overlapBottom; dir = 3; }

                if (dir == 0) {
                    // 从左边撞进去了 → 往左推回
                    player.center.x -= overlapLeft;
                }
                else if (dir == 1) {
                    // 从右边撞进去了 → 往右推回
                    player.center.x += overlapRight;
                }
                else if (dir == 2) {
                    // 玩家脚踩到平台顶 / 或者压到它上面
                    player.center.y -= overlapTop;
                    player.vy = 0;
                    player.isJumping = false;
                }
                else {
                    // 撞到下面（比如头撞到砖块）
                    player.center.y += overlapBottom;
                    // 这里我们没有清 vy，保持角色被顶回去的感觉
                }

                // 更新px/py，方便继续后面的块也能算
                px = player.center.x - player.w / 2.0f;
                py = player.center.y - player.h / 2.0f;
            }
        }
    }
}







static inline void DrawGrounds(
    const Ground* grounds, int num,
    int groundTex,
    float blockW, float blockH,
    int screenH
) {
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < grounds[i].blockCount; j++) {
            float tileX = grounds[i].x + j * blockW;
            float tileY = grounds[i].y - blockH;
            Novice::DrawQuad(
                (int)tileX, (int)tileY,
                (int)(tileX + blockW), (int)tileY,
                (int)tileX, screenH,
                (int)(tileX + blockW), screenH,
                0, 0, (int)blockW, (int)blockH,
                groundTex, 0xFFFFFFFF
            );
        }
    }
}

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

    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHow = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int texHowTo = Novice::LoadTexture("./Resources/howto.png");

    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    int groundTex = Novice::LoadTexture("./Resources/floor/floor.png");

    Background bg;
    InitBackground(bg, bgFar, bgNear);

    Title title;
    InitTitle(title, texTitle, 120.0f, 100.0f);

    Button btn[3];
    InitButton(btn[0], texStart, 850.0f, 400.0f);
    InitButton(btn[1], texHow, 850.0f, 450.0f);
    InitButton(btn[2], texExit, 850.0f, 500.0f);
    int selected = 1;

    Player player;
    player.Init();

    Ground grounds[kNumSegments];
    InitGrounds(grounds, kNumSegments, 600.0f, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);

    // ✅ 出生时自动贴地
    player.Reset(225.0f, 600.0f);
    float spawnGroundY = QueryGroundYAtX(grounds, kNumSegments, player.center.x, kBlockWidth);
    float tileTop = spawnGroundY - kBlockHeight;
    player.center.y = tileTop - player.h * 0.5f;


    bool  isGameOver = false;
    float scrollSpeed = 6.0f;
    const float maxScrollSpeed = 64.0f;
    const float baseAccel = 0.05f;
    const float gravity = 1.2f;
    const float jumpPower = -30.0f;
    float bgSpeedFactor = 1.0f;
    float targetBgFactor = 1.0f;
    const float BG_LERP = 0.15f;
    const float FLY_T = 0.18f;

    GameState gameState = READY;
    UIState uiState = MENU;

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        std::memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

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
                    float gY = QueryGroundYAtX(grounds, kNumSegments, player.center.x, kBlockWidth);
                    player.center.y = (gY - kBlockHeight) - player.h * 0.5f;
                    scrollSpeed = 6.0f;
                }
                else if (selected == 1) uiState = HOWTO;
                else if (selected == 2) { Novice::Finalize(); return 0; }
            }
        }

        if (gameState == PLAY) {
            bool pressJump = (keys[DIK_SPACE] && !preKeys[DIK_SPACE]);
            if (!isGameOver) {
                if (pressJump && !player.isJumping) Novice::PlayAudio(seJump, false, VOLUME_JUMP);
                bool wasAir = player.isJumping;
                // 由 Player::Update 负责：起跳 + 重力 + 动画（groundY 传 2000，避免自动吸地）
                player.Update(pressJump, gravity, jumpPower, 2000.0f);  // ← 关键！:contentReference[oaicite:3]{index=3}
                // 由我们这边负责：左右墙 & 脚底落地
                ResolvePlayerCollision(player, grounds, kNumSegments, kBlockWidth, kBlockHeight);
                // ✅ 检查是否掉出屏幕底部
                if (player.center.y - player.h / 2.0f > SCREEN_H + 200.0f) {
                    isGameOver = true;  // 掉落判定
                }

                if (wasAir && !player.isJumping) Novice::PlayAudio(seLand, false, VOLUME_LAND);
                scrollSpeed += baseAccel;
                if (scrollSpeed > maxScrollSpeed) scrollSpeed = maxScrollSpeed;
                UpdateGrounds(grounds, kNumSegments, scrollSpeed, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);
            }
            if (keys[DIK_R] && !preKeys[DIK_R]) {
                gameState = READY;
                uiState = MENU;
                title.targetOffsetX = 0;
                for (int i = 0; i < 3; i++) btn[i].targetOffsetX = 0;
                player.Reset(225.0f, 600.0f);
                InitGrounds(grounds, kNumSegments, 600.0f, kBlockWidth, kBlocksPerSeg, kHeights, kNumHeights);
                float gY = QueryGroundYAtX(grounds, kNumSegments, player.center.x, kBlockWidth);
                player.center.y = (gY - kBlockHeight) - player.h * 0.5f;
                scrollSpeed = 6.0f;
            }

        }

        if (!isGameOver) {
            bgSpeedFactor = Lerp(bgSpeedFactor, targetBgFactor, BG_LERP);
            UpdateBackground(bg, bgSpeedFactor);
        }

        UpdateTitle(title);
        title.offsetX = Lerp(title.offsetX, title.targetOffsetX, FLY_T);
        for (int i = 0; i < 3; i++) {
            UpdateButton(btn[i], i == selected);
            btn[i].offsetX = Lerp(btn[i].offsetX, btn[i].targetOffsetX, FLY_T);
        }

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
