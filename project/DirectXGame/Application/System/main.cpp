#include <Windows.h>
#include <cassert>
#include <dbghelp.h> // dump
#include <dxgi1_6.h> // dump
#include <strsafe.h> // dump

// #include <vector>
#include <numbers> //particleに使用
#include <random>  //particleに使用

// エンジン
#include "D3DResourceLeakChecker.h"
#include "DX12Context.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Logger.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Win32Window.h"
// 管理系
#include "AudioManager.h"
#include "ModelManager.h"
#include "PipelineManager.h"
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

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

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

#pragma region RootSignature作成
  // RootSignature作成

#pragma region 共通のSampler設定
  // 共通のSamplerの設定
  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
  staticSamplers[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;    // PixelShaderで使う
  staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う

#pragma endregion 共通のSampler設定ここまで

#pragma region Particle用のRootSignature作成
  // Particle用のRootSignature作成

  // Particle用 SRV (Vertex Shader用)
  D3D12_DESCRIPTOR_RANGE particleInstancingSrvRange[1] = {};
  particleInstancingSrvRange[0].BaseShaderRegister = 0; // t0
  particleInstancingSrvRange[0].NumDescriptors = 1;     // 数は1つ
  particleInstancingSrvRange[0].RangeType =
      D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  particleInstancingSrvRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // テクスチャ用 SRV (Pixel Shader用)
  D3D12_DESCRIPTOR_RANGE particleTextureSrvRange[1] = {};
  particleTextureSrvRange[0].BaseShaderRegister = 0; // t0 (Texture)
  particleTextureSrvRange[0].NumDescriptors = 1;     // 数は1つ
  particleTextureSrvRange[0].RangeType =
      D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  particleTextureSrvRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
  D3D12_ROOT_PARAMETER particleRootParameters[3] = {};

  // Root Parameter 0: Pixel Shader用 Material CBV (b0)
  particleRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  particleRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  particleRootParameters[0].Descriptor.ShaderRegister = 0; // b0

  // Root Parameter 1: Vertex Shader用 Instancing SRV (t0)
  particleRootParameters[1].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  particleRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  particleRootParameters[1].DescriptorTable.pDescriptorRanges =
      particleInstancingSrvRange;
  particleRootParameters[1].DescriptorTable.NumDescriptorRanges =
      _countof(particleInstancingSrvRange);

  // Root Parameter 2: Pixel Shader用 Texture SRV (t0)
  particleRootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  particleRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  particleRootParameters[2].DescriptorTable.pDescriptorRanges =
      particleTextureSrvRange;
  particleRootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(particleTextureSrvRange);

  // Particle用のRootSignatureDesc
  D3D12_ROOT_SIGNATURE_DESC particleRootSignatureDesc{};
  particleRootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  particleRootSignatureDesc.pParameters =
      particleRootParameters; // ルートパラメータ配列へのポインタ
  particleRootSignatureDesc.NumParameters =
      _countof(particleRootParameters); // 配列の長さ (3)
  particleRootSignatureDesc.pStaticSamplers = staticSamplers;
  particleRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズしてバイナリにする
  ComPtr<ID3D12RootSignature> particleRootSignature = nullptr;
  ComPtr<ID3DBlob> particleSignatureBlob = nullptr;
  ComPtr<ID3DBlob> particleErrorBlob = nullptr;

  HRESULT hr;

  hr = D3D12SerializeRootSignature(&particleRootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1,
                                   &particleSignatureBlob, &particleErrorBlob);
  if (FAILED(hr)) {
    Logger::Log(
        reinterpret_cast<char *>(particleErrorBlob->GetBufferPointer()));
    assert(false);
  }
  // バイナリを元に生成
  hr = dxBase->GetDevice()->CreateRootSignature(
      0, particleSignatureBlob->GetBufferPointer(),
      particleSignatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&particleRootSignature));
  assert(SUCCEEDED(hr));

#pragma endregion Particle用のRootSignature作成ここまで

#pragma endregion RootSignature作成ここまで

