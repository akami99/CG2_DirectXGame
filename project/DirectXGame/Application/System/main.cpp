#include <Windows.h>
#include <cassert>
#include <dbghelp.h> // dump
#include <dxgi1_6.h> // dump
#include <strsafe.h> // dump

// #include <vector>
#include <numbers> //particleに使用
#include <random>  //particleに使用

// エンジン
#include "Camera.h"
#include "D3DResourceLeakChecker.h"
#include "DX12Context.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Logger.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ParticleEmitter.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Win32Window.h"
// 管理系
#include "AudioManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "PipelineManager.h"
#include "SrvManager.h"
#include "TextureManager.h"

// グラフィック関連の構造体
// #include "GraphicsTypes.h"
#include "LightTypes.h"
// #include "ModelTypes.h"
#include "ParticleTypes.h"

// 計算用関数など
#include "MathTypes.h"
#include "MathUtils.h"
#include "MatrixGenerators.h"

// ゲーム内設定用
#include "ApplicationConfig.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace MathUtils;
using namespace MathGenerators;
using namespace BlendMode;

#pragma region 関数定義
// 関数定義

static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
  // ログと同じく、exeと同じ階層に「dumps」フォルダを作成する
  const wchar_t *dumpDir = L"dumps";

  CreateDirectoryW(dumpDir, nullptr);

  // 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
  SYSTEMTIME time;
  GetLocalTime(&time);
  wchar_t filePath[MAX_PATH] = {0};

  StringCchPrintfW(filePath, MAX_PATH, L"%s/%04d-%02d%02d-%02d%02d%02d.dmp",
                   dumpDir, time.wYear, time.wMonth, time.wDay, time.wHour,
                   time.wMinute, time.wSecond);

  HANDLE dumpFileHandle =
      CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  // 4. ハンドルの有効性チェック
  if (dumpFileHandle == INVALID_HANDLE_VALUE) {
    // ファイル作成に失敗した場合（権限不足など）、ダンプ処理はスキップして終了
    // エラーコードをログに記録できれば理想だが、ここではシンプルな終了とする
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // processId（このexeのID）とクラッシュ（例外）の発生したthreadIdを取得
  DWORD processId = GetCurrentProcessId();
  DWORD threadId = GetCurrentThreadId();
  // 設定情報を入力
  MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{0};
  minidumpInformation.ThreadId = threadId;
  minidumpInformation.ExceptionPointers = exception;
  minidumpInformation.ClientPointers = TRUE;
  // Dumpを出力、MiniDumpNormalは最低限の情報を出力するフラグ
  MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
                    MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

  // ファイルハンドルを閉じる
  CloseHandle(dumpFileHandle);

  // 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
  return EXCEPTION_EXECUTE_HANDLER;
}

// パーティクル生成関数
Particle MakeNewParticle(std::mt19937 &randomEngine, const Vector3 &translate) {
  // ランダムな速度ベクトルを生成
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> distribution(
      -1.0f, 1.0f); // distribution()という関数のように使うイメージ
  std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
  // パーティクルの初期化
  Particle particle;
  particle.transform.scale = {1.0f, 1.0f, 1.0f};
  particle.transform.rotate = {0.0f, 0.0f, 0.0f};
  particle.lifeTime = distTime(randomEngine);
  particle.currentTime = 0.0f;
  // ランダムに設定
  Vector3 randomTranslate{distribution(randomEngine),
                          distribution(randomEngine),
                          distribution(randomEngine)};
  particle.transform.translate = translate + randomTranslate;

  particle.velocity = {distribution(randomEngine), distribution(randomEngine),
                       distribution(randomEngine)}; // 指定できても良い

  particle.color = {distribution(randomEngine), distribution(randomEngine),
                    distribution(randomEngine),
                    1.0f}; // 基準色から幅を取る際はHSVで出来ると良い

  return particle;
}

