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
	END    // エンド
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

typedef struct Vector2 {
	float x;
	float y;
} Vector2;

typedef struct Corners {
	Vector2 leftTop;
	Vector2 rightTop;
	Vector2 leftBottom;
	Vector2 rightBottom;
} Corners;

typedef struct Player {
	Vector2 centerPos;
	Corners cornersPos;
	float vecY;
	float width;
	float height;
	bool isJumping;
	bool isHit;
} Player;

typedef struct Object {
	Corners pos;
	float width;
	float height;
	int speed;
	int startDelay;
	bool isHitX = false;
	bool isHitY = false;
	bool isSpawned = false;
}Object;

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
	Player player = {
		.centerPos = {225,625},
		.vecY = 0,
		.width = 50,
		.height = 50,
		.isJumping = false,
		.isHit = false
	};

	player.cornersPos = {
			{player.centerPos.x - player.width / 2,player.centerPos.y - player.height / 2},
			{player.centerPos.x + player.width / 2,player.centerPos.y - player.height / 2},
			{player.centerPos.x - player.width / 2,player.centerPos.y + player.height / 2},
			{player.centerPos.x + player.width / 2,player.centerPos.y + player.height / 2}
	};

	Object obs = {
		.width = 50,
		.height = 100,
		.isHitX = false,
		.isHitY = false,
		.isSpawned = false
	};

	obs.pos = {
		{1000,600},
		{1050,600},
		{1000,700},
		{1050,700}
	};

	float groundY = 650;                  // 地面の高さ
	bool isGameOver = false;              // ゲームオーバーかどうか

	const float gravity = 1.2f;           // 重力
	const float jumpPower = -24.0f;      // ジャンプ力

	float playTime = 0.0f;
	float scrollSpeed = 6.0f;      // スクロール速度
	const float maxScrollSpeed = 64.0f;	//スクロール最高速度
	const float baseAccel = 0.2f;

	int playerHandle = Novice::LoadTexture("./NoviceResources/white1x1.png");
	int obsHandle = Novice::LoadTexture("./NoviceResources/white1x1.png");

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
					player.centerPos.x = 225; player.centerPos.y = 625;
					obs.pos = {
						{1000,600},
						{1050,600},
						{1000,700},
						{1050,700}
					};
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

				playTime += 1.0f / 60.0f;

				// スペースでジャンプ
				if (keys[DIK_SPACE] && !preKeys[DIK_SPACE] && !player.isJumping) {
					player.vecY = jumpPower;
					player.isJumping = true;
				}

				// 重力を適用
				player.vecY += gravity;
				player.centerPos.y += player.vecY;

				//プレイヤーの座標更新
				player.cornersPos = {
					{player.centerPos.x - player.width / 2,player.centerPos.y - player.height / 2},
					{player.centerPos.x + player.width / 2,player.centerPos.y - player.height / 2},
					{player.centerPos.x - player.width / 2,player.centerPos.y + player.height / 2},
					{player.centerPos.x + player.width / 2,player.centerPos.y + player.height / 2}
				};

				// 地面との接触判定
				if (player.centerPos.y + player.height / 2 >= groundY) {
					player.centerPos.y = groundY - player.height / 2;
					player.vecY = 0;
					player.isJumping = false;
				}

				float startSpeed = fmin(10.0f + playTime * 0.3f, maxScrollSpeed * 0.8f); // 初速
				float accel = baseAccel + playTime * 0.005f;							 //加速度

				//現在速度の更新
				scrollSpeed += accel;
				if (scrollSpeed > maxScrollSpeed) {
					scrollSpeed = maxScrollSpeed;
				}

				// 障害物の移動（左に流れる）
				obs.pos.leftTop.x -= scrollSpeed;
				if (obs.pos.leftTop.x + obs.width < 0) {
					obs.pos.leftTop.x = float(1280 + rand() % 300); // 画面右から再出現
					scrollSpeed = startSpeed;
				}

				//障害物の座標更新
				obs.pos = {
					.leftTop = {obs.pos.leftTop.x,obs.pos.leftTop.y},
					.rightTop = {obs.pos.leftTop.x + obs.width,obs.pos.leftTop.y},
					.leftBottom = {obs.pos.leftTop.x,obs.pos.leftTop.y + obs.height},
					.rightBottom = {obs.pos.leftTop.x + obs.width,obs.pos.leftTop.y + obs.height}
				};

				//x座標の判定
				if (player.cornersPos.rightBottom.x >= obs.pos.leftTop.x &&
					player.cornersPos.leftTop.x <= obs.pos.rightBottom.x) {
					obs.isHitX = true;
				}
				else {
					obs.isHitX = false;
				}
				//y座標の判定
				if (player.cornersPos.leftBottom.y >= obs.pos.leftTop.y &&
					player.cornersPos.leftTop.y <= obs.pos.leftBottom.y) {
					obs.isHitY = true;
				}
				else {
					obs.isHitY = false;
				}
				//当たり判定フラグの変更
				if (obs.isHitX && obs.isHitY) {
					isGameOver = true;
				}
			}
			else {
				// Rキーでリトライ
				if (keys[DIK_R] && !preKeys[DIK_R]) {
					isGameOver = false;
					player.cornersPos = {
						{player.centerPos.x - player.width / 2,player.centerPos.y - player.height / 2},
						{player.centerPos.x + player.width / 2,player.centerPos.y - player.height / 2},
						{player.centerPos.x - player.width / 2,player.centerPos.y + player.height / 2},
						{player.centerPos.x + player.width / 2,player.centerPos.y + player.height / 2}
					};
					obs.pos = {
						{1000,600},
						{1050,600},
						{1000,700},
						{1050,700}
					};
					player.vecY = 0;
					playTime = 0.0f;
					scrollSpeed = 6.0f;
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
			Novice::DrawQuad( //プレイヤー
				(int)player.cornersPos.leftTop.x, (int)player.cornersPos.leftTop.y,
				(int)player.cornersPos.rightTop.x, (int)player.cornersPos.rightTop.y,
				(int)player.cornersPos.leftBottom.x, (int)player.cornersPos.leftBottom.y,
				(int)player.cornersPos.rightBottom.x, (int)player.cornersPos.rightBottom.y,
				0, 0, (int)player.width, (int)player.height, playerHandle, RED);
			Novice::DrawQuad( //障害物
				(int)obs.pos.leftTop.x, (int)obs.pos.leftTop.y,
				(int)obs.pos.rightTop.x, (int)obs.pos.rightTop.y,
				(int)obs.pos.leftBottom.x, (int)obs.pos.leftBottom.y,
				(int)obs.pos.rightBottom.x, (int)obs.pos.rightBottom.y,
				0, 0, (int)obs.width, (int)obs.height, obsHandle, BLACK);

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