#pragma region パイプラインステート作成用設定

  // InputLayout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].InputSlot = 0; // 明示的に指定
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[0].InputSlotClass =
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
  inputElementDescs[0].InstanceDataStepRate =
      0; // デフォルト値だが明示するとより分かりやすい
  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].InputSlot = 0; // 明示的に指定
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[1].InputSlotClass =
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
  inputElementDescs[1].InstanceDataStepRate = 0;
  inputElementDescs[2].SemanticName = "NORMAL";
  inputElementDescs[2].SemanticIndex = 0;
  inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  inputElementDescs[2].InputSlot = 0; // 明示的に指定
  inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[2].InputSlotClass =
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
  inputElementDescs[2].InstanceDataStepRate = 0;
  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDescs;
  inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion パイプラインステート作成用設定ここまで

#pragma region RasterizerState作成
  // RasterizerState(particle)
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  // 裏面をカリング
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  // 三角形の中を塗りつぶす
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

#pragma endregion RasterizerState作成ここまで

#pragma region Shaderコンパイル
  // Shaderをコンパイル

  // パーティクル用
  ComPtr<IDxcBlob> particleVSBlob =
      dxBase->CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
  ComPtr<IDxcBlob> particlePSBlob =
      dxBase->CompileShader(L"Particle.PS.hlsl", L"ps_6_0");
  assert(particleVSBlob != nullptr && particlePSBlob != nullptr);

#pragma endregion Shaderコンパイルここまで

#pragma region DepthStencilState設定
  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  // Depthの機能を有効化する
  depthStencilDesc.DepthEnable = true;
  // 書き込みします
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  // 比較関数はLessEqual。つまり、近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

#pragma endregion DepthStencilState設定ここまで

#pragma region PSO作成
  /// ベースのPSO設定----------------------------------

#pragma region Particle用PSO共通設定

  // パーティクル用PSO共通設定
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticleBase{};

  // **共通設定**を一度だけセット
  psoDescParticleBase.pRootSignature =
      particleRootSignature.Get();                   // RootSignature
  psoDescParticleBase.InputLayout = inputLayoutDesc; // InputLayout
  psoDescParticleBase.VS = {particleVSBlob->GetBufferPointer(),
                            particleVSBlob->GetBufferSize()}; // VertexShader
  psoDescParticleBase.PS = {particlePSBlob->GetBufferPointer(),
                            particlePSBlob->GetBufferSize()}; // PixelShader
  psoDescParticleBase.RasterizerState = rasterizerDesc;       // RasterizerState
  // 書き込むRTVの情報
  psoDescParticleBase.NumRenderTargets = 1;
  psoDescParticleBase.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  psoDescParticleBase.SampleDesc.Count = 1;
  // DepthStencilの設定
  psoDescParticleBase.DepthStencilState = depthStencilDesc;
  psoDescParticleBase.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  // 利用するトポロジ（形状）のタイプ。三角形
  psoDescParticleBase.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // どのように画面に色を打ち込むかの設定（気にしなくていい）
  psoDescParticleBase.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // 外部で宣言（クラスのメンバー変数など）
  ComPtr<ID3D12PipelineState> particlePsoArray[kCountOfBlendMode];

#pragma endregion Particle用PSO共通設定ここまで