// エミッタに合わせてパーティクルを生成しリストを取得
std::list<Particle> Emit(const Emitter &emitter, std::mt19937 &randomEngine) {
  std::list<Particle> particles;
  for (uint32_t count = 0; count < emitter.count; ++count) {
    particles.push_back(
        MakeNewParticle(randomEngine, emitter.transform.translate));
  }
  return particles;
}

#pragma endregion 関数定義ここまで

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  // 誰も捕捉しなかった場合に(Unhandled)、補足する関数を登録
  // main関数始まってすぐに登録すると良い
  SetUnhandledExceptionFilter(ExportDump);

  // COMの初期化
  CoInitializeEx(0, COINIT_MULTITHREADED);

  D3DResourceLeakChecker leakCheck; // リソースリークチェック用のオブジェクト

  // ログを用意
  Logger::InitializeFileLogging();

#pragma region 基盤システムの初期化

#pragma region WindowsAPIの初期化
  // WindowsAPIの初期化

  // ポインタ
  Win32Window *window = nullptr;

  // WindowWrapperの生成と初期化
  window = new Win32Window();
  window->Initialize();

#pragma endregion WindowsAPIの初期化ここまで

#pragma region DirectXの初期化
  // DirectXの初期化

  // ポインタ
  DX12Context *dxBase = nullptr;
  // DirectXBaseの生成と初期化
  dxBase = new DX12Context();
  dxBase->Initialize(window);

#pragma endregion DirectXの初期化ここまで

#pragma region PipelineManagerの生成と初期化
  // PipelineManagerの生成と初期化

  // ポインタ
  PipelineManager *pipelineManager = nullptr;

  // PipelineManagerの初期化
  pipelineManager = new PipelineManager();
  pipelineManager->Initialize(dxBase);

#pragma endregion PipelineManagerの生成と初期化ここまで

#pragma region SRVManagerの生成と初期化
  // SRVManagerの生成と初期化

  // ポインタ
  SrvManager *srvManager = nullptr;

  // SRVManagerの初期化
  srvManager = new SrvManager();
  srvManager->Initialize(dxBase);

#pragma endregion SRVManagerの生成と初期化ここまで

#pragma region スプライト共通部の生成と初期化
  // スプライト共通部の初期化

  // ポインタ
  SpriteCommon *spriteCommon = nullptr;

  // スプライト共通部の初期化
  spriteCommon = new SpriteCommon();
  spriteCommon->Initialize(dxBase, pipelineManager);

#pragma endregion スプライト共通部の生成と初期化ここまで

#pragma region 3Dオブジェクト共通部の生成と初期化
  // 3Dオブジェクト共通部の初期化

  // ポインタ
  Object3dCommon *object3dCommon = nullptr;

  // 3Dオブジェクト共通部の初期化
  object3dCommon = new Object3dCommon();
  object3dCommon->Initialize(dxBase, pipelineManager);

#pragma endregion 3Dオブジェクト共通部の生成と初期化ここまで

#pragma region DirectInputの初期化
  // DirectInputの初期化

  Input *input = new Input();
  input->Initialize(window);

#pragma endregion DirectInputの初期化ここまで

#pragma region DebugCameraの初期化
  // DebugCameraの初期化

  DebugCamera debugCamera;
  debugCamera.Initialize();

#pragma endregion DebugCameraの初期化ここまで

#pragma region TextureManagerの初期化
  // TextureManagerの初期化

  TextureManager::GetInstance()->Initialize(dxBase, srvManager);
  uint32_t testTextureSrvIndex = srvManager->GetNewIndex();

#pragma endregion TextureManagerの初期化ここまで

#pragma region ModelManagerの初期化
  // ModelManagerの初期化

  ModelManager::GetInstance()->Initialize(dxBase);

#pragma endregion ModelManagerの初期化ここまで

