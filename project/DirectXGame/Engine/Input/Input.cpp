#include "Input.h"
#include "Win32Window.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// スマートポインタの実体定義
std::unique_ptr<Input> Input::instance_ = nullptr;

// シングルトンインスタンスの取得
Input* Input::GetInstance() {
    if (instance_ == nullptr) {
        // Token{} を渡せるのは Input クラス内だけ！
        instance_ = std::make_unique<Input>(Token{});
    }
    return instance_.get();
}

// 終了処理(明示的に破棄したい場合)
void Input::Destroy() {
    instance_.reset(); // これでデストラクタが呼ばれる
}

// コンストラクタ
Input::Input(Token) {
    // コンストラクタでの特別な処理があれば記述
    // 現在はメンバ初期化子で初期化済み
}

// デストラクタ
// DirectInputオブジェクトとキーボードデバイスを解放する
Input::~Input() {
  if (keyboard_) {
    keyboard_->Unacquire(); // デバイスの取得を解除
  }
}

// 初期化
// ウィンドウハンドルとHINSTANCEを引数として受け取る
void Input::Initialize() {
  ZeroMemory(key_, sizeof(key_));
  ZeroMemory(preKey_, sizeof(preKey_));

  // DirectInputの初期化
  HRESULT result;

  result =
      DirectInput8Create(Win32Window::GetInstance()->GetHinstance(), DIRECTINPUT_VERSION,
                         IID_IDirectInput8, (void **)&directInput_, nullptr);
  assert(SUCCEEDED(result));

  // キーボードデバイスの生成
  result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
  assert(SUCCEEDED(result));

  // 入力データ形式のセット
  result = keyboard_->SetDataFormat(
      &c_dfDIKeyboard); // c_dfDIKeyboardはDirectInputで定義済み
  assert(SUCCEEDED(result));

  // 排他制御のレベルのセット
  result = keyboard_->SetCooperativeLevel(
      Win32Window::GetInstance()->GetHwnd(),
      DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(result));

  // デバイスの取得開始 (Acquire)
  // 初期化時に一度Acquireしておくと、初回UpdateでGetDeviceStateを呼ぶ際にエラーになりにくい
  keyboard_->Acquire();
}

// キーボードの状態を更新する関数 (毎フレーム呼び出す)
void Input::Update() {
  HRESULT result;

  // 前回の状態を現在の状態として保存
  memcpy(preKey_, key_, sizeof(key_));

  // 現在のキー状態を取得
  result = keyboard_->GetDeviceState(sizeof(key_), key_);

  // デバイスがロストした場合の処理 (フォアグラウンドから切り替わった場合など)
  if (FAILED(result)) {
    // 再びAcquireを試みる
    result = keyboard_->Acquire();
    if (SUCCEEDED(result)) {
      // 再Acquireに成功したら再度状態を取得
      keyboard_->GetDeviceState(sizeof(key_), key_);
    } else {
      // 再Acquireにも失敗した場合は、キー状態を全てクリアする
      ZeroMemory(key_, sizeof(key_));
    }
  }
}

// 指定されたキーが押されているか (押しっぱなしも含む)
bool Input::IsKeyDown(BYTE keyNumber) {
  // キーが押されているかをチェック
  if (key_[keyNumber]) {
    return true;
  }
  // 押されていない
  return false;
}

// 指定されたキーが押された瞬間か (トリガー)
bool Input::IsKeyTriggered(BYTE keyNumber) {
  // 押された瞬間かをチェック
  if (!(preKey_[keyNumber]) && (key_[keyNumber] & 0x80)) {
    return true;
  }
  // 押された瞬間ではない
  return false;
}

// 指定されたキーが離された瞬間か
bool Input::IsKeyReleased(BYTE keyNumber) {
  // 離された瞬間かをチェック
  if ((preKey_[keyNumber] & 0x80) && !(key_[keyNumber] & 0x80)) {
    return true;
  }
  // 離された瞬間ではない
  return false;
}