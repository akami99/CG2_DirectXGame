#include "PipelineManager.h"

#include "Logger/Logger.h"

using namespace Microsoft::WRL;
using namespace Logger;

void PipelineManager::Initialize(DX12Context *dxBase) {
  dxBase_ = dxBase;

  // シェーダーのロード
  LoadShader();

  // ルートシグネチャの生成
  CreateRootSignature();
}

// グラフィックスパイプラインの生成
ComPtr<ID3D12PipelineState>
PipelineManager::CreateSpritePSO(const D3D12_BLEND_DESC &blendDesc) {

#pragma region PSO共通設定
  // PSO共通設定

  // ---固定で良いのでこの段階で設定---
  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
  };

  // ラスタライザステート作成
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 三角形の中を塗りつぶす
  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;  // カリングなし

  // Depth/Stencilステート
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
  depthStencilDesc.DepthEnable = FALSE; // 2DなのでDepthテスト無効
  // ------------------------------

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = rootSignatureSprite_.Get(); // Sprite用Root Signature
  // Sprite用シェーダー
  psoDesc.VS = {vsBlobSprite_->GetBufferPointer(),
                vsBlobSprite_->GetBufferSize()};
  psoDesc.PS = {psBlobSprite_->GetBufferPointer(),
                psBlobSprite_->GetBufferSize()};
  // ステート
  psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
  psoDesc.BlendState = blendDesc;
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.DepthStencilState = depthStencilDesc;
  // その他の共通設定
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

#pragma endregion ここまで

#pragma region PSO生成

  ComPtr<ID3D12PipelineState> pso;
  HRESULT hr = dxBase_->GetDevice()->CreateGraphicsPipelineState(
      &psoDesc, IID_PPV_ARGS(&pso));
  assert(SUCCEEDED(hr));

#pragma endregion ここまで

  return pso;
}

ComPtr<ID3D12PipelineState>
PipelineManager::CreateParticlePSO(const D3D12_BLEND_DESC &blendDesc) {

    // ---------------------------------------------------------------------
    // I. サブステートの定義 (BlendState, RasterizerState, DepthStencilState, InputLayout)
    // ---------------------------------------------------------------------

    // 1. InputLayout (頂点レイアウト)
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].InputSlot = 0;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[0].InstanceDataStepRate = 0;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].InputSlot = 0;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[1].InstanceDataStepRate = 0;

    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].InputSlot = 0;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[2].InstanceDataStepRate = 0;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);


    // 2. RasterizerState (ラスタライザ設定)
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 裏面カリング無効
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 三角形の中を塗りつぶす


    // 3. DepthStencilState (深度/ステンシル設定)
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true; // 深度テスト有効 (3D描画のため)
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度書き込み無効 (ブレンドのため)
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 比較関数


    // ---------------------------------------------------------------------
    // II. グラフィックスパイプラインステート (PSO) の構築
    // ---------------------------------------------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

    // シェーダとシグネチャ (パイプラインの核となる部分)
    psoDesc.pRootSignature = rootSignatureParticle_.Get();
    psoDesc.VS = { vsBlobParticle_->GetBufferPointer(), vsBlobParticle_->GetBufferSize() };
    psoDesc.PS = { psBlobParticle_->GetBufferPointer(), psBlobParticle_->GetBufferSize() };

    // ステート設定 (I. で定義したサブステートを適用)
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.DepthStencilState = depthStencilDesc;

    // レンダリング設定 (RT/DSVフォーマット、トポロジ)
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // サンプリング設定 (固定値)
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;


    // ---------------------------------------------------------------------
    // III. PSOの生成
    // ---------------------------------------------------------------------

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = dxBase_->GetDevice()->CreateGraphicsPipelineState(
        &psoDesc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));

    return pso;
}