#pragma region ParticleManagerの初期化
  // ParticleManagerの初期化

  ParticleManager::GetInstance()->Initialize(dxBase, srvManager,
                                             pipelineManager);
  uint32_t testParticleSrvIndex = srvManager->GetNewIndex();
#pragma endregion ParticleManagerの初期化ここまで

#pragma region ImGuiManagerの初期化
  // ImGuiManagerの初期化
  ImGuiManager *imGuiManager = nullptr;
  imGuiManager = new ImGuiManager();
  imGuiManager->Initialize(dxBase, window, srvManager);
  uint32_t testImGuiSrvIndex = srvManager->GetNewIndex();
#pragma endregion ImGuiManagerの初期化ここまで

#pragma endregion 基盤システムの初期化ここまで

#pragma region 最初のシーンの初期化
  // 最初のシーンの初期化

#pragma region カメラ
  // カメラ

  // 生成
  Camera *camera = new Camera();
  camera->Initialize(dxBase);
  camera->SetRotate({0.0f, 0.0f, 0.0f});
  camera->SetTranslate({0.0f, 0.0f, -10.0f});
  object3dCommon->SetDefaultCamera(camera);

  Vector3 gameCameraRotate = camera->GetRotate();
  Vector3 gameCameraTranslate = camera->GetTranslate();

  bool useDebugCamera = false;

#pragma endregion カメラここまで

#pragma region モデルの読み込みとアップロード
  // モデルの読み込みとアップロード

  // モデルのパスを保持
  const std::string planeModel = ("plane.obj");
  //const std::string axisModel = ("axis.obj");
  const std::string teapotModel = ("teapot.obj");

  // .objファイルからモデルを呼び込む
  ModelManager::GetInstance()->LoadModel("Plane", planeModel);
  //ModelManager::GetInstance()->LoadModel("Axis", axisModel);
  ModelManager::GetInstance()->LoadModel("Teapot", teapotModel);

#pragma endregion モデルの読み込みとアップロードここまで

#pragma region パーティクル

  // パーティクルグループの作成
  const std::string particleTexturePath = "circle.png";
  const std::string particleGroupName = "default";
  TextureManager::GetInstance()->LoadTexture(particleTexturePath);
  ParticleManager::GetInstance()->CreateParticleGroup(particleGroupName,
                                                      particleTexturePath);

  // エミッターの設定と登録
  ParticleEmitter defaultEmitter(
      Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}},
      3,   // count: 1回で3個生成
      0.2f // frequency: 0.2秒ごとに発生
  );

  ParticleManager::GetInstance()->SetEmitter(particleGroupName, defaultEmitter);

  // 回転
  Vector3 particleRotation{};

  // 更新
  bool isUpdate = true;

  // ビルボードの適用
  bool useBillboard = true;

  // 画像の変更(particle,)
  bool changeTexture = true;

  // パーティクルの生成
  // bool generateParticle = false;

#pragma endregion パーティクルここまで

#pragma region テクスチャの読み込みとアップロード
  // テクスチャの読み込みとアップロード

  // テクスチャファイルパスを保持
  const std::string uvCheckerPath = "uvChecker.png";
  TextureManager::GetInstance()->LoadTexture(uvCheckerPath);

  const std::string monsterBallPath = "monsterBall.png";
  TextureManager::GetInstance()->LoadTexture(monsterBallPath);

  // コマンド実行と完了待機
  dxBase->ExecuteInitialCommandAndSync();
  TextureManager::GetInstance()->ReleaseIntermediateResources();
  ParticleManager::GetInstance()->ReleaseIntermediateResources();

#pragma endregion テクスチャの読み込みとアップロードここまで(コマンド実行済み)

#pragma region サウンドの初期化
  // サウンドの初期化

  // サウンド用の宣言
  AudioManager audioManager;
  // 1. AudioManager を初期化
  if (!audioManager.Initialize()) {
    // 初期化失敗時の処理
    return 1;
  }

  // 2. 音声ファイルをロード
  if (!audioManager.LoadSound("alarm1", "Alarm01.mp3")) {
    // ロード失敗時の処理
    return 1;
  }

