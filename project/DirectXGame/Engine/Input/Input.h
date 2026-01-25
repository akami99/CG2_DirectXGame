#pragma once

#define DIRECTIONINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <Windows.h>                  // HWNDのために必要
#include <dinput.h>                   // DirectInputのヘッダーファイル
#include <wrl.h>                      // Microsoft::WRL::ComPtrのために必要

class Win32Window; // 前方宣言

using namespace Microsoft::WRL; // ComPtrを使うために必要

// キーボード入力を管理するクラス
class Input {
public:
  // namespace省略のためにComPtrをusing宣言
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:                                 // メンバ変数
  static Input *instance_; // シングルトンインスタンス

  ComPtr<IDirectInput8> directInput_;    // DirectInputオブジェクト
  ComPtr<IDirectInputDevice8> keyboard_; // DirectInputキーボードデバイス
  BYTE key_[256] = {};                   // 現在のキー状態
  BYTE preKey_[256] = {};                // 前回のキー状態
  // WindowsAPI
  Win32Window *window_ = nullptr;

public: // メンバ関数
  // シングルトンインスタンスの取得
	static Input* GetInstance();

  // 初期化
  // ウィンドウハンドルとHINSTANCEを引数として受け取る
  void Initialize(Win32Window *window);

  // 終了
  void Finalize();

  // キーボードの状態を更新するメソッド (毎フレーム呼び出す)
  void Update();

  /// <summary>
  /// キーの押下をチェック
  /// </summary>
  /// <param name="keyNumber">キー番号</param>
  /// <returns>押されているか (押しっぱなしも含む)</returns>
  bool IsKeyDown(BYTE keyNumber);

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

private:
  // コンストラクタ
  Input() = default;
  ~Input();
  Input(const Input &) = delete;
  const Input &operator=(const Input &) = delete;
};