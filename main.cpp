#include <Novice.h>
#include <cstring>
#include "button.h"
#include "bg.h"
#include "title.h"

const char kWindowTitle[] = "タイトル付きメニュー（分離版）";

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256]{}, preKeys[256]{};

    int texStart = Novice::LoadTexture("./Resources/button_start.png");
    int texHowTo = Novice::LoadTexture("./Resources/button_howToPlay.png");
    int texExit = Novice::LoadTexture("./Resources/button_exit.png");
    int texTitle = Novice::LoadTexture("./Resources/title.png");
    int bgFar = Novice::LoadTexture("./Resources/bg_far.png");
    int bgNear = Novice::LoadTexture("./Resources/bg_near.png");
    int bgFloor = Novice::LoadTexture("./Resources/floor.png"); 



    Background bg;
    InitBackground(bg, bgFar, bgNear, bgFloor); 

    Button btn[3];
    InitButton(btn[0], 850, 400, 280, 63, texStart);
    InitButton(btn[1], 850, 450, 280, 63, texHowTo);
    InitButton(btn[2], 850, 500, 280, 63, texExit);

    Title title;
    InitTitle(title, texTitle, 80, 108);

    int selected = 1;

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        if (keys[DIK_W] && !preKeys[DIK_W]) selected = (selected + 2) % 3;
        if (keys[DIK_S] && !preKeys[DIK_S]) selected = (selected + 1) % 3;

        UpdateBackground(bg);
        UpdateTitle(title);

        for (int i = 0; i < 3; i++)
            UpdateButton(btn[i], i == selected);

        DrawBackground(bg);
        DrawTitle(title);

        for (int i = 0; i < 3; i++) {
            unsigned int unselectedColor = 0xFFF2FC32;
            unsigned int selectedColor = 0xF2FC32FF;
            unsigned int color = (i == selected) ? selectedColor : unselectedColor;
            DrawButton(btn[i], color);
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
