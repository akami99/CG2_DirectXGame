//#include <Windows.h>
//#include <cstdint>
//#include <string>
//#include <format>
//#include <filesystem>
//#include <fstream>
//#include <chrono>

#include <dxgi1_6.h>
#include <cassert>
#include <dbghelp.h>
#include <strsafe.h>

//#include <dxcapi.h>
#include <vector>
#include <numbers>
#include <DirectXMath.h>
//#include <iostream>
#include <sstream>
//#include <wrl.h>
#include <random>

#include "Win32Window.h"
#include "DX12Context.h"
#include "D3DResourceLeakChecker.h"
#include "Logger.h"
#include "Input.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "AudioManager.h"
#include "DebugCamera.h"

#include "MathTypes.h"
#include "MathUtils.h"
#include "MatrixGenerators.h"

#include "ApplicationConfig.h"

//#include "externals/DirectXTex/DirectXTex.h"
//#include "externals/DirectXTex/d3dx12.h"

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

#pragma region 構造体定義
// 構造体定義

// 頂点データの構造体(Spriteにも有り)
struct VertexData {
	Vector4 position;                 // ワールド座標系での位置
	Vector2 texcoord;                 // UV座標
	Vector3 normal;                   // 法線ベクトル
};// 16+8+12=36バイト。float*4+float*2+float*3

// 変換行列を構成するための構造体(s,r,t)
struct Transform {
	Vector3 scale;                    // スケーリング
	Vector3 rotate;                   // 回転
	Vector3 translate;                // 平行移動
};// Vector3*3=36バイト

// マテリアルの構造体(Spriteにも有り)
struct Material {
	Vector4 color;                    // マテリアルの色
	int32_t enableLighting;           // ライティングの有効・無効フラグ
	float padding[3];                 // 16バイトのアライメントを確保するためのパディング
	Matrix4x4 uvTransform;            // UV変換行列
};// Vector4(16)+int32_t(4)=20バイト + float*3(12)=32バイト

// DirectionalLightの構造体
struct DirectionalLight {
	Vector4 color;                    // ライトの色
	Vector3 direction;                // ライトの向き(正規化する)
	float _padding;                   // 16バイトのアライメントを確保するためのパディング
	float intensity;                  // 輝度
};// Vector4(16)+Vector3(12)+float(4)=32バイト + float(4)=36バイト

// パーティクルの構造体
struct Particle {
	Transform transform;              // 変換行列
	Vector3 velocity;                 // 速度
	Vector4 color;                    // 色
	float lifeTime;                   // 生存時間
	float currentTime;                // 経過時間
}; // Transform(36)+Vector3(12)+Vector4(16)=64バイト

// 変換行列をまとめた構造体
struct TransformationMatrix {
	Matrix4x4 WVP;                    // ワールドビュー射影行列
	Matrix4x4 World;                  // ワールド行列
};// Matrix4x4*2=128バイト

// マテリアルデータの構造体
struct MaterialData {
	std::string textureFilePath;      // テクスチャファイルパス
};

// モデルデータの構造体
struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ配列
	MaterialData material;  		  // マテリアルデータ
};

// パーティクルインスタンスデータの構造体
struct ParticleInstanceData {
	Matrix4x4 WVP;                    // ワールドビュー射影行列
	Matrix4x4 World;                  // ワールド行列
	Vector4 color;                    // 色
};// Matrix4x4(64)+Vector4(16)=80バイト

#pragma endregion ここまで

