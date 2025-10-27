#include <Novice.h>
#include <cmath>
#include <cstdlib> // rand() 関数用

//--------------------------------------------
// 定数（ウィンドウタイトルなど）
//--------------------------------------------
const char kWindowTitle[] = "ランゲーム（Title + Game Scene Integrated）";
const int kWindowWidth = 1280;
const int kWindowHeight = 720;
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
	float vecX;
	float vecY;
	float width;
	float height;
	bool isJumping;
	bool onGround;
} Player;

struct Ground {
	float x;
	float y;
	int blockCount;
	bool isGround;
};

const float kHeights[] = { 400.0f, 500.0f, 600.0f,700.0f };
const int kNumHeights = sizeof(kHeights) / sizeof(kHeights[0]);

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
		.vecX = 0,
		.vecY = 0,
		.width = 64,
		.height = 64,
		.isJumping = false
	};

	player.cornersPos = {
			{player.centerPos.x - player.width / 2,player.centerPos.y - player.height / 2},
			{player.centerPos.x + player.width / 2,player.centerPos.y - player.height / 2},
			{player.centerPos.x - player.width / 2,player.centerPos.y + player.height / 2},
			{player.centerPos.x + player.width / 2,player.centerPos.y + player.height / 2}
	};

	bool isGameOver = false;              // ゲームオーバーかどうか

	const float gravity = 1.2f;           // 重力
	const float jumpPower = -24.0f;      // ジャンプ力

	float playTime = 0.0f;
	float scrollSpeed = 6.0f;
	const float kStageTimes[3] = { 60.0f, 120.0f, 180.0f };    // 各段階の開始時間
	const float kScrollSpeeds[4] = { 6.0f, 6.0f, 18.0f, 36.0f };   // 各段階のスクロール速度

	// 難易度レベル
	// 0 = TITLE用（穴なし）、1 = easy、2 = normal、3 = hard
	int difficultyLevel = 0;


	const float kBlockWidth = 128.0f;
	const float kBlockHeight = 128.0f;
	const int kGroundBlocks = 5;
	const int kHoleBlocks[4] = { 0, 1, 2, 3 };
	const int kNumGrounds = 4;

	Ground grounds[kNumGrounds];

	int groundTexHandle = Novice::LoadTexture("./Resources/floor-export.png");
	int playerHandle = Novice::LoadTexture("./NoviceResources/white1x1.png");

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
					difficultyLevel = 1;
					for (int i = 0; i < kNumGrounds; i++) {
						grounds[i].x = i * kBlockWidth * (kGroundBlocks + kHoleBlocks[difficultyLevel]); // 穴2ブロック分を想定
						if (i <= 3) {
							grounds[i].y = player.centerPos.y + player.height / 2;
						}
						else {
							float prevY = grounds[i - 1].y;
							float newY;
							do {
								newY = kHeights[rand() % kNumHeights];
							} while (fabs(newY - prevY) < 120.0f); // 近すぎ回避
							grounds[i].y = newY;
						}
						grounds[i].blockCount = kGroundBlocks;
						grounds[i].isGround = true;
					}
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

				// 最大落下位置
				const float kMaxFallY = kWindowHeight + 100.0f;

				// 重力を適用
				player.vecY += gravity;
				player.centerPos.y += player.vecY;

				// 落下上限チェック
				if (player.centerPos.y > kMaxFallY) {
					player.centerPos.y = kMaxFallY;
					player.vecY = 0; // 落下速度リセット
				}

				//プレイヤーの座標更新
				player.cornersPos = {
					{player.centerPos.x - player.width / 2,player.centerPos.y - player.height / 2},
					{player.centerPos.x + player.width / 2,player.centerPos.y - player.height / 2},
					{player.centerPos.x - player.width / 2,player.centerPos.y + player.height / 2},
					{player.centerPos.x + player.width / 2,player.centerPos.y + player.height / 2}
				};

				// 現在のスクロール段階を決定
				int stageIndex = 0;
				for (int i = 0; i < 3; i++) {
					if (playTime >= kStageTimes[i]) {
						stageIndex = i;
					}
				}

				// スクロール速度を段階に応じて設定
				scrollSpeed = kScrollSpeeds[difficultyLevel];

				for (int i = 0; i < kNumGrounds; i++) {
					grounds[i].x -= scrollSpeed;

					// 左に消えたら右端に再配置
					if (grounds[i].x + kBlockWidth * kGroundBlocks < 0) {
						// 右端にある足場を探す
						float maxX = grounds[0].x;
						for (int j = 1; j < kNumGrounds; j++) {
							if (grounds[j].x > maxX) {
								maxX = grounds[j].x;
							}
						}

						// 高さをランダムに変える（似すぎないように）
						float newY;
						do {
							newY = kHeights[rand() % kNumHeights];
						} while (fabs(newY - grounds[i].y) < 120.0f); // 近すぎは避ける

						// 最後の穴の後に再出現
						grounds[i].x = maxX + kBlockWidth * (kGroundBlocks + 2);
						grounds[i].y = newY;
					}
				}

				// 地面との接触（上からだけ見る）
				bool onGround = false;

				for (int i = 0; i < kNumGrounds; i++) {
					for (int j = 0; j < grounds[i].blockCount; j++) {
						float blockX = grounds[i].x + j * kBlockWidth;
						float blockY = grounds[i].y;

						// 横範囲チェック
						if (player.centerPos.x + player.width / 2 > blockX &&
							player.centerPos.x - player.width / 2 < blockX + kBlockWidth) {

							// 上から着地
							if (player.centerPos.y + player.height / 2 >= blockY &&
								player.centerPos.y < blockY) {
								player.centerPos.y = blockY - player.height / 2;
								player.vecY = 0;
								player.isJumping = false;
								onGround = true;
							}
						}
					}
				}

				// 足場の上にいない → 落下扱い
				if (!onGround) {
					player.isJumping = true;
				}

				// プレイヤーが地面より下がったらスクロール開始
				if (player.centerPos.y > currentGroundY) {
					for (int i = 0; i < kNumGrounds; i++) {
						grounds[i].x -= scrollSpeed;
					}
				}

				// 画面左端に出たらゲームオーバー
				if (player.centerPos.x + player.width / 2 < 0) isGameOver = true;

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
			for (int i = 0; i < kNumGrounds; i++) {
				for (int j = 0; j < grounds[i].blockCount; j++) {
					float tileX = grounds[i].x + j * kBlockWidth;
					float tileY = grounds[i].y;

					// 足場は上端 tileY から画面下まで描画
					Novice::DrawQuad(
						(int)tileX, (int)tileY,        // 左上
						(int)(tileX + kBlockWidth), (int)tileY,   // 右上
						(int)tileX, kWindowHeight,     // 左下
						(int)(tileX + kBlockWidth), kWindowHeight, // 右下
						0, 0, (int)kBlockWidth, (int)kBlockHeight,
						groundTexHandle,
						WHITE
					);
				}
			}

			// プレイヤー（赤）と障害物（黒）
			Novice::DrawQuad( //プレイヤー
				(int)player.cornersPos.leftTop.x, (int)player.cornersPos.leftTop.y,
				(int)player.cornersPos.rightTop.x, (int)player.cornersPos.rightTop.y,
				(int)player.cornersPos.leftBottom.x, (int)player.cornersPos.leftBottom.y,
				(int)player.cornersPos.rightBottom.x, (int)player.cornersPos.rightBottom.y,
				0, 0, (int)player.width, (int)player.height, playerHandle, RED);

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
