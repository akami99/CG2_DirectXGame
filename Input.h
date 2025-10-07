#pragma once

#define DIRECTIONINPUT_VERSION    0x0800 // DirectInputのバージョン指定
#include <dinput.h> // DirectInputのヘッダーファイル
#include <Windows.h> // HWNDのために必要
#include <wrl.h>   // Microsoft::WRL::ComPtrのために必要

#include "WindowWrapper.h" // WindowWrapperのヘッダーファイル

using namespace Microsoft::WRL; // ComPtrを使うために必要

// キーボード入力を管理するクラス
class Input {
public:
	// namespace省略のためにComPtrをusing宣言
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private: // メンバ変数
	ComPtr<IDirectInput8> directInput_;    // DirectInputオブジェクト
	ComPtr<IDirectInputDevice8> keyboard_; // DirectInputキーボードデバイス
	BYTE key_[256] = {};                        // 現在のキー状態
	BYTE preKey_[256] = {};                     // 前回のキー状態

	// WindowsAPI
	WindowWrapper* window_ = nullptr;

public: // メンバ関数
	// 初期化
	// ウィンドウハンドルとHINSTANCEを引数として受け取る
	void Initialize(WindowWrapper* window);

	// デストラクタ
	// デバイスの取得を解除
	~Input();

	// キーボードの状態を更新するメソッド (毎フレーム呼び出す)
	void Update();

	/// <summary>
	/// キーの押下をチェック
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>押されているか (押しっぱなしも含む)</returns>
	bool IsKeyDown(BYTE keyNumber);

	// 指定されたキーが押された瞬間か (トリガー)

	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>トリガーか</returns>
	bool IsKeyTriggered(BYTE keyNumber);

	// 指定されたキーが離された瞬間か
	/// <summary>
	/// キーが離された瞬間かをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>離された瞬間か</returns>
	bool IsKeyReleased(BYTE keyNumber);
};