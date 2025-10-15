#include <Novice.h>

const char kWindowTitle[] = "5106_ランゲーム（簡易版）";

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	Novice::Initialize(kWindowTitle, 1280, 720);
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	//================================
	// ゲーム用変数の初期化
	//================================

	// プレイヤー（赤い四角）
	float playerX = 200;
	float playerY = 600;      // 初期位置（地面上）
	float playerW = 50;
	float playerH = 50;
	float velocityY = 0;      // 縦方向の速度
	bool isJumping = false;   // ジャンプ中かどうか

	// 障害物（黒い四角）
	float obsX = 1000;
	float obsY = 600;
	float obsW = 50;
	float obsH = 100;

	// 地面
	float groundY = 650; // 地面の高さ

	// ゲーム状態
	bool isGameOver = false;

	// 重力設定
	const float GRAVITY = 0.6f;
	const float JUMP_POWER = -12.0f;
	const float SCROLL_SPEED = 6.0f;

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		if (!isGameOver) {

			//-----------------------------
			// スペースキーでジャンプ
			//-----------------------------
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE] && !isJumping) {
				velocityY = JUMP_POWER;
				isJumping = true;
			}

			//-----------------------------
			// 重力を適用
			//-----------------------------
			velocityY += GRAVITY;
			playerY += velocityY;

			//-----------------------------
			// 地面との接触判定
			//-----------------------------
			if (playerY + playerH >= groundY) {
				playerY = groundY - playerH;
				velocityY = 0;
				isJumping = false;
			}

			//-----------------------------
			// 障害物の移動（左に流す）
			//-----------------------------
			obsX -= SCROLL_SPEED;
			if (obsX + obsW < 0) {
				obsX = float(1280 + rand() % 300); // 画面外右側から再出現
			}

			//-----------------------------
			// 当たり判定（簡易）
			//-----------------------------
			if (playerX < obsX + obsW &&
				playerX + playerW > obsX &&
				playerY < obsY + obsH &&
				playerY + playerH > obsY) {
				isGameOver = true;
			}
		}
		else {
			//-----------------------------
			// ゲームオーバー時のリセット
			//-----------------------------
			if (keys[DIK_R] && !preKeys[DIK_R]) {
				isGameOver = false;
				playerX = 200;
				playerY = 600;
				obsX = 1000;
				velocityY = 0;
			}
		}

		///
		/// ↑更新処理ここまで
		///


		///
		/// ↓描画処理ここから
		///

		// 背景（空）
		Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x87CEFAFF, kFillModeSolid);

		// 地面（灰色）
		Novice::DrawBox(0, (int)groundY, 1280, 720 - (int)groundY, 0.0f, 0x505050FF, kFillModeSolid);

		// プレイヤー（赤い四角）
		Novice::DrawBox((int)playerX, (int)playerY, (int)playerW, (int)playerH, 0.0f, 0xFF0000FF, kFillModeSolid);

		// 障害物（黒い四角）
		Novice::DrawBox((int)obsX, (int)obsY, (int)obsW, (int)obsH, 0.0f, 0x000000FF, kFillModeSolid);

		// テキスト表示
		if (isGameOver) {
			Novice::ScreenPrintf(540, 300, "GAME OVER!");
			Novice::ScreenPrintf(480, 340, "Press [R] to Retry");
		}
		else {
			Novice::ScreenPrintf(50, 50, "SPACE = Jump");
		}

		///
		/// ↑描画処理ここまで
		///

		Novice::EndFrame();

		// ESCで終了
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	Novice::Finalize();
	return 0;
}