#pragma region 関数定義
// 関数定義

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// ログと同じく、exeと同じ階層に「dumps」フォルダを作成する
	const wchar_t* dumpDir = L"dumps";

	CreateDirectoryW(dumpDir, nullptr);

	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };

	StringCchPrintfW(filePath, MAX_PATH, L"%s/%04d-%02d%02d-%02d%02d%02d.dmp", dumpDir, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

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
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力、MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	// ファイルハンドルを閉じる
	CloseHandle(dumpFileHandle);

	// 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& fileName) {
	// 1. 中で必要となる変数の宣言
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	// 3. 実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		// identifierに応じた処理
		if (identifier == "map_Kd") { // テクスチャファイルパス
			std::string textureFilename;
			s >> textureFilename;
			// 連結してテクスチャファイルパスにする
			materialData.textureFilePath = textureFilename;
		}
	}

	// 4. MaterialDataを返す
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
	// 1. 中で必要となる変数の宣言
	ModelData modelData; // 構築するModelData
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ1行を格納するもの
	// 2. ファイルを開く
	std::string fullPath = directoryPath;
	// 既にパスに"DirectXGame/Resources/Assets/Sounds/"が含まれている場合は追加しない
	if (directoryPath.find("Resources/Assets/Models/") == std::string::npos &&
		directoryPath.find("resources/assets/models/") == std::string::npos) {
		fullPath = "Resources/Assets/Models/" + directoryPath;
	}

	std::ifstream file(fullPath + "/" + fileName); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	// 3. 実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		// identifierに応じた処理
		if (identifier == "v") { // 頂点位置
			Vector4 position;
			s >> position.x >> position.y >> position.z; // x, y, zを読む
			position.w = 1.0f; // wは1.0fに設定
			positions.push_back(position); // positionsに追加
		} else if (identifier == "vt") { // テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y; // x, yを読む
			texcoord.y = 1.0f - texcoord.y; // Y軸を反転させる
			texcoords.push_back(texcoord); // texcoordsに追加
		} else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z; // x, y, zを読む
			normals.push_back(normal); // normalsに追加
		} else if (identifier == "f") { // 面情報
			VertexData triangle[3];
			// 面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を読んでいく
				Vector4 position = positions[elementIndices[0] - 1]; // OBJは1-indexedなので-1する
				//position.x *= -1.0f; // X軸を反転する
				Vector2 texcoord = texcoords[elementIndices[1] - 1]; // OBJは1-indexedなので-1する
				Vector3 normal = normals[elementIndices[2] - 1]; // OBJは1-indexedなので-1する
				//normal.x *= -1.0f; // X軸を反転する
				VertexData vertex = { position, texcoord, normal }; // 頂点データを構築
				modelData.vertices.push_back(vertex); // ModelDataに追加
				triangle[faceVertex] = { position, texcoord, normal };
			}
			// 頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFileName;
			s >> materialFileName;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(fullPath, materialFileName);
		}
	}
	// 4. ModelDataを返す
	return modelData;
}

// パーティクル生成関数
Particle MakeNewParticle(std::mt19937& randomEngine) {
	// ランダムな速度ベクトルを生成
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f); // distribution()という関数のように使うイメージ
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	// パーティクルの初期化
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;
	// ランダムに設定
	particle.transform.translate = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };

	particle.velocity = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine) };

	particle.color = { distribution(randomEngine), distribution(randomEngine), distribution(randomEngine), 1.0f };

	return particle;
}

#pragma endregion ここまで

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
	Win32Window* window = nullptr;

	// WindowWrapperの生成と初期化
	window = new Win32Window();
	window->Initialize();

#pragma endregion ここまで

#pragma region DirectXの初期化
	// DirectXの初期化

	// ポインタ
	DX12Context* dxBase = nullptr;
	// DirectXBaseの生成と初期化
	dxBase = new DX12Context();
	dxBase->Initialize(window);

#pragma endregion ここまで

#pragma region スプライト共通部の初期化
	// スプライト共通部の初期化

	// ポインタ
	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxBase);

#pragma endregion ここまで

#pragma region RootSignature作成
	// RootSignature作成

#pragma region 共通のSampler設定
	// 共通のSamplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;   // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;   // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;   // ありったけのMipmapを使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	staticSamplers[0].ShaderRegister = 0;   // レジスタ番号0を使う

#pragma endregion ここまで