ComPtr<ID3D12PipelineState> PipelineManager::CreateObject3dPSO(
    const D3D12_INPUT_ELEMENT_DESC *inputLayout, uint32_t numElements,
    const D3D12_RASTERIZER_DESC &rasterizerDesc,
    const D3D12_DEPTH_STENCIL_DESC &depthStencilDesc,
    const D3D12_BLEND_DESC &blendDesc) {
  HRESULT hr;

  // PSO設定の雛形を作成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  // ルートシグネチャを設定
  psoDesc.pRootSignature = rootSignature3D_.Get();
  // シェーダーを設定
  psoDesc.VS = {vsBlob3D_->GetBufferPointer(), vsBlob3D_->GetBufferSize()};
  psoDesc.PS = {psBlob3D_->GetBufferPointer(), psBlob3D_->GetBufferSize()};
  // 入力レイアウトを設定
  psoDesc.InputLayout = {inputLayout, numElements};
  // その他のステートを設定
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.DepthStencilState = depthStencilDesc;
  psoDesc.BlendState = blendDesc;
  // 描画ターゲットとトポロジの設定
  psoDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 3dは三角形リスト
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] =
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // メインレンダーターゲット
  psoDesc.DSVFormat =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度バッファのフォーマット
  // サンプリング設定
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  // PSOの生成
  ComPtr<ID3D12PipelineState> pso;
  hr = dxBase_->GetDevice()->CreateGraphicsPipelineState(&psoDesc,
                                                         IID_PPV_ARGS(&pso));

  return pso;
}

// シェーダーの読み込み
void PipelineManager::LoadShader() {
  // VertexShaderの読み込み
  vsBlobSprite_ = dxBase_->CompileShader(L"Sprite.VS.hlsl", L"vs_6_0");
  assert(vsBlobSprite_ != nullptr);
  // PixelShaderの読み込み
  psBlobSprite_ = dxBase_->CompileShader(L"Sprite.PS.hlsl", L"ps_6_0");
  assert(psBlobSprite_ != nullptr);

  // オブジェクト用
  vsBlob3D_ = dxBase_->CompileShader(L"Object3d.VS.hlsl", L"vs_6_0");
  assert(vsBlob3D_ != nullptr);
  psBlob3D_ = dxBase_->CompileShader(L"Object3d.PS.hlsl", L"ps_6_0");
  assert(psBlob3D_ != nullptr);

  // パーティクル用
  vsBlobParticle_ = dxBase_->CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
  assert(vsBlobParticle_ != nullptr);
  psBlobParticle_ = dxBase_->CompileShader(L"Particle.PS.hlsl", L"ps_6_0");
  assert(psBlobParticle_ != nullptr);
}

// ルートシグネチャの作成
void PipelineManager::CreateRootSignature() {
  HRESULT hr;

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

#pragma region Sprite用のRootSignature作成
  // Sprite用のRootSignature作成

  // Texture用 SRV (Pixel Shader用)
  D3D12_DESCRIPTOR_RANGE spriteTextureSrvRange[1] = {};
  spriteTextureSrvRange[0].BaseShaderRegister = 0; // t0
  spriteTextureSrvRange[0].NumDescriptors = 1;
  spriteTextureSrvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  spriteTextureSrvRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // RootParameter作成
  D3D12_ROOT_PARAMETER spriteRootParameters[3] = {};

  // Root Parameter 0: Pixel Shader用 Material CBV (b0)
  spriteRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  spriteRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  spriteRootParameters[0].Descriptor.ShaderRegister = 0; // b0

  // Root Parameter 1: Vertex Shader用 Transform CBV (b0)
  spriteRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  spriteRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  spriteRootParameters[1].Descriptor.ShaderRegister = 1; // b1

  // Root Parameter 2: Pixel Shader用 Texture SRV (t0)
  spriteRootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  spriteRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  spriteRootParameters[2].DescriptorTable.pDescriptorRanges =
      spriteTextureSrvRange;
  spriteRootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(spriteTextureSrvRange);

  // RootSignatureDesc
  D3D12_ROOT_SIGNATURE_DESC spriteRootSignatureDesc{};
  spriteRootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  spriteRootSignatureDesc.pParameters = spriteRootParameters;
  spriteRootSignatureDesc.NumParameters = _countof(spriteRootParameters);
  spriteRootSignatureDesc.pStaticSamplers = staticSamplers; // 共通Samplerを使用
  spriteRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズと生成
  ComPtr<ID3DBlob> spriteSignatureBlob = nullptr;
  ComPtr<ID3DBlob> spriteErrorBlob = nullptr;

  hr = D3D12SerializeRootSignature(&spriteRootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1,
                                   &spriteSignatureBlob, &spriteErrorBlob);
  // エラーチェックと生成処理...
  if (FAILED(hr)) {
    Logger::Log(reinterpret_cast<char *>(spriteErrorBlob->GetBufferPointer()));
    assert(false);
  }
  hr = dxBase_->GetDevice()->CreateRootSignature(
      0, spriteSignatureBlob->GetBufferPointer(),
      spriteSignatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&rootSignatureSprite_));
  assert(SUCCEEDED(hr));