#pragma endregion サウンドの初期化ここまで

#pragma region マテリアル
  // マテリアル

  // 1つ目のオブジェクト
  Object3d *object3d_1 = new Object3d(); // object3dをobject3d_1に変更
  object3d_1->Initialize(object3dCommon);

  // モデルの設定
  object3d_1->SetModel(planeModel);
  // オブジェクトの位置を設定
  object3d_1->SetTranslate({-2.0f, 0.0f, 0.0f});

  // 2つ目のオブジェクトを新規作成
  Object3d *object3d_2 = new Object3d();
  object3d_2->Initialize(object3dCommon);

  // モデルの設定
  object3d_2->SetModel(teapotModel);
  // オブジェクトの位置を設定
  object3d_2->SetTranslate({2.0f, 0.0f, 0.0f});

  // Object3dを格納するstd::vectorを作成
  std::vector<Object3d *> object3ds;
  object3ds.push_back(object3d_1);
  object3ds.push_back(object3d_2);

  // 表示
  bool showMaterial = true;

#pragma endregion マテリアルここまで

  // ブレンドモード
  int currentBlendMode = kBlendModeNormal;
  int particleBlendMode = kBlendModeAdd;

#pragma region スプライト
  // スプライト

  // 描画サイズ
  const float spriteCutSize = 64.0f;
  const float spriteScale = 64.0f;
  // 生成
  std::vector<Sprite *> sprites;
  for (uint32_t i = 0; i < 10; ++i) {
    Sprite *newSprite = new Sprite();
    newSprite->Initialize(spriteCommon, uvCheckerPath);
    newSprite->SetAnchorPoint({0.5f, 0.5f});
    if (i == 7) {
      newSprite->SetFlipX(1);
      newSprite->SetFlipY(1);
    } else if (i == 8) {
      newSprite->SetFlipX(1);
      newSprite->SetTextureLeftTop({0.0f, 0.0f});
      newSprite->SetTextureSize({spriteCutSize, spriteCutSize});
      newSprite->SetScale({spriteScale, spriteScale});
    } else if (i == 9) {
      newSprite->SetTextureLeftTop({0.0f, 0.0f});
      newSprite->SetTextureSize({spriteCutSize, spriteCutSize});
      newSprite->SetScale({spriteScale, spriteScale});
    }
    newSprite->SetTranslate(
        {float(i * spriteScale / 3), float(i * spriteScale / 3)});
    sprites.push_back(newSprite);
  }

  // 表示
  bool showSprite = false;

#pragma endregion スプライトここまで

