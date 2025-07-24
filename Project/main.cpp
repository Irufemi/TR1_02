#include <Novice.h>

const char kWindowTitle[] = "LE2B_11_スエヒロ_コウイチ_TR1_02";

const int kWindowWidth = 1280;
const int kWindowHeight = 720;

#include "MapManager.h"
#include <string>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    int posX = 20;
    int posY = 7;

    int offsetX = 0;
    int offsetY = 0;

    const std::string spreadsheetId = "1fL9it6HK4IsAzmViTchDWWG5koE7nbxEfSYKjx6VFM8";
    const std::string sheetNameRange = "TR1_02";
    const std::string apiKey = "AIzaSyAXQOKxJEPBPtuw8kyp0MZIGsQzhrVBzkc";

    MapManager mapMgr(
        spreadsheetId,  // スプレッドシートID
        sheetNameRange, // 範囲
        apiKey,       // APIキー
        20,   // タイルサイズ
        30   // Yオフセット
    );

    mapMgr.Initialize(posX, posY);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        if (preKeys[DIK_A] == 0 && keys[DIK_A] != 0) {
            posX--;
        }
        else if (preKeys[DIK_D] == 0 && keys[DIK_D] != 0) {
            posX++;
        }

        if (preKeys[DIK_W] == 0 && keys[DIK_W] != 0) {
            posY--;
        } else if (preKeys[DIK_S] == 0 && keys[DIK_S] != 0) {
            posY++;
        }

        mapMgr.Update(keys, preKeys,posX,posY);

        Novice::ScreenPrintf(10, 30, "%d,%d", posX, posY);

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        mapMgr.Draw();

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}