#pragma region ブレンドモードごとのPSO生成
  // ループで各ブレンドモードのPSOを生成

  for (int i = 0; i < kCountOfBlendMode; ++i) {
    // ----------------------------------------------------
    // Particle用 PSO 生成 (ブレンド設定適用)
    // ----------------------------------------------------
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticle = psoDescParticleBase;
    std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs =
        CreateBlendStateDescs();

    // Particleはアルファブレンドを多用するため、Depth書き込みは無効化
    if (i != kBlendModeNone) { // ブレンドモードがNoneでない場合
      // アルファブレンドを行う場合はDepthへの書き込みを無効にする（順番依存を許容するため）
      psoDescParticle.DepthStencilState.DepthWriteMask =
          D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    // ブレンド設定を適用
    psoDescParticle.BlendState = blendDescs[i];

    // PSO生成
    hr = dxBase->GetDevice()->CreateGraphicsPipelineState(
        &psoDescParticle, IID_PPV_ARGS(&particlePsoArray[i]));
    assert(SUCCEEDED(hr));
  }

#pragma endregion ブレンドモードごとのPSO生成ここまで

#pragma endregion PSO作成ここまで

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

  TextureManager::GetInstance()->Initialize(dxBase);

#pragma endregion TextureManagerの初期化ここまで

#pragma region ModelManagerの初期化
  // ModelManagerの初期化

  ModelManager::GetInstance()->Initialize(dxBase);

#pragma endregion ModelManagerの初期化ここまで

#pragma region Particle用リソース作成
  // Particle用リソース作成

  // Particle最大数
  const uint32_t kNumMaxParticle = 100;
  // Particle用のTransformationMatrixリソースを作る
  ComPtr<ID3D12Resource> particleResource = dxBase->CreateBufferResource(
      sizeof(ParticleInstanceData) * kNumMaxParticle);
  // 書き込むためのアドレスを取得
  ParticleInstanceData *particleData = nullptr;
  particleResource->Map(0, nullptr, reinterpret_cast<void **>(&particleData));
  // 単位行列を書き込んでおく
  for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
    particleData[index].WVP = MakeIdentity4x4();
    particleData[index].World = MakeIdentity4x4();
    particleData[index].color = {1.0f, 1.0f, 1.0f, 1.0f};
  }

  D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU =
      dxBase->CreateStructuredBufferSRV(particleResource, kNumMaxParticle,
                                        sizeof(ParticleInstanceData), 4);

  // ランダムエンジンの初期化(パーティクル用)
  static std::random_device particleSeedGenerator;
  static std::mt19937 particleRandomEngine(particleSeedGenerator());

  // ランダムの範囲
  std::uniform_real_distribution<float> particleDist(-1.0f, 1.0f);

  // パーティクルのリスト
  std::list<Particle> particles;

#pragma endregion Particle用リソース作成ここまで

#pragma endregion 基盤システムの初期化ここまで

#pragma region 最初のシーンの初期化

#pragma region モデルの読み込みとアップロード
  // モデルの読み込みとアップロード

  // モデルのパスを保持
  const std::string planeModel = ("plane.obj");
  const std::string axisModel = ("axis.obj");

  // .objファイルからモデルを呼び込む
  ModelManager::GetInstance()->LoadModel("plane", planeModel);
  ModelManager::GetInstance()->LoadModel("axis", axisModel);

#pragma endregion モデルの読み込みとアップロードここまで

#pragma region テクスチャの読み込みとアップロード
  // テクスチャの読み込みとアップロード

  // テクスチャインデックスを保持
  uint32_t uvCheckerIndex =
      TextureManager::GetInstance()->LoadTexture("uvChecker.png");
  uint32_t monsterBallIndex =
      TextureManager::GetInstance()->LoadTexture("monsterBall.png");
  uint32_t particleTextureIndex =
      TextureManager::GetInstance()->LoadTexture("circle.png");

  // コマンド実行と完了待機
  dxBase->ExecuteInitialCommandAndSync();
  TextureManager::GetInstance()->ReleaseIntermediateResources();

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
  if (!audioManager.LoadSound("alarm1", "Alarm01.wav")) {
    // ロード失敗時の処理
    return 1;
  }

#pragma endregion サウンドの初期化ここまで
  // 最初のシーンの初期化

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
  object3d_2->SetModel(axisModel);
  // オブジェクトの位置を設定
  object3d_2->SetTranslate({2.0f, 0.0f, 0.0f});

  // Object3dを格納するstd::vectorを作成
  std::vector<Object3d *> object3ds;
  object3ds.push_back(object3d_1);
  object3ds.push_back(object3d_2);

  // 表示
  bool showMaterial = true;

#pragma endregion マテリアルここまで

#pragma region カメラ
  // カメラ
  Vector3 gameCameraRotate = object3d_1->GetCameraRotation();
  Vector3 gameCameraTranslate = object3d_1->GetCameraTranslate();

  bool useDebugCamera = true;
  bool isSavedCamera = true;

#pragma endregion カメラここまで

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
    newSprite->Initialize(spriteCommon, uvCheckerIndex);
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