#pragma endregion Sprite用のRootSignature作成ここまで

#pragma region Object3d用のRootSignature作成
  // Object3d用のRootSignature作成

  // Texture用 SRV (Pixel Shader用)
  D3D12_DESCRIPTOR_RANGE object3dTextureSrvRange[1] = {};
  object3dTextureSrvRange[0].BaseShaderRegister = 0; // 0から始まる
  object3dTextureSrvRange[0].NumDescriptors = 1;     // 数は1つ
  object3dTextureSrvRange[0].RangeType =
      D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  object3dTextureSrvRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  // RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
  D3D12_ROOT_PARAMETER object3dRootParameters[4] = {};

  // Root Parameter 0: Pixel Shader用 Material CBV (b0)
  object3dRootParameters[0].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う(b0のbと一致する)
  object3dRootParameters[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  object3dRootParameters[0].Descriptor.ShaderRegister =
      0; // レジスタ番号0とバインド(b0の0と一致する。もしb11と紐づけたいなら11となる)

  // Root Parameter 1: Vertex Shader用 Transform CBV (b0)
  object3dRootParameters[1].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う(b0のbと一致する)
  object3dRootParameters[1].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_VERTEX;                      // VertexShaderで使う
  object3dRootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0

  // Root Parameter 2: Pixel Shader用 Texture SRV (t0)
  object3dRootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
  object3dRootParameters[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  object3dRootParameters[2].DescriptorTable.pDescriptorRanges =
      object3dTextureSrvRange; // Tableの中身の配列を指定
  object3dRootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(object3dTextureSrvRange); // Tableで利用する数

  // Root Parameter 3: Pixel Shader用 DirectionalLight CBV (b1)
  object3dRootParameters[3].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  object3dRootParameters[3].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  object3dRootParameters[3].Descriptor.ShaderRegister =
      1; // レジスタ番号1を使う

  // object用のRootSignatureDesc
  D3D12_ROOT_SIGNATURE_DESC object3dRootSignatureDesc{};
  object3dRootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  object3dRootSignatureDesc.pParameters =
      object3dRootParameters; // ルートパラメータ配列へのポインタ
  object3dRootSignatureDesc.NumParameters =
      _countof(object3dRootParameters); // 配列の長さ
  object3dRootSignatureDesc.pStaticSamplers = staticSamplers;
  object3dRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズしてバイナリにする
  ComPtr<ID3DBlob> object3dSignatureBlob = nullptr;
  ComPtr<ID3DBlob> object3dErrorBlob = nullptr;

  hr = D3D12SerializeRootSignature(&object3dRootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1,
                                   &object3dSignatureBlob, &object3dErrorBlob);
  if (FAILED(hr)) {
    Logger::Log(
        reinterpret_cast<char *>(object3dErrorBlob->GetBufferPointer()));
    assert(false);
  }
  // バイナリを元に生成
  hr = dxBase_->GetDevice()->CreateRootSignature(
      0, object3dSignatureBlob->GetBufferPointer(),
      object3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature3D_));
  assert(SUCCEEDED(hr));

#pragma endregion Object3d用のRootSignature作成ここまで

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
  ComPtr<ID3DBlob> particleSignatureBlob = nullptr;
  ComPtr<ID3DBlob> particleErrorBlob = nullptr;

  hr = D3D12SerializeRootSignature(&particleRootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1,
                                   &particleSignatureBlob, &particleErrorBlob);
  if (FAILED(hr)) {
    Logger::Log(
        reinterpret_cast<char *>(particleErrorBlob->GetBufferPointer()));
    assert(false);
  }
  // バイナリを元に生成
  hr = dxBase_->GetDevice()->CreateRootSignature(
      0, particleSignatureBlob->GetBufferPointer(),
      particleSignatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&rootSignatureParticle_));
  assert(SUCCEEDED(hr));

#pragma endregion Particle用のRootSignature作成ここまで
}