#pragma region Object3d用のRootSignature作成
	// Object3d用のRootSignature作成

	// Texture用 SRV (Pixel Shader用)
	D3D12_DESCRIPTOR_RANGE object3dTextureSrvRange[1] = {};
	object3dTextureSrvRange[0].BaseShaderRegister = 0;  // 0から始まる
	object3dTextureSrvRange[0].NumDescriptors = 1;  // 数は1つ
	object3dTextureSrvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	object3dTextureSrvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform 
	D3D12_ROOT_PARAMETER object3dRootParameters[4] = {};

	// Root Parameter 0: Pixel Shader用 Material CBV (b0)
	object3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う(b0のbと一致する)
	object3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;   // PixelShaderで使う
	object3dRootParameters[0].Descriptor.ShaderRegister = 0;    // レジスタ番号0とバインド(b0の0と一致する。もしb11と紐づけたいなら11となる)

	// Root Parameter 1: Vertex Shader用 Transform CBV (b0)
	object3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う(b0のbと一致する)
	object3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;   // VertexShaderで使う
	object3dRootParameters[1].Descriptor.ShaderRegister = 0;    // レジスタ番号0

	// Root Parameter 2: Pixel Shader用 Texture SRV (t0)
	object3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	object3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	object3dRootParameters[2].DescriptorTable.pDescriptorRanges = object3dTextureSrvRange;  // Tableの中身の配列を指定
	object3dRootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(object3dTextureSrvRange); // Tableで利用する数

	// Root Parameter 3: Pixel Shader用 DirectionalLight CBV (b1)
	object3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	object3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	object3dRootParameters[3].Descriptor.ShaderRegister = 1;  // レジスタ番号1を使う

	// object用のRootSignatureDesc
	D3D12_ROOT_SIGNATURE_DESC object3dRootSignatureDesc{};
	object3dRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	object3dRootSignatureDesc.pParameters = object3dRootParameters; // ルートパラメータ配列へのポインタ
	object3dRootSignatureDesc.NumParameters = _countof(object3dRootParameters); // 配列の長さ
	object3dRootSignatureDesc.pStaticSamplers = staticSamplers;
	object3dRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ComPtr<ID3D12RootSignature> object3dRootSignature = nullptr;
	ComPtr<ID3DBlob> object3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> object3dErrorBlob = nullptr;

	HRESULT hr;

	hr = D3D12SerializeRootSignature(&object3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &object3dSignatureBlob, &object3dErrorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(object3dErrorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = dxBase->GetDevice()->CreateRootSignature(0, object3dSignatureBlob->GetBufferPointer(), object3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&object3dRootSignature));
	assert(SUCCEEDED(hr));

#pragma endregion ここまで

#pragma region Particle用のRootSignature作成
	// Particle用のRootSignature作成

	// Particle用 SRV (Vertex Shader用)
	D3D12_DESCRIPTOR_RANGE particleInstancingSrvRange[1] = {};
	particleInstancingSrvRange[0].BaseShaderRegister = 0;// t0
	particleInstancingSrvRange[0].NumDescriptors = 1;// 数は1つ
	particleInstancingSrvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	particleInstancingSrvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// テクスチャ用 SRV (Pixel Shader用)
	D3D12_DESCRIPTOR_RANGE particleTextureSrvRange[1] = {};
	particleTextureSrvRange[0].BaseShaderRegister = 0;// t0 (Texture)
	particleTextureSrvRange[0].NumDescriptors = 1;// 数は1つ
	particleTextureSrvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	particleTextureSrvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform 
	D3D12_ROOT_PARAMETER particleRootParameters[3] = {};

	// Root Parameter 0: Pixel Shader用 Material CBV (b0)
	particleRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	particleRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	particleRootParameters[0].Descriptor.ShaderRegister = 0; // b0

	// Root Parameter 1: Vertex Shader用 Instancing SRV (t0)
	particleRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	particleRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	particleRootParameters[1].DescriptorTable.pDescriptorRanges = particleInstancingSrvRange;
	particleRootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(particleInstancingSrvRange);

	// Root Parameter 2: Pixel Shader用 Texture SRV (t0)
	particleRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	particleRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	particleRootParameters[2].DescriptorTable.pDescriptorRanges = particleTextureSrvRange;
	particleRootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(particleTextureSrvRange);

	// Particle用のRootSignatureDesc
	D3D12_ROOT_SIGNATURE_DESC particleRootSignatureDesc{};
	particleRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	particleRootSignatureDesc.pParameters = particleRootParameters; // ルートパラメータ配列へのポインタ
	particleRootSignatureDesc.NumParameters = _countof(particleRootParameters); // 配列の長さ (3)
	particleRootSignatureDesc.pStaticSamplers = staticSamplers;
	particleRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ComPtr<ID3D12RootSignature> particleRootSignature = nullptr;
	ComPtr<ID3DBlob> particleSignatureBlob = nullptr;
	ComPtr<ID3DBlob> particleErrorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&particleRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &particleSignatureBlob, &particleErrorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(particleErrorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = dxBase->GetDevice()->CreateRootSignature(0, particleSignatureBlob->GetBufferPointer(), particleSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&particleRootSignature));
	assert(SUCCEEDED(hr));

#pragma endregion ここまで

#pragma endregion ここまで

#pragma region パイプラインステート作成用設定

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].InputSlot = 0; // 明示的に指定
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[0].InstanceDataStepRate = 0; // デフォルト値だが明示するとより分かりやすい
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].InputSlot = 0; // 明示的に指定
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[1].InstanceDataStepRate = 0;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].InputSlot = 0; // 明示的に指定
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // 明示的に指定
	inputElementDescs[2].InstanceDataStepRate = 0;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion ここまで

#pragma region RasterizerState作成
	// RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面をカリング
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

#pragma endregion ここまで

#pragma region ブレンドモード
	// ブレンドモード
	int currentBlendMode = kBlendModeNormal;
	int particleBlendMode = kBlendModeAdd;

	// BlendStateの設定を配列で用意しておく
	D3D12_BLEND_DESC blendDescs[kCountOfBlendMode] = {};
	// ブレンド無し
	blendDescs[kBlendModeNone].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeNone].RenderTarget[0].BlendEnable = FALSE;

	// 通常のアルファブレンド
	blendDescs[kBlendModeNormal].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeNormal].RenderTarget[0].BlendEnable = TRUE;
	blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	blendDescs[kBlendModeNormal].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeNormal].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // 1.0f - ソースのアルファ値
	blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	blendDescs[kBlendModeNormal].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeNormal].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

	// 加算ブレンド
	blendDescs[kBlendModeAdd].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeAdd].RenderTarget[0].BlendEnable = TRUE;
	blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	blendDescs[kBlendModeAdd].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeAdd].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
	blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	blendDescs[kBlendModeAdd].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeAdd].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

	// 減算ブレンド
	blendDescs[kBlendModeSubtract].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendEnable = TRUE;
	blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 減算
	blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
	blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

	// 乗算ブレンド
	blendDescs[kBlendModeMultiply].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendEnable = TRUE;
	blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO; // 0.0f
	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // ソースカラー値
	blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

	// スクリーンブレンド
	blendDescs[kBlendModeScreen].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs[kBlendModeScreen].RenderTarget[0].BlendEnable = TRUE;
	blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // デストカラー値
	blendDescs[kBlendModeScreen].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeScreen].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // ソースアルファ値
	blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	blendDescs[kBlendModeScreen].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	blendDescs[kBlendModeScreen].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