#pragma endregion 最初のシーンの初期化ここまで

  // ウィンドウのxボタンが押されるまでループ
  // 1. ウィンドウメッセージ処理
  while (true) {
    // Windowのメッセージ処理
    if (window->ProcessMessage()) {
      // ゲームループを抜ける
      break;
    }

    // 描画前処理
    dxBase->PreDraw();
    srvManager->PreDraw();

    // DIrectX更新処理

    // フレームの開始を告げる
    imGuiManager->Begin();

#pragma region UI処理

#ifdef USE_IMGUI

    // 開発用UIの処理
    // ImGui::ShowDemoWindow();
    // 設定ウィンドウ(画面右側)
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f),
                            ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::Begin("Settings");
    // デバッグウィンドウ

    // グローバル設定
    ImGui::Separator();
    if (ImGui::TreeNode("Global Settings")) {
      ImGui::Checkbox("useDebugCamera", &useDebugCamera);
      ImGui::Combo("BlendMode", &currentBlendMode,
                   "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("GameCamera")) {

      ImGui::DragFloat3("rotate", &gameCameraRotate.x, 0.01f);
      ImGui::DragFloat3("translate", &gameCameraTranslate.x, 0.01f);

      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Object3d")) {
      ImGui::Checkbox("showMaterial", &showMaterial);

      // ここからループ処理
      for (size_t i = 0; i < object3ds.size(); ++i) {
        Object3d *object3d = object3ds[i]; // 現在のオブジェクトを取得

        // ノードのラベルを動的に生成
        std::string label = "Object3d " + std::to_string(i + 1);

        if (ImGui::TreeNode(label.c_str())) {
          Vector3 scale = object3d->GetScale();
          Vector3 rotate = object3d->GetRotation();
          Vector3 translate = object3d->GetTranslate();

          ImGui::DragFloat3("scale", &scale.x, 0.01f);
          ImGui::DragFloat3("rotate", &rotate.x, 0.01f);
          ImGui::DragFloat3("translate", &translate.x, 0.01f);

          object3d->SetScale(scale);
          object3d->SetRotation(rotate);
          object3d->SetTranslate(translate);

          ImGui::Separator();

          if (ImGui::TreeNode(("Light_" + std::to_string(i + 1)).c_str())) {
            Vector4 color = object3d->GetDirectionalLightColor();
            Vector3 direction = object3d->GetDirectionalLightDirection();
            float intensity = object3d->GetDirectionalLightIntensity();

            ImGui::ColorEdit4("colorLight", &color.x);
            ImGui::DragFloat3("directionLight", &direction.x, 0.01f);
            ImGui::DragFloat("intensityLight", &intensity, 0.01f);

            object3d->SetDirectionalLightColor(color);
            object3d->SetRotation(rotate);
            object3d->SetDirectionalLightDirection(direction);
            object3d->SetDirectionalLightIntensity(intensity);

            ImGui::TreePop();
          }
          ImGui::TreePop();
        }
      }
      // ループ処理ここまで

      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Particle")) {
      ImGui::Checkbox("update", &isUpdate);
      ImGui::Checkbox("useBillboard", &useBillboard);
      // ImGui::Checkbox("changeTexture", &changeTexture);
      ImGui::SliderFloat3("rotateParticleZ", &particleRotation.x, -6.28f,
                          6.28f);
      if (ImGui::Button("Generate Particle(SPACE)")) {
        // エミッターの Transform, count を直接指定して、一度だけEmitを呼び出す

        // 例: プレイヤー位置 {10.0f, 0.0f, 0.0f}
        // から、10個のパーティクルを一度に生成
        Vector3 playerPos = {0.0f, 0.0f, 0.0f};
        uint32_t burstCount = 10;

        ParticleManager::GetInstance()->Emit(particleGroupName, playerPos,
                                             burstCount);
      }
      ImGui::Combo("ParticleBlendMode", &particleBlendMode,
                   "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Sprite")) {

      ImGui::Checkbox("showSprite", &showSprite);

      for (size_t i = 0; i < sprites.size(); ++i) {
        std::string label = "Sprite " + std::to_string(i);
        if (ImGui::TreeNode(label.c_str())) {
          Sprite *sprite = sprites[i];

          Vector2 spritePosition = sprite->GetTranslate();
          float spriteRotation = sprite->GetRotation();
          Vector2 spriteScale = sprite->GetScale();
          Vector4 spriteColor = sprite->GetColor();

          ImGui::SliderFloat2("translateSprite", &spritePosition.x, 0.0f,
                              1000.0f);
          ImGui::SliderFloat("rotateSprite", &spriteRotation, -6.28f, 6.28f);
          ImGui::SliderFloat2("scaleSprite", &spriteScale.x, 1.0f,
                              Win32Window::kClientWidth);
          ImGui::ColorEdit4("colorSprite", &spriteColor.x);
          sprite->SetTranslate(spritePosition);
          sprite->SetRotation(spriteRotation);
          sprite->SetScale(spriteScale);
          sprite->SetColor(spriteColor);

          ImGui::Separator();
          ImGui::Text("UV");

          // UVTransform用の変数を用意
          Transform uvTransformSprite = {};
          uvTransformSprite.translate = sprite->GetUvTranslate();
          uvTransformSprite.scale = sprite->GetUvScale();
          uvTransformSprite.rotate = sprite->GetUvRotation();

          ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x,
                            0.01f, 0.0f, 1.0f);
          ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f,
                            -10.0f, 10.0f);
          ImGui::SliderAngle("uvRotate", &uvTransformSprite.rotate.z);
          sprite->SetUvTranslate(uvTransformSprite.translate);
          sprite->SetUvScale(uvTransformSprite.scale);
          sprite->SetUvRotation(uvTransformSprite.rotate);
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Sound")) {
      if (ImGui::Button("Play Sound")) {
        // サウンドの再生
        audioManager.PlaySound("alarm1");
      }
      ImGui::TreePop();
    }

    ImGui::End();

    // gameCameraモード用の操作説明(画面左上)
    if (!useDebugCamera) {
      ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
      ImGui::Begin("Game Camera Control");
      ImGui::Text("A/D: Left/Right");
      ImGui::Text("W/S: Up/Down");
      ImGui::Text("Q/E: Forward/Backward");
      ImGui::Text("Z/C: Rotate Left/Right");
      ImGui::Text("X: Reset Rotation");
      ImGui::End();
    }

#endif

#pragma endregion UI処理ここまで

    // ゲームの処理-----------------------------------------------------------------------------------

#pragma region 更新処理

    // inputを更新
    input->Update();

    audioManager.CleanupFinishedVoices(); // 完了した音声のクリーンアップ

    // カメラの更新

    if (!useDebugCamera) { // ※カメラマネージャーを作っておく

      if (input->IsKeyDown(DIK_A)) { // Aキーが押されている間は左移動
        gameCameraTranslate.x -= 0.1f;
      }
      if (input->IsKeyDown(DIK_D)) { // Dキーが押されている間は右移動
        gameCameraTranslate.x += 0.1f;
      }
      if (input->IsKeyDown(DIK_W)) { // Wキーが押されている間は上移動
        gameCameraTranslate.y += 0.1f;
      }
      if (input->IsKeyDown(DIK_S)) { // Sキーが押されている間は下移動
        gameCameraTranslate.y -= 0.1f;
      }
      if (input->IsKeyDown(DIK_Q)) { // Qキーが押されている間は前進
        gameCameraTranslate.z += 0.1f;
      }
      if (input->IsKeyDown(DIK_E)) { // Eキーが押されている間は後退
        gameCameraTranslate.z -= 0.1f;
      }

      if (input->IsKeyDown(DIK_C)) { // Cキーが押されている間は右に回転
        gameCameraRotate.y += 0.02f;
      }
      if (input->IsKeyDown(DIK_Z)) { // Zキーが押されている間は左に回転
        gameCameraRotate.y -= 0.02f;
      }
      if (input->IsKeyReleased(DIK_X)) { // Xキーが離されたら回転をリセット
        gameCameraRotate.y = 0.0f;
      }
    }

    if (useDebugCamera) {
      // デバッグカメラ
      debugCamera.Update(*input); // ※操作が微妙なので調整しておく

      camera->SetRotate(debugCamera.GetRotation());
      camera->SetTranslate(debugCamera.GetTranslate());
    } else {
      // カメラ
      camera->SetRotate(gameCameraRotate);
      camera->SetTranslate(gameCameraTranslate);
    }

    camera->Update();

    // ----------------------------------------------------
    // パーティクルシステム全体の更新
    // ----------------------------------------------------

    // 頻度計算、パーティクル実体の更新、寿命チェック、GPUデータ転送の全てが
    // ParticleManager::Updateの中で行われる。
    ParticleManager::GetInstance()->Update(*camera, isUpdate, useBillboard);

    // ----------------------------------------------------
    // スペースキーでの手動 Emit（デバッグ/テスト用）
    // ----------------------------------------------------
    if (input->IsKeyTriggered(DIK_SPACE)) {
      // エミッターの Transform, count を直接指定して、一度だけEmitを呼び出す

      // 例: プレイヤー位置 {10.0f, 0.0f, 0.0f}
      // から、10個のパーティクルを一度に生成
      Vector3 playerPos = {0.0f, 0.0f, 0.0f};
      uint32_t burstCount = 10;

      ParticleManager::GetInstance()->Emit(particleGroupName, playerPos,
                                           burstCount);
    }

    // オブジェクト
    object3d_1->Update();
    object3d_2->Update();

    // スプライト
    for (size_t i = 0; i < sprites.size(); ++i) {
      sprites[i]->Update();
    }

#pragma endregion 更新処理ここまで

    imGuiManager->End(); // ImGuiのフレーム終了処理

    ////ここまで----------------------------------------------------------------------------------------

    //// 3. グラフィックコマンド

    ////
    /// 描画の処理-------------------------------------------------------------------------------------

#pragma region 描画処理

    // コマンドを積む

    /// object3dの描画---------------------------------------

    // object3d用の設定
    object3dCommon->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode), pipelineManager);

    // 描画！（DrawCall/ドローコール）
    if (showMaterial) {
      object3d_1->Draw();
      object3d_2->Draw();
    }

    /// Particle描画---------------------------------------

    // 描画！（DrawCall/ドローコール）
    ParticleManager::GetInstance()->Draw(
        static_cast<BlendState>(particleBlendMode));

    /// スプライト-------------------------------
    // スプライト用の設定
    spriteCommon->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode), pipelineManager);
    // 描画！（DrawCall/ドローコール）
    if (showSprite) {
      for (size_t i = 0; i < sprites.size(); ++i) {

        if (i == 6) {
          // monsterBallテクスチャを適用
          sprites[i]->SetTexture(monsterBallPath);
        }

        // 描画
        sprites[i]->Draw();
      }
    }

    // ImGuiの描画
    imGuiManager->Draw();

    // 描画後処理
    dxBase->PostDraw();

