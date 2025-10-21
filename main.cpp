#include <Novice.h>
#include <cmath>

const char kWindowTitle[] = "タイトル画面（背景スクロール付き）";

enum Scene { TITLE, PLAY, CLEAR, END };

float Lerp(float a, float b, float t) { return a + (b - a) * t; }

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    Scene currentScene = TITLE;

    //=============================
    // 背景画像の読み込み
    //=============================
    int bgFarTex = Novice::LoadTexture("./bg_far.png");   // 遠景（例：空・ビル影）
    int bgNearTex = Novice::LoadTexture("./bg_near.png"); // 近景（例：地面・街並み）

    float bgFarX = 0.0f;
    float bgNearX = 0.0f;

    const float FAR_SPEED = 1.0f;   // 遠景のスクロール速度
    const float NEAR_SPEED = 3.0f;  // 近景のスクロール速度

    //=============================
    // タイトル用変数
    //=============================
    const int buttonCount = 3;
    const char* buttonNames[buttonCount] = { "ゲーム開始", "操作説明", "終了" };

    int selectedIndex = 0;
    float baseY = 240.0f;
    float spacing = 120.0f;
    float scrollY = 0.0f;
    float targetY = 0.0f;
    float scale[buttonCount] = { 1.0f, 1.0f, 1.0f };
    float scaleTarget[buttonCount] = { 1.2f, 1.0f, 1.0f };

    bool showHowTo = false;

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        //=============================
        // 更新処理
        //=============================
        switch (currentScene) {
        case TITLE:
            // 背景スクロール更新（ループ）
            bgFarX -= FAR_SPEED;
            bgNearX -= NEAR_SPEED;
            if (bgFarX <= -1280) bgFarX += 1280;
            if (bgNearX <= -1280) bgNearX += 1280;

            // 上下/W/Sキーで選択
            if ((keys[DIK_UP] && !preKeys[DIK_UP]) || (keys[DIK_W] && !preKeys[DIK_W])) {
                selectedIndex--;
                if (selectedIndex < 0) selectedIndex = buttonCount - 1;
            }
            if ((keys[DIK_DOWN] && !preKeys[DIK_DOWN]) || (keys[DIK_S] && !preKeys[DIK_S])) {
                selectedIndex++;
                if (selectedIndex >= buttonCount) selectedIndex = 0;
            }

            // スクロール位置のLerp
            targetY = selectedIndex * spacing;
            scrollY = Lerp(scrollY, targetY, 0.15f);

            // スケールLerp
            for (int i = 0; i < buttonCount; ++i) {
                scaleTarget[i] = (i == selectedIndex) ? 1.2f : 1.0f;
                scale[i] = Lerp(scale[i], scaleTarget[i], 0.15f);
            }

            // 決定
            if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
                if (selectedIndex == 0) currentScene = PLAY;
                else if (selectedIndex == 1) showHowTo = !showHowTo;
                else if (selectedIndex == 2) { Novice::Finalize(); return 0; }
            }
            break;

        case PLAY:
            if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) currentScene = CLEAR;
            break;

        case CLEAR:
            if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) currentScene = END;
            break;

        case END:
            if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) currentScene = TITLE;
            break;
        }

        //=============================
        // 描画処理
        //=============================
        // 背景（2枚でループスクロール）
        for (int i = 0; i < 2; i++) {
            Novice::DrawSprite((int)bgFarX + 1280 * i, 0, bgFarTex, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
            Novice::DrawSprite((int)bgNearX + 1280 * i, 0, bgNearTex, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
        }

        // タイトルロゴ
        Novice::DrawBox(100, 80, 420, 120, 0.0f, 0x88CCFFFF, kFillModeSolid);
        Novice::ScreenPrintf(120, 130, "ゲームタイトル ロゴ（画像予定）");

        if (currentScene == TITLE) {
            // 右側ボタン列
            for (int i = 0; i < buttonCount; ++i) {
                float y = baseY + i * spacing - scrollY;
                float s = scale[i];
                int color = (i == selectedIndex) ? 0xFFFFFFFF : 0xCCCCCCFF;

                int w = int(240 * s);
                int h = int(64 * s);
                int x = 980 - w / 2;
                int by = (int)(y - h / 2);

                Novice::DrawBox(x, by, w, h, 0.0f, color, kFillModeSolid);
                Novice::ScreenPrintf(x + 60, by + 20, "%s", buttonNames[i]);
            }

            // 選択枠（固定一番上）
            Novice::DrawBox(980 - 120, int(baseY) - 32 - 4, 240 + 8, 64 + 8, 0.0f, 0xFFDD00AA, kFillModeWireFrame);

            // 操作説明
            if (showHowTo) {
                Novice::DrawBox(120, 260, 440, 280, 0.0f, 0x000000AA, kFillModeSolid);
                Novice::ScreenPrintf(150, 300, "↑/↓ or W/S：選択移動");
                Novice::ScreenPrintf(150, 340, "SPACE：決定");
            }
        }
        else {
            Novice::ScreenPrintf(540, 300, "【PLAY/他シーン】");
        }

        Novice::EndFrame();

        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    Novice::Finalize();
    return 0;
}