#pragma endregion ここまで

#pragma region Shaderコンパイル
	// Shaderをコンパイル

	// オブジェクト用
	ComPtr<IDxcBlob> object3dVSBlob = dxBase->CompileShader(L"Object3d.VS.hlsl", L"vs_6_0");
	ComPtr<IDxcBlob> object3dPSBlob = dxBase->CompileShader(L"Object3d.PS.hlsl", L"ps_6_0");
	assert(object3dVSBlob != nullptr && object3dPSBlob != nullptr);

	// パーティクル用
	ComPtr<IDxcBlob> particleVSBlob = dxBase->CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
	ComPtr<IDxcBlob> particlePSBlob = dxBase->CompileShader(L"Particle.PS.hlsl", L"ps_6_0");
	assert(particleVSBlob != nullptr && particlePSBlob != nullptr);

#pragma endregion ここまで

#pragma region DepthStencilState設定
	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

#pragma endregion ここまで

#pragma region PSO作成
	/// ベースのPSO設定----------------------------------

#pragma region Object3D用PSO共通設定
	// Object3D用PSO共通設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescObject3dBase{};

	// **共通設定**を一度だけセット
	psoDescObject3dBase.pRootSignature = object3dRootSignature.Get(); // RootSignature
	psoDescObject3dBase.InputLayout = inputLayoutDesc; // InputLayout
	psoDescObject3dBase.VS = { object3dVSBlob->GetBufferPointer(), object3dVSBlob->GetBufferSize() }; // VertexShader
	psoDescObject3dBase.PS = { object3dPSBlob->GetBufferPointer(), object3dPSBlob->GetBufferSize() }; // PixelShader
	psoDescObject3dBase.RasterizerState = rasterizerDesc; // RasterizerState
	// 書き込むRTVの情報
	psoDescObject3dBase.NumRenderTargets = 1;
	psoDescObject3dBase.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDescObject3dBase.SampleDesc.Count = 1;
	// DepthStencilの設定
	psoDescObject3dBase.DepthStencilState = depthStencilDesc;
	psoDescObject3dBase.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 利用するトポロジ（形状）のタイプ。三角形
	psoDescObject3dBase.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定（気にしなくていい）
	psoDescObject3dBase.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 外部で宣言（クラスのメンバー変数など）
	ComPtr<ID3D12PipelineState> object3dPsoArray[kCountOfBlendMode];

#pragma endregion ここまで

#pragma region Particle用PSO共通設定

	// パーティクル用PSO共通設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticleBase{};

	// **共通設定**を一度だけセット
	psoDescParticleBase.pRootSignature = particleRootSignature.Get(); // RootSignature
	psoDescParticleBase.InputLayout = inputLayoutDesc; // InputLayout
	psoDescParticleBase.VS = { particleVSBlob->GetBufferPointer(), particleVSBlob->GetBufferSize() }; // VertexShader
	psoDescParticleBase.PS = { particlePSBlob->GetBufferPointer(), particlePSBlob->GetBufferSize() }; // PixelShader
	psoDescParticleBase.RasterizerState = rasterizerDesc; // RasterizerState
	// 書き込むRTVの情報
	psoDescParticleBase.NumRenderTargets = 1;
	psoDescParticleBase.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDescParticleBase.SampleDesc.Count = 1;
	// DepthStencilの設定
	psoDescParticleBase.DepthStencilState = depthStencilDesc;
	psoDescParticleBase.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 利用するトポロジ（形状）のタイプ。三角形
	psoDescParticleBase.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定（気にしなくていい）
	psoDescParticleBase.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 外部で宣言（クラスのメンバー変数など）
	ComPtr<ID3D12PipelineState> particlePsoArray[kCountOfBlendMode];

#pragma endregion ここまで

