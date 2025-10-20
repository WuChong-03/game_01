#include <Novice.h>
#include <cmath>
#include <cstdlib> // rand() 関数用

//--------------------------------------------
// 定数（ウィンドウタイトルなど）
//--------------------------------------------
const char kWindowTitle[] = "ランゲーム（Title + Game Scene Integrated）";

//--------------------------------------------
// シーンの定義
//--------------------------------------------
enum Scene {
	TITLE, // タイトル画面
	PLAY,  // ゲーム中
	END    // エンド（今回は未使用）
};

//--------------------------------------------
// 線形補間（Lerp）関数：なめらかな変化に使用
//--------------------------------------------
float Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

//--------------------------------------------
// ボタン構造体（タイトル画面用）
//--------------------------------------------
struct Button {
	float x, y;          // 位置
	float w, h;          // 幅・高さ
	float targetScale;   // 目標スケール（選択時に拡大）
	float currentScale;  // 現在スケール（Lerpで更新）
	const char* label;   // ボタンに表示する文字
};

//--------------------------------------------
// ボタンの初期化
//--------------------------------------------
void InitButton(Button& button, float x, float y, float w, float h, const char* label) {
	button.x = x;
	button.y = y;
	button.w = w;
	button.h = h;
	button.targetScale = 1.0f;
	button.currentScale = 1.0f;
	button.label = label;
}

//--------------------------------------------
// ボタンの更新（Lerpアニメーション）
//--------------------------------------------
void UpdateButton(Button& button, bool isSelected) {
	button.targetScale = isSelected ? 1.2f : 1.0f;       // 選択中は少し大きく
	button.currentScale = Lerp(button.currentScale, button.targetScale, 0.1f);
}

//--------------------------------------------
// ボタンの描画
//--------------------------------------------
void DrawButton(const Button& button, bool isSelected) {
	int color = isSelected ? 0xFFFFFFFF : 0xAAAAAAFF;    // 選択中は白、非選択はグレー
	Novice::DrawBox(
		int(button.x - button.w * button.currentScale / 2),
		int(button.y - button.h * button.currentScale / 2),
		int(button.w * button.currentScale),
		int(button.h * button.currentScale),
		0.0f,
		color,
		kFillModeSolid
	);
	// ボタン上にテキスト表示
	Novice::ScreenPrintf(int(button.x - 40), int(button.y - 10), "%s", button.label);
}