#pragma region パーティクル

  // ※カリングがoffになっていないのなら表裏を反転し、billboardMatrixの初期化、更新時にbackToFrontMatrixを掛ける
  // Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
  // カメラの回転を適用する
  Matrix4x4 billboardMatrix = debugCamera.GetWorldMatrix();
  billboardMatrix.m[3][0] = 0.0f; // 平行移動成分はいらない
  billboardMatrix.m[3][1] = 0.0f;
  billboardMatrix.m[3][2] = 0.0f;

  Emitter emitter{};
  emitter.count = 3;
  emitter.frequency = 0.2f;     // 0.2秒ごとに発生
  emitter.frequencyTime = 0.0f; // 発生頻度用の時刻、0で初期化
  emitter.transform.translate = {0.0f, 0.0f, 0.0f};
  emitter.transform.rotate = {0.0f, 0.0f, 0.0f};
  emitter.transform.scale = {1.0f, 1.0f, 1.0f};

  // 回転
  Vector3 particleRotation{};

  // 更新
  bool isUpdate = true;

  // ビルボードの適用
  bool useBillboard = true;

  // 画像の変更(particle,)
  bool changeTexture = true;

  // パーティクルの生成
  bool generateParticle = false;

#pragma region フィールドの設定

  AccelerationField accelerationField;
  accelerationField.acceleration = {15.0f, 0.0f, 0.0f};
  accelerationField.area.min = {-1.0f, -1.0f, -1.0f};
  accelerationField.area.max = {1.0f, 1.0f, 1.0f};

#pragma endregion フィールドの設定ここまで

#pragma endregion パーティクルここまで