#pragma region ブレンドモードごとのPSO生成
	// ループで各ブレンドモードのPSOを生成

	for (int i = 0; i < kCountOfBlendMode; ++i) {
		// ----------------------------------------------------
		// Particle用 PSO 生成 (ブレンド設定適用)
		// ----------------------------------------------------
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticle = psoDescParticleBase;

		// Particleはアルファブレンドを多用するため、Depth書き込みは無効化
		if (i != kBlendModeNone) { // ブレンドモードがNoneでない場合
			// アルファブレンドを行う場合はDepthへの書き込みを無効にする（順番依存を許容するため）
			psoDescParticle.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		}

		// ブレンド設定を適用
		psoDescParticle.BlendState = blendDescs[i];

		// PSO生成
		hr = dxBase->GetDevice()->CreateGraphicsPipelineState(&psoDescParticle, IID_PPV_ARGS(&particlePsoArray[i]));
		assert(SUCCEEDED(hr));

		// ----------------------------------------------------
		// Object3D用 PSO 生成 (ブレンド設定適用)
		// ----------------------------------------------------
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescObject = psoDescObject3dBase;

		// Object3DはDepth書き込みを有効
		// BlendModeがNone以外の場合はDepthWriteMaskを調整することもできるが、
		// ここではベース設定（D3D12_DEPTH_WRITE_MASK_ALL）を維持

		// ブレンド設定を適用
		psoDescObject.BlendState = blendDescs[i];

		// PSO生成
		hr = dxBase->GetDevice()->CreateGraphicsPipelineState(&psoDescObject, IID_PPV_ARGS(&object3dPsoArray[i]));
		assert(SUCCEEDED(hr));
	}

#pragma endregion ここまで

#pragma endregion ここまで

#pragma region DirectInputの初期化
	// DirectInputの初期化

	Input* input = new Input();
	input->Initialize(window);

#pragma endregion ここまで

#pragma region DebugCameraの初期化
	// DebugCameraの初期化

	DebugCamera debugCamera;
	debugCamera.Initialize();

#pragma endregion ここまで

#pragma region TextureManagerの初期化
	// TextureManagerの初期化

	TextureManager::GetInstance()->Initialize(dxBase);

#pragma endregion

#pragma region Particle用リソース作成
	// Particle用リソース作成

	// Particle最大数
	const uint32_t kNumMaxParticle = 20;
	// Particle用のTransformationMatrixリソースを作る
	ComPtr<ID3D12Resource> particleResource = dxBase->CreateBufferResource(sizeof(ParticleInstanceData) * kNumMaxParticle);
	// 書き込むためのアドレスを取得
	ParticleInstanceData* particleData = nullptr;
	particleResource->Map(0, nullptr, reinterpret_cast<void**>(&particleData));
	// 単位行列を書き込んでおく
	for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
		particleData[index].WVP = MakeIdentity4x4();
		particleData[index].World = MakeIdentity4x4();
		particleData[index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU = dxBase->CreateStructuredBufferSRV(particleResource, kNumMaxParticle, sizeof(ParticleInstanceData), 4);

	// ランダムエンジンの初期化(パーティクル用)
	static std::random_device particleSeedGenerator;
	static std::mt19937 particleRandomEngine(particleSeedGenerator());

	std::uniform_real_distribution<float> particleDist(-1.0f, 1.0f);

	Particle particles[kNumMaxParticle];

	bool generateParticle = false;

	for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
		particles[index] = MakeNewParticle(particleRandomEngine);

		// 色をGPUに転送
		particleData[index].color = particles[index].color;
	}

#pragma endregion ここまで

#pragma region いろいろリソース作成

	// モデル読み込み
	ModelData modelData = LoadObjFile("Plane", "plane.obj");

	// 実際に頂点リソースを作る
	ComPtr<ID3D12Resource> vertexResource = dxBase->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * totalUniqueVertexCount);
	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());// 頂点データをリソースにコピー

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//vertexBufferView.SizeInBytes = sizeof(VertexData) * totalUniqueVertexCount;
	// １頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 球
	// マテリアル用のリソースを作る。
	ComPtr<ID3D12Resource> materialResource = dxBase->CreateBufferResource(sizeof(Material));

	// マテリアルにデータを書き込む
	Material* materialData = nullptr;
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 色の書き込み
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();

	// 平行光源
	// データを書き込む
	ComPtr<ID3D12Resource> directionalLightResource = dxBase->CreateBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	// 書き込むためのアドレスを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	// デフォルト値はとりあえず以下のようにしておく
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	// WVP用のリソースを作る。Matrix4x4　1つ分のサイズを用意する
	ComPtr<ID3D12Resource> wvpResource = dxBase->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* object3dData = nullptr;
	// 書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&object3dData));
	// 単位行列を書き込んでおく
	object3dData->WVP = MakeIdentity4x4();
	object3dData->World = MakeIdentity4x4();


	// Transform変数を作る
	//三角形
	Transform object3dTransform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

#pragma endregion ここまで

#pragma region テクスチャの読み込みとアップロード
	// テクスチャの読み込みとアップロード

	// テクスチャインデックスを保持
	uint32_t uvCheckerIndex = TextureManager::GetInstance()->LoadTexture("uvChecker.png");// Index 1
	uint32_t monsterBallIndex = TextureManager::GetInstance()->LoadTexture("monsterBall.png");// Index 2
	uint32_t modelTextureIndex = TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);// Index 3
	uint32_t particleTextureIndex = TextureManager::GetInstance()->LoadTexture("circle.png");// Index 4

	// コマンド実行と完了待機
	dxBase->ExecuteInitialCommandAndSync();
	TextureManager::GetInstance()->ReleaseIntermediateResources();