//--------------------------------------------
// メイン関数（エントリーポイント）
//--------------------------------------------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Novice::Initialize(kWindowTitle, 1280, 720);

	// キー入力管理用配列
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	//--------------------------------------------
	// シーン管理変数
	//--------------------------------------------
	Scene currentScene = TITLE;
	bool showHowTo = false; // 操作説明ウィンドウの表示フラグ

	//--------------------------------------------
	// タイトル画面のボタン設定
	//--------------------------------------------
	const int buttonCount = 3;
	Button buttons[buttonCount];
	const char* buttonNames[buttonCount] = { "Start Game", "How to Play", "Exit" };

	// ボタンの初期化
	for (int i = 0; i < buttonCount; i++) {
		InitButton(buttons[i], 540, float(280 + i * 120), 200, 60, buttonNames[i]);
	}
	int selectedIndex = 0; // 現在選択中のボタン番号

	//--------------------------------------------
	// ランゲームの変数設定
	//--------------------------------------------
	float playerX = 200, playerY = 600;   // プレイヤー位置
	float playerW = 50, playerH = 50;     // プレイヤーのサイズ
	float velocityY = 0;                  // 縦方向速度
	bool isJumping = false;               // ジャンプ中かどうか

	float obsX = 1000, obsY = 600;        // 障害物の位置
	float obsW = 50, obsH = 100;          // 障害物のサイズ
	float groundY = 650;                  // 地面の高さ
	bool isGameOver = false;              // ゲームオーバーかどうか

	const float GRAVITY = 0.6f;           // 重力
	const float JUMP_POWER = -12.0f;      // ジャンプ力
	const float SCROLL_SPEED = 6.0f;      // スクロール速度

	//--------------------------------------------
	// メインループ
	//--------------------------------------------
	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		// キー入力を更新
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		//----------------------------------------
		// 更新処理（Update）
		//----------------------------------------
		switch (currentScene) {

		case TITLE:
			// 上下キーでボタンを選択
			if (keys[DIK_UP] && !preKeys[DIK_UP]) {
				selectedIndex--;
				if (selectedIndex < 0) selectedIndex = buttonCount - 1;
			}
			if (keys[DIK_DOWN] && !preKeys[DIK_DOWN]) {
				selectedIndex++;
				if (selectedIndex >= buttonCount) selectedIndex = 0;
			}

			// エンターキーで決定
			if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
				switch (selectedIndex) {
				case 0: // Start Game
					currentScene = PLAY;  // ゲーム開始
					isGameOver = false;
					playerX = 200; playerY = 600; obsX = 1000; velocityY = 0;
					break;
				case 1: // How to Play
					showHowTo = !showHowTo; // 操作説明ウィンドウの表示切替
					break;
				case 2: // Exit
					Novice::Finalize(); // 終了
					return 0;
				}
			}

			// ボタンのアニメーション更新
			for (int i = 0; i < buttonCount; i++) {
				UpdateButton(buttons[i], i == selectedIndex);
			}
			break;

		case PLAY:
			// プレイ中
			if (!isGameOver) {

				// スペースでジャンプ
				if (keys[DIK_SPACE] && !preKeys[DIK_SPACE] && !isJumping) {
					velocityY = JUMP_POWER;
					isJumping = true;
				}

				// 重力を適用
				velocityY += GRAVITY;
				playerY += velocityY;

				// 地面との接触判定
				if (playerY + playerH >= groundY) {
					playerY = groundY - playerH;
					velocityY = 0;
					isJumping = false;
				}

				// 障害物の移動（左に流れる）
				obsX -= SCROLL_SPEED;
				if (obsX + obsW < 0) {
					obsX = float(1280 + rand() % 300); // 画面右から再出現
				}

				// 当たり判定（AABB）
				if (playerX < obsX + obsW &&
					playerX + playerW > obsX &&
					playerY < obsY + obsH &&
					playerY + playerH > obsY) {
					isGameOver = true;
				}
			}
			else {
				// Rキーでリトライ
				if (keys[DIK_R] && !preKeys[DIK_R]) {
					isGameOver = false;
					playerX = 200;
					playerY = 600;
					obsX = 1000;
					velocityY = 0;
				}

				// ESCキーでタイトルに戻る
				if (keys[DIK_ESCAPE] && !preKeys[DIK_ESCAPE]) {
					currentScene = TITLE;
				}
			}
			break;
		}

		//----------------------------------------
		// 描画処理（Draw）
		//----------------------------------------
		Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x223355FF, kFillModeSolid);

		switch (currentScene) {

		case TITLE:
			// タイトル文字
			Novice::ScreenPrintf(480, 120, "★ Title Screen ★");

			// ボタンの描画
			for (int i = 0; i < buttonCount; i++) {
				DrawButton(buttons[i], i == selectedIndex);
			}

			// 操作説明ウィンドウ
			if (showHowTo) {
				Novice::DrawBox(380, 180, 520, 320, 0.0f, 0x000000AA, kFillModeSolid);
				Novice::ScreenPrintf(420, 220, "HOW TO PLAY");
				Novice::ScreenPrintf(420, 260, "SPACE: Jump");
				Novice::ScreenPrintf(420, 300, "Avoid the obstacles!");
				Novice::ScreenPrintf(420, 340, "Press ESC to return");
			}
			break;

		case PLAY:
			// 背景と地面
			Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x87CEFAFF, kFillModeSolid);
			Novice::DrawBox(0, (int)groundY, 1280, 720 - (int)groundY, 0.0f, 0x505050FF, kFillModeSolid);

			// プレイヤー（赤）と障害物（黒）
			Novice::DrawBox((int)playerX, (int)playerY, (int)playerW, (int)playerH, 0.0f, 0xFF0000FF, kFillModeSolid);
			Novice::DrawBox((int)obsX, (int)obsY, (int)obsW, (int)obsH, 0.0f, 0x000000FF, kFillModeSolid);

			// ゲームオーバー表示
			if (isGameOver) {
				Novice::ScreenPrintf(540, 300, "GAME OVER!");
				Novice::ScreenPrintf(480, 340, "Press [R] to Retry");
				Novice::ScreenPrintf(460, 380, "Press [ESC] to Title");
			}
			else {
				Novice::ScreenPrintf(50, 50, "SPACE = Jump");
			}
			break;
		}

		Novice::EndFrame();
	}

	Novice::Finalize();
	return 0;
}