#pragma endregion 最初のシーンの初期化ここまで

  // ウィンドウのxボタンが押されるまでループ
  // 1. ウィンドウメッセージ処理
  while (true) {
    // Windowのメッセージ処理
    if (window->ProcessMessage()) {
      // ゲームループを抜ける
      break;
    }

    // DIrectX更新処理

    // フレームの開始を告げる
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#pragma region UI処理
    // 開発用UIの処理
    // ImGui::ShowDemoWindow();
    // 設定ウィンドウ(画面右側)
    ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f),
                            ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::Begin("Settings");
    // デバッグウィンドウ
    // ImGui::Text("Camera");

    // ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x,
    // 0.01f); ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x,
    // 0.01f);

    // グローバル設定
    ImGui::Separator();
    if (ImGui::TreeNode("Global Settings")) {
      ImGui::Checkbox("useDebugCamera", &useDebugCamera);
      ImGui::Combo("BlendMode", &currentBlendMode,
                   "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
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
          // Vector4 color = object3d->GetColor();
          // uint32_t enableLighting = object3d->IsEnableLighting();

          ImGui::DragFloat3("scale", &scale.x, 0.01f);
          ImGui::DragFloat3("rotate", &rotate.x, 0.01f);
          ImGui::DragFloat3("translate", &translate.x, 0.01f);

          // ImGui::ColorEdit4("color", &color.x);

          // ImGui::CheckboxFlags("enableLighting", &enableLighting, 1 << 0);

          object3d->SetScale(scale);
          object3d->SetRotation(rotate);
          object3d->SetTranslate(translate);
          // object3d->SetColor(color);
          // object3d->SetEnableLighting(enableLighting);

          ImGui::Separator();

          if (ImGui::TreeNode(("Light_" + std::to_string(i + 1)).c_str())) {
            Vector4 color = object3d->GetDirectionalLightColor();
            Vector3 direction = object3d->GetDirectionalLightDirection();
            float intensity = object3d->GetDirectionalLightIntensity();

            ImGui::ColorEdit4("colorLight", &color.x);
            ImGui::DragFloat3("directionLight", &direction.x, 0.01f);
            ImGui::DragFloat("intensityLight", &intensity, 0.01f);

            object3d->SetDirectionalLightColor(color);
            // object3d->SetRotation(rotate);
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
      ImGui::Checkbox("changeTexture", &changeTexture);
      ImGui::SliderFloat("EmitterFrequency", &emitter.frequency, 0.01f, 1.0f);
      ImGui::DragFloat3("EmitterTranslate", &emitter.transform.translate.x,
                        0.01f, -100.0f, 100.0f);
      ImGui::SliderFloat3("rotateParticleZ", &particleRotation.x, -6.28f,
                          6.28f);
      if (ImGui::Button("Generate Particle(SPACE)")) {
        generateParticle = true;
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

    // 移動モード用の操作説明(画面左上)
    if (!useDebugCamera) {
      ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
      ImGui::Begin("Control Material Mode");
      ImGui::Text("A/D: Left/Right");
      ImGui::Text("W/S: Up/Down");
      ImGui::Text("Q/E: Forward/Backward");
      ImGui::Text("UP/DOWN: Scale Up/Down");
      ImGui::Text("C/Z: Rotate Left/Right");
      ImGui::Text("X: Reset Rotation");
      ImGui::End();
    }

#pragma endregion UI処理ここまで

    // ゲームの処理-----------------------------------------------------------------------------------

#pragma region 更新処理

    // inputを更新
    input->Update();

    // キーボードの入力処理

    audioManager.CleanupFinishedVoices(); // 完了した音声のクリーンアップ

    if (!useDebugCamera) {

      Vector3 scale = object3d_1->GetScale();
      Vector3 rotate = object3d_1->GetRotation();
      Vector3 translate = object3d_1->GetTranslate();

      if (input->IsKeyDown(DIK_A)) { // Aキーが押されている間は左移動
        translate.x -= 0.1f;
      }
      if (input->IsKeyDown(DIK_D)) { // Dキーが押されている間は右移動
        translate.x += 0.1f;
      }
      if (input->IsKeyDown(DIK_W)) { // Wキーが押されている間は上移動
        translate.y += 0.1f;
      }
      if (input->IsKeyDown(DIK_S)) { // Sキーが押されている間は下移動
        translate.y -= 0.1f;
      }
      if (input->IsKeyDown(DIK_Q)) { // Qキーが押されている間は前進
        translate.z += 0.1f;
      }
      if (input->IsKeyDown(DIK_E)) { // Eキーが押されている間は後退
        translate.z -= 0.1f;
      }

      if (input->IsKeyDown(DIK_UP)) { // 上キーが押されている間は拡大
        scale += Vector3(0.01f, 0.01f, 0.01f);
      }
      if (input->IsKeyDown(DIK_DOWN)) { // 下キーが押されている間は縮小
        scale -= Vector3(0.01f, 0.01f, 0.01f);
      }
      if (input->IsKeyDown(DIK_C)) { // Cキーが押されている間は左に回転
        rotate.y -= 0.02f;
      }
      if (input->IsKeyDown(DIK_Z)) { // Zキーが押されている間は右に回転
        rotate.y += 0.02f;
      }
      if (input->IsKeyReleased(DIK_X)) { // Xキーが離されたら回転をリセット
        rotate.y = 0.0f;
      }

      object3d_1->SetScale(scale);
      object3d_1->SetRotation(rotate);
      object3d_1->SetTranslate(translate);
    }

    // カメラの更新

    // デバッグカメラ

    // デバッグカメラを使うならオブジェクトに渡す
    if (useDebugCamera) {
      if (!isSavedCamera) {
        gameCameraRotate = object3d_1->GetCameraRotation();
        gameCameraTranslate = object3d_1->GetCameraTranslate();

        isSavedCamera = true;
      }

      debugCamera.Update(*input); // ※操作が微妙なので調整しておく

      object3d_1->SetCameraRotation(debugCamera.GetRotation());
      object3d_1->SetCameraTranslate(debugCamera.GetTranslate());
      object3d_2->SetCameraRotation(debugCamera.GetRotation());
      object3d_2->SetCameraTranslate(debugCamera.GetTranslate());
    } else if (isSavedCamera) {
      object3d_1->SetCameraRotation(gameCameraRotate);
      object3d_1->SetCameraTranslate(gameCameraTranslate);
      object3d_2->SetCameraRotation(gameCameraRotate);
      object3d_2->SetCameraTranslate(gameCameraTranslate);

      isSavedCamera = false;
    }

    // currentViewMatrix = debugCamera.GetViewMatrix();

    //// particle用データの更新

    ////
    /// 現在有効なインスタンスデータを、GPU転送用バッファ(particleData)のどこに詰めるかを示すインデックス
    // uint32_t currentLiveIndex = 0;

    // if (input->IsKeyTriggered(DIK_SPACE)) {
    //	generateParticle = true;
    // }

    // if (generateParticle) {
    //	//　パーティクルを発生させる
    //	particles.splice(particles.end(), Emit(emitter, particleRandomEngine));

    //	generateParticle = false;
    //}

    // emitter.frequencyTime += kDeltaTime; // 時刻を進める
    // if (emitter.frequency <= emitter.frequencyTime) { //
    // 頻度より大きいなら発生
    //	//　パーティクルを発生させる
    //	particles.splice(particles.end(), Emit(emitter, particleRandomEngine));
    //	emitter.frequencyTime -= emitter.frequency; //
    // 余計に過ぎた時間も加味して頻度計算する
    // }

    //      //
    //      既存のパーティクルを更新し、寿命が尽きたものは削除し、GPUへデータを転送する
    // for (std::list<Particle>::iterator particleIterator = particles.begin();
    //	particleIterator != particles.end(); /* ++particleIterator
    // はループ内で制御 */) {

    //	// Fieldの範囲内のParticleには加速度を適用する
    //	if (isUpdate) {
    //		if (IsCollision(accelerationField.area,
    //(*particleIterator).transform.translate)) {
    //			(*particleIterator).velocity +=
    // accelerationField.acceleration * kDeltaTime;
    //		}
    //	}
    //	// パーティクル実体の更新処理
    //	(*particleIterator).transform.translate += (*particleIterator).velocity
    //* kDeltaTime;
    //	(*particleIterator).currentTime += kDeltaTime; // 経過時間を加算

    //	// アルファ値を計算 (透明化の処理)
    //	float alpha = 1.0f - ((*particleIterator).currentTime /
    //(*particleIterator).lifeTime);

    //	// 寿命が尽きた場合の処理
    //	if ((*particleIterator).currentTime >= (*particleIterator).lifeTime) {
    //		// 寿命が尽きた場合、リストから削除する
    //		// list::erase
    // の戻り値は、削除された要素の次の要素を指すイテレータ
    // particleIterator = particles.erase(particleIterator);
    //		// continue
    // で次のループへ進み、++currentLiveIndexとGPU転送はスキップする
    // continue;
    //	}

    //	//
    // GPU転送処理は、有効なパーティクル（削除されなかったもの）のみを対象とし、
    //	// currentLiveIndex（GPUバッファのインデックス）を使用

    //	// GPU最大数チェック,GPUバッファのオーバーフローを防ぐ
    //	if (currentLiveIndex < kNumMaxParticle) {
    //		// 回転の適用
    //		if (useBillboard) {
    //			(*particleIterator).transform.rotate = {};
    //			(*particleIterator).transform.rotate.z =
    // particleRotation.z; 		} else {
    //			(*particleIterator).transform.rotate = particleRotation;
    //		}

    //		// パーティクルのワールド行列とWVP行列を計算して転送

    //		// 行列の計算
    //		billboardMatrix = debugCamera.GetWorldMatrix();
    //		billboardMatrix.m[3][0] = 0.0f; // 平行移動成分はいらない
    //		billboardMatrix.m[3][1] = 0.0f;
    //		billboardMatrix.m[3][2] = 0.0f;

    //		Matrix4x4 particleWorldMatrix = MakeIdentity4x4();
    //		Matrix4x4 particleScaleMatrix =
    // MakeScaleMatrix((*particleIterator).transform.scale); Matrix4x4
    // particleRotateMatrix =
    // MakeRotateXYZMatrix((*particleIterator).transform.rotate);
    // Matrix4x4 particleTranslateMatrix =
    // MakeTranslationMatrix((*particleIterator).transform.translate);

    //		if (useBillboard) {
    //			particleWorldMatrix = particleScaleMatrix *
    // particleRotateMatrix * billboardMatrix * particleTranslateMatrix;
    // } else { 			particleWorldMatrix =
    // MakeAffineMatrix((*particleIterator).transform.scale,
    //(*particleIterator).transform.rotate,
    //(*particleIterator).transform.translate);
    //		}
    //		Matrix4x4 particleWVPMatrix = Multiply(particleWorldMatrix,
    // Multiply(currentViewMatrix, object3dProjectionMatrix));

    //		// 転送
    //	    // currentLiveIndex
    // をインデックスとして使用し、GPUバッファにデータを詰める
    //		particleData[currentLiveIndex].WVP = particleWVPMatrix;
    //		particleData[currentLiveIndex].World = particleWorldMatrix;
    //		particleData[currentLiveIndex].color =
    //(*particleIterator).color; // イテレータからcolorを取得

    //		// 計算したアルファ値を適用
    //		particleData[currentLiveIndex].color.w = alpha; //
    // 徐々に透明にする

    //		currentLiveIndex++; // 有効なパーティクル数をカウント
    //	}

    //	// 4. 次の要素へ進める（削除されなかった場合のみ）
    //	++particleIterator;
    //}
    //// 描画数を更新
    // uint32_t numParticleToDraw = currentLiveIndex;

    // オブジェクト
    object3d_1->Update();
    object3d_2->Update();

    // スプライト
    for (size_t i = 0; i < sprites.size(); ++i) {
      sprites[i]->Update();
    }

#pragma endregion 更新処理ここまで

    ////ここまで----------------------------------------------------------------------------------------

    //// 3. グラフィックコマンド

    ////
    /// 描画の処理-------------------------------------------------------------------------------------

#pragma region 描画処理

    // 描画前処理
    dxBase->PreDraw();

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
    // commandList->DrawIndexedInstanced(totalIndexCount, 1, 0, 0, 0); // 引数を
    // totalIndexCount に変更

    /// Particle描画---------------------------------------

    //// RootSignatureを設定
    // dxBase->GetCommandList()->SetGraphicsRootSignature(
    //     particleRootSignature.Get());
    //// 現在のブレンドモードに応じたPSOを設定
    // if (particleBlendMode >= 0 && particleBlendMode < kCountOfBlendMode) {
    //   // PSOを設定
    //   dxBase->GetCommandList()->SetPipelineState(
    //       particlePsoArray[particleBlendMode].Get());
    // } else {
    //   // 不正な値の場合
    //   dxBase->GetCommandList()->SetPipelineState(
    //       particlePsoArray[kBlendModeNormal].Get());
    // }
    //// VBVを設定
    // dxBase->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
    //// トポロジを設定
    // dxBase->GetCommandList()->IASetPrimitiveTopology(
    //     D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ///// ルートパラメータを設定
    //// マテリアルCBVの場所を設定
    // dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(
    //     0, materialResource->GetGPUVirtualAddress());
    ////
    /// インスタンス用SRVのDescriptorTableの先頭を設定。1はrootParametersForInstancing[1]である。
    // dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(
    //     1, instanceSrvHandleGPU);
    //// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
    // dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(
    //     2, changeTexture ? TextureManager::GetInstance()->GetSrvHandleGPU(
    //                            particleTextureIndex)
    //                      : TextureManager::GetInstance()->GetSrvHandleGPU(
    //                            uvCheckerIndex));

    //// 描画！（DrawCall/ドローコール）
    // if (numParticleToDraw > 0) {
    //   dxBase->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()),
    //                                           numParticleToDraw, 0, 0);
    // }

    /// スプライト-------------------------------
    // スプライト用の設定
    spriteCommon->SetCommonDrawSettings(
        static_cast<BlendState>(currentBlendMode), pipelineManager);
    // 描画！（DrawCall/ドローコール）
    if (showSprite) {
      for (size_t i = 0; i < sprites.size(); ++i) {

        if (i == 6) {
          // monsterBallテクスチャを適用
          sprites[i]->SetTextureIndex(monsterBallIndex);
        }

        // 描画
        sprites[i]->Draw();
      }
    }

    ////ここまで-ImGui_ImplDX12_Init()--------------------------------------------------------------------------------------

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
  // 初期化と逆順に行う
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

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

  // spriteCommonの解放
  delete spriteCommon;
  spriteCommon = nullptr;

  // PipelineManagerの解放
  if (pipelineManager) {
    delete pipelineManager;
    pipelineManager = nullptr;
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