#pragma endregion ここまで(コマンド実行済み)

	// カメラの設定
	/*Transform cameraTransform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -10.0f } };
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);*/
	Matrix4x4 currentViewMatrix = debugCamera.GetViewMatrix();

	// 更新前に一度だけ計算しておく
	Matrix4x4 object3dProjectionMatrix = MakePerspectiveFovMatrix(0.45f, float(Win32Window::kClientWidth) / float(Win32Window::kClientHeight), 0.1f, 100.0f);

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

#pragma endregion ここまで

#pragma endregion ここまで

#pragma region 最初のシーンの初期化
	// 最初のシーンの初期化
	const float spriteSize = 200.0f;
	std::vector<Sprite*> sprites;
	for (uint32_t i = 0; i < 10; ++i) {
		Sprite* newSprite = new Sprite();
		newSprite->Initialize(spriteCommon, uvCheckerIndex);
		newSprite->SetAnchorPoint({ 0.5f, 0.5f });
		if (i == 7) {
			newSprite->SetFlipX(1);
			newSprite->SetFlipY(1);
		} else if (i == 8) {
			newSprite->SetFlipX(1);
			newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
			newSprite->SetTextureSize({ 64.0f, 64.0f });
			newSprite->SetScale({ spriteSize, spriteSize });
		} else if (i == 9) {
			newSprite->SetTextureLeftTop({ 0.0f, 0.0f });
			newSprite->SetTextureSize({ 64.0f, 64.0f });
			newSprite->SetScale({ spriteSize, spriteSize });
		}
		newSprite->SetTranslate({ float(i * spriteSize / 3), float(i * spriteSize / 3) });
		sprites.push_back(newSprite);
	}

	//スプライトの表示
	bool showSprite = true;

	//マテリアル
	bool showMaterial = true;

	bool controlMaterial = true;


	// 画像の変更(particle,)
	bool changeTexture = true;

