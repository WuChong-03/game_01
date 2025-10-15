#include <Novice.h>
#include <cmath> // for fabsf

const char kWindowTitle[] = "Title Screen (3 Buttons + Lerp Animation)";

//===============================//
// Scene Definition
//===============================//
enum Scene {
	TITLE,
	PLAY,
	CLEAR,
	END
};

//===============================//
// Linear Interpolation (Lerp)
//===============================//
float Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

//===============================//
// Main Function
//===============================//
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Novice::Initialize(kWindowTitle, 1280, 720);
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	Scene currentScene = TITLE;
	bool showHowTo = false; // Show "How to Play" flag

	//------------------------------------
	// Button settings for title screen
	//------------------------------------
	const int buttonCount = 3;
	const char* buttonNames[buttonCount] = { "Start Game", "How to Play", "Exit" };

	struct Button {
		float x, y, w, h;
		float targetScale;  // Target scale
		float currentScale; // Current scale
	};

	Button buttons[buttonCount];

	for (int i = 0; i < buttonCount; i++) {
		buttons[i].x = 540;
		buttons[i].y = float(280 + i * 120);
		buttons[i].w = 200;
		buttons[i].h = 60;
		buttons[i].targetScale = 1.0f;
		buttons[i].currentScale = 1.0f;
	}

	int selectedIndex = 0; // Current selected button index

	//------------------------------------
	// Main loop
	//------------------------------------
	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓ Update
		///
		switch (currentScene) {

		case TITLE:
			//--------------------------------
			// Move selection with Up/Down keys
			//--------------------------------
			if (keys[DIK_UP] && !preKeys[DIK_UP]) {
				selectedIndex--;
				if (selectedIndex < 0) selectedIndex = buttonCount - 1;
			}
			if (keys[DIK_DOWN] && !preKeys[DIK_DOWN]) {
				selectedIndex++;
				if (selectedIndex >= buttonCount) selectedIndex = 0;
			}

			//--------------------------------
			// Confirm selection with SPACE key
			//--------------------------------
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
				switch (selectedIndex) {
				case 0: // Start Game
					currentScene = PLAY;
					showHowTo = false;
					break;
				case 1: // How to Play
					showHowTo = !showHowTo; // Toggle visibility
					break;
				case 2: // Exit
					Novice::Finalize();
					return 0;
				}
			}

			//--------------------------------
			// Lerp animation update
			//--------------------------------
			for (int i = 0; i < buttonCount; i++) {
				if (i == selectedIndex) {
					buttons[i].targetScale = 1.2f; // Enlarge when selected
				}
				else {
					buttons[i].targetScale = 1.0f; // Normal size
				}
				buttons[i].currentScale = Lerp(buttons[i].currentScale, buttons[i].targetScale, 0.1f);
			}
			break;

		case PLAY:
			// Demo: Press SPACE to go to CLEAR scene
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
				currentScene = CLEAR;
			}
			break;

		case CLEAR:
			// Press SPACE to go to END scene
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
				currentScene = END;
			}
			break;

		case END:
			// Press SPACE to return to title
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
				currentScene = TITLE;
			}
			break;
		}

		///
		/// ↓ Draw
		///
		Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x223355FF, kFillModeSolid);

		switch (currentScene) {

		case TITLE:
			// Title text
			Novice::ScreenPrintf(480, 120, "★ Run Game Title Screen ★");

			// Button drawing
			for (int i = 0; i < buttonCount; i++) {
				float scale = buttons[i].currentScale;
				int color = (i == selectedIndex) ? 0xFFFFFFFF : 0xAAAAAAFF;

				Novice::DrawBox(
					int(buttons[i].x - buttons[i].w * scale / 2),
					int(buttons[i].y - buttons[i].h * scale / 2),
					int(buttons[i].w * scale),
					int(buttons[i].h * scale),
					0.0f,
					color,
					kFillModeSolid
				);

				Novice::ScreenPrintf(int(buttons[i].x - 40), int(buttons[i].y - 10), "%s", buttonNames[i]);
			}

			// "How to Play" window (temporary)
			if (showHowTo) {
				Novice::DrawBox(380, 180, 520, 320, 0.0f, 0x000000AA, kFillModeSolid);
				Novice::ScreenPrintf(420, 220, "← HOW TO PLAY (will be replaced with image)");
				Novice::ScreenPrintf(420, 260, "SPACE: Select / Confirm");
				Novice::ScreenPrintf(420, 300, "UP / DOWN: Move cursor");
				Novice::ScreenPrintf(420, 340, "ESC: Quit game");
			}
			break;

		case PLAY:
			Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x336633FF, kFillModeSolid);
			Novice::ScreenPrintf(540, 300, "[ PLAY SCENE ]");
			break;

		case CLEAR:
			Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x4444AAFF, kFillModeSolid);
			Novice::ScreenPrintf(540, 300, "[ CLEAR SCENE ]");
			break;

		case END:
			Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x111111FF, kFillModeSolid);
			Novice::ScreenPrintf(560, 300, "[ END SCENE ]");
			break;
		}

		Novice::EndFrame();

		// Force quit with ESC
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	Novice::Finalize();
	return 0;
}