#pragma endregion 描画処理ここまで
  }

#pragma region 解放処理

  //// 解放処理
  // CloseHandle(fenceEvent);

  if (input) {
    delete input;
    input = nullptr;
  }

  // ImGuiの終了処理
  imGuiManager->Finalize();
  delete imGuiManager;
  imGuiManager = nullptr;

  // ParticleManagerの終了
  ParticleManager::GetInstance()->Finalize();

  // ModelManagerの終了
  ModelManager::GetInstance()->Finalize();

  // TextureManagerの終了
  TextureManager::GetInstance()->Finalize();

  // Object3dの解放
  delete object3d_1;
  object3d_1 = nullptr;
  delete object3d_2;
  object3d_2 = nullptr;

  // Object3dCommonの解放
  delete object3dCommon;
  object3dCommon = nullptr;

  // Spriteの解放
  for (size_t i = 0; i < sprites.size(); ++i) {
    delete sprites[i];
    sprites[i] = nullptr;
  }

  // cameraの解放
  delete camera;
  camera = nullptr;

  // spriteCommonの解放
  delete spriteCommon;
  spriteCommon = nullptr;

  // PipelineManagerの解放
  if (pipelineManager) {
    delete pipelineManager;
    pipelineManager = nullptr;
  }

  // SrvManagerの解放
  if (srvManager) {
    delete srvManager;
    srvManager = nullptr;
  }

  // DerectXの解放
  delete dxBase;
  dxBase = nullptr;

  // WindowsAPIの終了処理
  window->Finalize();

  // WindowsAPI解放
  delete window;
  window = nullptr;

#pragma endregion 解放処理ここまで

  return 0;
}