#pragma endregion ここまで

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
		//ImGui::ShowDemoWindow();
		// 設定ウィンドウ(画面右側)
		ImGui::SetNextWindowPos(ImVec2(Win32Window::kClientWidth - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
		ImGui::Begin("Settings");
		// デバッグウィンドウ
		//ImGui::Text("Camera");

		//ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f);
		//ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f);

		// グローバル設定
		ImGui::Separator();
		if (ImGui::TreeNode("Global Settings")) {
			ImGui::Combo("BlendMode", &currentBlendMode, "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
			ImGui::TreePop();
		}

		ImGui::Separator();
		if (ImGui::TreeNode("Material")) {
			ImGui::Checkbox("showMaterial", &showMaterial);
			ImGui::Checkbox("controlMaterial", &controlMaterial);
			ImGui::DragFloat3("scale", &object3dTransform.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &object3dTransform.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &object3dTransform.translate.x, 0.01f);

			ImGui::ColorEdit4("color", &materialData->color.x);
			ImGui::TreePop();
		}

		ImGui::Separator();
		if (ImGui::TreeNode("Particle")) {
			ImGui::Checkbox("changeTexture", &changeTexture);
			if (ImGui::Button("Reload Particle")) {
				generateParticle = true;
			}
			ImGui::Combo("ParticleBlendMode", &particleBlendMode, "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");
			ImGui::TreePop();
		}

		ImGui::Separator();
		if (ImGui::TreeNode("Sprite")) {

			ImGui::Checkbox("showSprite", &showSprite);

			for (size_t i = 0; i < sprites.size(); ++i) {
				std::string label = "Sprite " + std::to_string(i);
				if (ImGui::TreeNode(label.c_str())) {
					Sprite* sprite = sprites[i];

					Vector2 spritePosition = sprite->GetTranslate();
					float spriteRotation = sprite->GetRotation();
					Vector2 spriteScale = sprite->GetScale();
					Vector4 spriteColor = sprite->GetColor();

					ImGui::SliderFloat2("translateSprite", &spritePosition.x, 0.0f, 1000.0f);
					ImGui::SliderFloat("rotateSprite", &spriteRotation, -6.28f, 6.28f);
					ImGui::SliderFloat2("scaleSprite", &spriteScale.x, 1.0f, Win32Window::kClientWidth);
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

					ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
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
		if (ImGui::TreeNode("Light")) {
			ImGui::ColorEdit4("colorLight", &directionalLightData->color.x);
			ImGui::DragFloat3("directionLight", &directionalLightData->direction.x, 0.01f);
			ImGui::DragFloat("intensityLight", &directionalLightData->intensity, 0.01f);
			ImGui::CheckboxFlags("enableLighting", reinterpret_cast<uint32_t*>(&materialData->enableLighting), 1 << 0);
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
		if (controlMaterial) {
			ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
			ImGui::Begin("Control Material Mode");
			ImGui::Text("A/D: Left/Right");
			ImGui::Text("W/S: Up/Down");
			ImGui::Text("Q/E: Forward/Backward");
			ImGui::Text("UP/DOWN: Scale Up/Down");
			ImGui::Text("C/Z: Rotate Left/Right");
			ImGui::Text("X: Reset Rotation");
			ImGui::Text("Space: Generate Particle");
			ImGui::End();
		}

#pragma endregion ここまで

		// ゲームの処理-----------------------------------------------------------------------------------

#pragma region 更新処理

		// inputを更新
		input->Update();

		// キーボードの入力処理

		audioManager.CleanupFinishedVoices(); // 完了した音声のクリーンアップ

		if (controlMaterial) {
			if (input->IsKeyDown(DIK_A)) { // Aキーが押されている間は左移動
				object3dTransform.translate.x -= 0.1f;
			}
			if (input->IsKeyDown(DIK_D)) { // Dキーが押されている間は右移動
				object3dTransform.translate.x += 0.1f;
			}
			if (input->IsKeyDown(DIK_W)) { // Wキーが押されている間は上移動
				object3dTransform.translate.y += 0.1f;
			}
			if (input->IsKeyDown(DIK_S)) { // Sキーが押されている間は下移動
				object3dTransform.translate.y -= 0.1f;
			}
			if (input->IsKeyDown(DIK_Q)) { // Qキーが押されている間は前進 
				object3dTransform.translate.z += 0.1f;
			}
			if (input->IsKeyDown(DIK_E)) { // Eキーが押されている間は後退
				object3dTransform.translate.z -= 0.1f;
			}

			if (input->IsKeyDown(DIK_UP)) { // 上キーが押されている間は拡大
				object3dTransform.scale += Vector3(0.01f, 0.01f, 0.01f);
			}
			if (input->IsKeyDown(DIK_DOWN)) { // 下キーが押されている間は縮小
				object3dTransform.scale -= Vector3(0.01f, 0.01f, 0.01f);
			}
			if (input->IsKeyDown(DIK_C)) { // Cキーが押されている間は左に回転
				object3dTransform.rotate.y -= 0.02f;
			}
			if (input->IsKeyDown(DIK_Z)) { // Zキーが押されている間は右に回転
				object3dTransform.rotate.y += 0.02f;
			}
			if (input->IsKeyReleased(DIK_X)) { // Xキーが離されたら回転をリセット
				object3dTransform.rotate.y = 0.0f;
			}
		}

		//object3dTransform.rotate.y += 0.01f;

		// カメラの更新
		if (!controlMaterial) {
			debugCamera.Update(*input);
		}
		currentViewMatrix = debugCamera.GetViewMatrix();

		// particle用データの更新

		// 現在有効なインスタンスデータを、GPU転送用バッファ(particleData)のどこに詰めるかを示すインデックス
		uint32_t currentLiveIndex = 0;

		if (input->IsKeyTriggered(DIK_SPACE)) {
			generateParticle = true;
		}

		if (generateParticle) {
			for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
				particles[index] = MakeNewParticle(particleRandomEngine);
				// 発生位置をオブジェクトの位置に合わせる
				particles[index].transform.translate = particles[index].transform.translate + object3dTransform.translate;
			}
			generateParticle = false;
		}

		for (uint32_t index = 0; index < kNumMaxParticle; ++index) {
			// パーティクル実体の更新
			particles[index].transform.translate = particles[index].transform.translate + (particles[index].velocity * kDeltaTime);
			particles[index].currentTime += kDeltaTime; // 経過時間を加算

			// アルファ値を計算 (透明化の処理)
			float alpha = 1.0f - (particles[index].currentTime / particles[index].lifeTime);

			if (particles[index].currentTime >= particles[index].lifeTime) {
				// 寿命が尽きた場合、新しいパーティクルとして再初期化（リサイクル）
				particles[index] = MakeNewParticle(particleRandomEngine);
				// 発生位置をオブジェクトの位置に合わせる
				particles[index].transform.translate = particles[index].transform.translate + object3dTransform.translate;
			}

			// パーティクルのワールド行列とWVP行列を計算して転送

			// 行列の計算
			Matrix4x4 particleWorldMatrix = MakeAffineMatrix(particles[index].transform.scale, particles[index].transform.rotate, particles[index].transform.translate);
			Matrix4x4 particleWVPMatrix = Multiply(particleWorldMatrix, Multiply(currentViewMatrix, object3dProjectionMatrix));

			// 転送
			particleData[currentLiveIndex].WVP = particleWVPMatrix;
			particleData[currentLiveIndex].World = particleWorldMatrix;
			particleData[currentLiveIndex].color = particles[index].color;

			// 計算したアルファ値を適用
			particleData[currentLiveIndex].color.w = alpha; // 徐々に透明にする	

			currentLiveIndex++; // 有効なパーティクル数をカウント
		}
		// 描画数を更新
		uint32_t numParticleToDraw = currentLiveIndex;

		// 三角形
		Matrix4x4 object3dWorldMatrix = MakeAffineMatrix(object3dTransform.scale, object3dTransform.rotate, object3dTransform.translate);
		Matrix4x4 object3dWVPMatrix = Multiply(object3dWorldMatrix, Multiply(currentViewMatrix, object3dProjectionMatrix));
		object3dData->WVP = object3dWVPMatrix; // World-View-Projection行列をWVPメンバーに入れる
		object3dData->World = object3dWorldMatrix;           // 純粋なワールド行列をWorldメンバーに入れる

		// スプライト
		for (size_t i = 0; i < sprites.size(); ++i) {
			sprites[i]->Update();
		}

#pragma endregion ここまで

		////ここまで----------------------------------------------------------------------------------------

		//// 3. グラフィックコマンド

		//// 描画の処理-------------------------------------------------------------------------------------

#pragma region 描画処理

		// 描画前処理
		dxBase->PreDraw();

		// コマンドを積む

		/// object3dの描画---------------------------------------

		// object3d用のRootSignatureを設定
		dxBase->GetCommandList()->SetGraphicsRootSignature(object3dRootSignature.Get());
		// 現在のブレンドモードに応じたPSOを設定
		if (currentBlendMode >= 0 && currentBlendMode < kCountOfBlendMode) {
			// PSOを設定
			dxBase->GetCommandList()->SetPipelineState(object3dPsoArray[currentBlendMode].Get());
		} else {
			// 不正な値の場合
			dxBase->GetCommandList()->SetPipelineState(object3dPsoArray[kBlendModeNormal].Get());
		}
		// VBVを設定
		dxBase->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
		// IBVを設定
		//dxBase->GetCommandList()->IASetIndexBuffer(&indexBufferView);
		// トポロジを設定
		dxBase->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		/// ルートパラメータを設定
		// マテリアルCBVの場所を設定
		dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		// WVP用CBVの場所を設定
		dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
		// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
		dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelTextureIndex));
		// DirectionalLightのCBufferの場所を設定 (PS b1, rootParameter[3]に対応)
		dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress()); // directionalLightResourceはライトのCBV
		// 描画！（DrawCall/ドローコール）
		if (showMaterial) {
			dxBase->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
		}
		//commandList->DrawIndexedInstanced(totalIndexCount, 1, 0, 0, 0); // 引数を totalIndexCount に変更

		/// Particle描画---------------------------------------

		// RootSignatureを設定
		dxBase->GetCommandList()->SetGraphicsRootSignature(particleRootSignature.Get());
		// 現在のブレンドモードに応じたPSOを設定
		if (particleBlendMode >= 0 && particleBlendMode < kCountOfBlendMode) {
			// PSOを設定
			dxBase->GetCommandList()->SetPipelineState(particlePsoArray[particleBlendMode].Get());
		} else {
			// 不正な値の場合
			dxBase->GetCommandList()->SetPipelineState(particlePsoArray[kBlendModeNormal].Get());
		}
		// VBVを設定
		dxBase->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
		// トポロジを設定
		dxBase->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		/// ルートパラメータを設定
		// マテリアルCBVの場所を設定
		dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		// インスタンス用SRVのDescriptorTableの先頭を設定。1はrootParametersForInstancing[1]である。
		dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(1, instanceSrvHandleGPU);
		// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
		dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(2,
			changeTexture ?
			TextureManager::GetInstance()->GetSrvHandleGPU(particleTextureIndex) :
			TextureManager::GetInstance()->GetSrvHandleGPU(uvCheckerIndex)
		);

		// 描画！（DrawCall/ドローコール）
		if (numParticleToDraw > 0) {
			dxBase->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), numParticleToDraw, 0, 0);
		}

		/// スプライト-------------------------------
		// スプライト用の設定
		spriteCommon->SetCommonDrawSettings(static_cast<BlendState>(currentBlendMode));
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

#pragma endregion ここまで
	}

#pragma region 解放処理

	//// 解放処理
	//CloseHandle(fenceEvent);

	if (input) {
		delete input;
		input = nullptr;
	}

	// ImGuiの終了処理
	// 初期化と逆順に行う
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// TextureManagerの終了
	TextureManager::GetInstance()->Finalize();

	// Spriteの解放
	for (size_t i = 0; i < sprites.size(); ++i) {
		delete sprites[i];
	}

	// SpriteBaseの解放
	delete spriteCommon;

	// DerectXの解放
	delete dxBase;

	// WindowsAPIの終了処理
	window->Finalize();

	// WindowsAPI解放
	delete window;
	window = nullptr;

#pragma endregion ここまで

	return 0;
}