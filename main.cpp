//#include <Windows.h>
//#include <cstdint>
//#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dbghelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
//#include <dxcapi.h>
#include <vector>
#include <numbers>
#include <DirectXMath.h>
//#include <iostream>
#include <sstream>
//#include <wrl.h>

#include "EngineMath.h"
#include "EngineMathFunctions.h"
#include "MatrixGenerators.h"
#include "WindowWrapper.h"
#include "DirectXBase.h"
#include "AudioManager.h"
#include "Input.h"
#include "DebugCamera.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// .lidはヘッダに書いてはいけない
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};// 頂点データの構造体。16+8+12=36バイト。float*4+float*2+float*3

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};// 変換行列を構成するための構造体。Vector3*3=36バイト

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3]; // 16バイトのアライメントを確保するためのパディング
	Matrix4x4 uvTransform;
};// マテリアルの構造体。Vector4(16)+int32_t(4)=20バイト + float*3(12)=32バイト

struct DirectionalLight {
	Vector4 color; //!< ライトの色
	Vector3 direction; //!< ライトの向き(正規化する)
	float _padding; //!< 16バイトのアライメントを確保するためのパディング
	float intensity; //!< 輝度
};// DirectionalLightの構造体。Vector4(16)+Vector3(12)+float(4)=32バイト + float(4)=36バイト

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};// 変換行列をまとめた構造体。Matrix4x4*2=128バイト

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
//
//static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
//	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
//	SYSTEMTIME time;
//	GetLocalTime(&time);
//	wchar_t filePath[MAX_PATH] = { 0 };
//	CreateDirectory(L"./Dumps", nullptr);
//	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
//	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
//	// processId（このexeのID）とクラッシュ（例外）の発生したthreadIdを取得
//	DWORD processId = GetCurrentProcessId();
//	DWORD threadId = GetCurrentThreadId();
//	// 設定情報を入力
//	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
//	minidumpInformation.ThreadId = threadId;
//	minidumpInformation.ExceptionPointers = exception;
//	minidumpInformation.ClientPointers = TRUE;
//	// Dumpを出力、MiniDumpNormalは最低限の情報を出力するフラグ
//	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
//	// 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
//	return EXCEPTION_EXECUTE_HANDLER;
//}
//
//// Shaderをコンパイルする関数
//ComPtr<IDxcBlob> CompileShader(
//	// CompilerするShaderファイルへのパス
//	const std::wstring& filePath,
//	// Compileに使用するProfile
//	const wchar_t* profile,
//	// 初期化で生成したものを3つ
//	ComPtr<IDxcUtils> dxcUtils,
//	ComPtr<IDxcCompiler3> dxcCompiler,
//	ComPtr<IDxcIncludeHandler> includeHandler) {
//	// 1.hlslファイルを読む
//	// これからシェーダーをコンパイルする旨をログに出す
//	Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
//	// hlslファイルを読む
//	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
//	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
//	// 読めなかったら止める
//	assert(SUCCEEDED(hr));
//	// 読み込んだファイルの内容を設定する
//	DxcBuffer shaderSourceBuffer;
//	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
//	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
//	shaderSourceBuffer.Encoding = DXC_CP_UTF8;  // UTF8の文字コードであることを通知
//
//	// 2.Compileする
//	LPCWSTR arguments[] = {
//		filePath.c_str(), // コンパイル対象のhlslファイル名
//		L"-E", L"main", // エントリーポイントの指定。基本的にmain以外には市内
//		L"-T", profile, // ShaderProfileの設定
//		L"-Zi", L"-Qembed_debug",   // デバッグ用の情報を埋め込む
//		L"-Od",     // 最適化を外しておく
//		L"-Zpr",     // メモリレイアウトは行優先
//	};
//	// 実際にShaderをコンパイルする
//	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
//	hr = dxcCompiler->Compile(
//		&shaderSourceBuffer, // 読み込んだファイル
//		arguments,           // コンパイルオプション
//		_countof(arguments),  // コンパイルオプションの数
//		includeHandler.Get(),      // includeが含まれた諸々
//		IID_PPV_ARGS(&shaderResult) // コンパイル結果
//	);
//	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
//	assert(SUCCEEDED(hr));
//
//	// 3.警告・エラーがでていないか確認する
//	// 警告・エラーが出てたらログに出して止める
//	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
//	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
//	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
//		Log(shaderError->GetStringPointer());
//		// 警告・エラーダメゼッタイ
//		assert(false);
//	}
//
//	// 4.Compile結果を受け取って返す
//	// コンパイル結果から実行用のバイナリ部分を取得
//	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
//	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
//	assert(SUCCEEDED(hr));
//	// 成功したログを出す
//	Log(ConvertString(std::format(L"CompileSucceded, path:{}, profile:{}\n", filePath, profile)));
//	// 実行用のバイナリを返却
//	return shaderBlob;
//}
//
//ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
//	// ヒープ設定（UploadHeap）
//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
//
//	// リソース設定（バッファ用）
//	D3D12_RESOURCE_DESC vertexResourceDesc{};
//	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	vertexResourceDesc.Width = sizeInBytes;
//	vertexResourceDesc.Height = 1;
//	vertexResourceDesc.DepthOrArraySize = 1;
//	vertexResourceDesc.MipLevels = 1;
//	vertexResourceDesc.SampleDesc.Count = 1;
//	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//
//	// リソース作成
//	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&uploadHeapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&vertexResourceDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&vertexResource)
//	);
//	assert(SUCCEEDED(hr));
//
//	return vertexResource;
//}
//
//DirectX::ScratchImage LoadTexture(const std::string& filePath) {
//	// テクスチャを読み込んでプログラムで扱えるようにする
//	DirectX::ScratchImage image{};
//	std::wstring filePathW = ConvertString(filePath);
//	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
//	assert(SUCCEEDED(hr));
//
//	// ミップマップの作成
//	DirectX::ScratchImage mipImages{};
//	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
//	assert(SUCCEEDED(hr));
//
//	// ミップマップ付きのデータを返す
//	return mipImages;
//}
//
//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& fileName) {
//	// 1. 中で必要となる変数の宣言
//	MaterialData materialData; // 構築するMaterialData
//	std::string line; // ファイルから読んだ1行を格納するもの
//	// 2. ファイルを開く
//	std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
//	assert(file.is_open()); // とりあえず開けなかったら止める
//	// 3. 実際にファイルを読み、MaterialDataを構築していく
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier; // 先頭の識別子を読む
//
//		// identifierに応じた処理
//		if (identifier == "map_Kd") { // テクスチャファイルパス
//			std::string textureFilename;
//			s >> textureFilename;
//			// 連結してテクスチャファイルパスにする
//			materialData.textureFilePath = directoryPath + "/" + textureFilename;
//		}
//	}
//
//	// 4. MaterialDataを返す
//	return materialData;
//}
//
//ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
//	// 1. 中で必要となる変数の宣言
//	ModelData modelData; // 構築するModelData
//	std::vector<Vector4> positions; // 位置
//	std::vector<Vector3> normals; // 法線
//	std::vector<Vector2> texcoords; // テクスチャ座標
//	std::string line; // ファイルから読んだ1行を格納するもの
//	// 2. ファイルを開く
//	std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
//	assert(file.is_open()); // とりあえず開けなかったら止める
//	// 3. 実際にファイルを読み、ModelDataを構築していく
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier; // 先頭の識別子を読む
//
//		// identifierに応じた処理
//		if (identifier == "v") { // 頂点位置
//			Vector4 position;
//			s >> position.x >> position.y >> position.z; // x, y, zを読む
//			position.w = 1.0f; // wは1.0fに設定
//			positions.push_back(position); // positionsに追加
//		} else if (identifier == "vt") { // テクスチャ座標
//			Vector2 texcoord;
//			s >> texcoord.x >> texcoord.y; // x, yを読む
//			texcoord.y = 1.0f - texcoord.y; // Y軸を反転させる
//			texcoords.push_back(texcoord); // texcoordsに追加
//		} else if (identifier == "vn") { // 法線ベクトル
//			Vector3 normal;
//			s >> normal.x >> normal.y >> normal.z; // x, y, zを読む
//			normals.push_back(normal); // normalsに追加
//		} else if (identifier == "f") { // 面情報
//			VertexData triangle[3];
//			// 面は三角形限定。その他は未対応
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
//				std::string vertexDefinition;
//				s >> vertexDefinition;
//				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
//				std::istringstream v(vertexDefinition);
//				uint32_t elementIndices[3];
//				for (int32_t element = 0; element < 3; ++element) {
//					std::string index;
//					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
//					elementIndices[element] = std::stoi(index);
//				}
//				// 要素へのIndexから、実際の要素の値を取得して、頂点を読んでいく
//				Vector4 position = positions[elementIndices[0] - 1]; // OBJは1-indexedなので-1する
//				//position.x *= -1.0f; // X軸を反転する
//				Vector2 texcoord = texcoords[elementIndices[1] - 1]; // OBJは1-indexedなので-1する
//				Vector3 normal = normals[elementIndices[2] - 1]; // OBJは1-indexedなので-1する
//				//normal.x *= -1.0f; // X軸を反転する
//				VertexData vertex = { position, texcoord, normal }; // 頂点データを構築
//				modelData.vertices.push_back(vertex); // ModelDataに追加
//				triangle[faceVertex] = { position, texcoord, normal };
//			}
//			// 頂点を逆順で登録することで、周り順を逆にする
//			modelData.vertices.push_back(triangle[2]);
//			modelData.vertices.push_back(triangle[1]);
//			modelData.vertices.push_back(triangle[0]);
//		} else if (identifier == "mtllib") {
//			// materialTemplateLibraryファイルの名前を取得する
//			std::string materialFileName;
//			s >> materialFileName;
//			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFileName);
//		}
//	}
//	// 4. ModelDataを返す
//	return modelData;
//}
//
//ComPtr<ID3D12Resource> CreateTextureResource(ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata) {
//	// metadataを基にResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = UINT(metadata.width); // Textureの幅
//	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
//	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
//	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き or 配列Textureの配列数
//	resourceDesc.Format = metadata.format; // TextureのFormat
//	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定。
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元
//
//	// 利用するHeapの設定。非常に特殊な運用。->一般的なケースに変更
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う->VRAM上に作る
//	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
//	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置
//
//	// Resourceを生成する
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, // Heapの設定
//		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
//		&resourceDesc,  // Resourceの設定
//		D3D12_RESOURCE_STATE_COPY_DEST,  // データ転送される設定(03_00ex)
//		nullptr, // Clear最適値。使わないでnullptr
//		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
//	assert(SUCCEEDED(hr));
//	return resource;
//}
//
//[[nodiscard]] // 戻り値を破棄しない
//ComPtr<ID3D12Resource> UploadTextureData(ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
//	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
//	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
//	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
//	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
//	// Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
//	D3D12_RESOURCE_BARRIER barrier{};
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = texture.Get();
//	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
//	commandList->ResourceBarrier(1, &barrier);
//	return intermediateResource;
//
//}
//
//struct D3DResourceLeakChecker {
//	~D3DResourceLeakChecker() {
//		// リソースリークチェック
//		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
//		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
//			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
//			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
//			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
//		}
//	}
//};

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	//// 誰も捕捉しなかった場合に(Unhandled)、補足する関数を登録
	//// main関数始まってすぐに登録すると良い
	//SetUnhandledExceptionFilter(ExportDump);

	//// COMの初期化
	//CoInitializeEx(0, COINIT_MULTITHREADED);

	//D3DResourceLeakChecker leakCheck; // リソースリークチェック用のオブジェクト

	//// ログのディレクトリを用意
	//std::filesystem::create_directory("logs");

	//// 現在時刻を取得（UTC時刻）
	//std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	//// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	//std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
	//	nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	//// 日本時間（PCの設定時間）に変換
	//std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	//// formatを使って年月日_時分秒の文字列に変換
	//std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//// 時刻を使ってファイル名を決定
	//std::string logFilePath = std::string("logs/") + dateString + ".log";
	//// ファイルを作って書き込み準備
	//std::ofstream logStream(logFilePath);


#pragma region WindowsAPIの初期化
	// WindowsAPIの初期化

	// ポインタ
	WindowWrapper* window = nullptr;

	// WindowWrapperの生成と初期化
	window = new WindowWrapper();
	window->Initialize();

#pragma endregion

#pragma region DirectXの初期化
	// DirectXの初期化

	// ポインタ
	DirectXBase* dxBase = nullptr;
	// DirectXBaseの生成と初期化
	dxBase = new DirectXBase();
	dxBase->Initialize(window);

#pragma endregion
	//
	//	// RootSignature作成
	//	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//
	//	// RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform 
	//	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う(b0のbと一致する)
	//	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;   // PixelShaderで使う
	//	rootParameters[0].Descriptor.ShaderRegister = 0;    // レジスタ番号0とバインド(b0の0と一致する。もしb11と紐づけたいなら11となる)
	//	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う(b0のbと一致する)
	//	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;   // VertexShaderで使う
	//	rootParameters[1].Descriptor.ShaderRegister = 0;    // レジスタ番号0
	//	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	//	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ
	//
	//	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//	descriptorRange[0].BaseShaderRegister = 0;  // 0から始まる
	//	descriptorRange[0].NumDescriptors = 1;  // 数は1つ
	//	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	//	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算
	//	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	//	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;  // Tableの中身の配列を指定
	//	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
	//
	//	//ライティング
	//	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//	rootParameters[3].Descriptor.ShaderRegister = 1;  // レジスタ番号1を使う
	//
	//	// Samplerの設定
	//	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	//	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;   // 0~1の範囲外をリピート
	//	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;   // 比較しない
	//	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;   // ありったけのMipmapを使う
	//	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//	staticSamplers[0].ShaderRegister = 0;   // レジスタ番号0を使う
	//	descriptionRootSignature.pStaticSamplers = staticSamplers;
	//	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	//
	//	// シリアライズしてバイナリにする
	//	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	//	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	//	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	//	if (FAILED(hr)) {
	//		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	//		assert(false);
	//	}
	//	// バイナリを元に生成
	//	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	//	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	//	assert(SUCCEEDED(hr));
	//
	//	// InputLayout
	//	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	//	inputElementDescs[0].SemanticName = "POSITION";
	//	inputElementDescs[0].SemanticIndex = 0;
	//	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	inputElementDescs[0].InputSlot = 0; // ★明示的に指定
	//	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//	inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // ★明示的に指定
	//	inputElementDescs[0].InstanceDataStepRate = 0; // デフォルト値だが明示するとより分かりやすい
	//	inputElementDescs[1].SemanticName = "TEXCOORD";
	//	inputElementDescs[1].SemanticIndex = 0;
	//	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	//	inputElementDescs[1].InputSlot = 0; // ★明示的に指定
	//	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//	inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // ★明示的に指定
	//	inputElementDescs[1].InstanceDataStepRate = 0;
	//	inputElementDescs[2].SemanticName = "NORMAL";
	//	inputElementDescs[2].SemanticIndex = 0;
	//	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//	inputElementDescs[2].InputSlot = 0; // ★明示的に指定
	//	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//	inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // ★明示的に指定
	//	inputElementDescs[2].InstanceDataStepRate = 0;
	//	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	//	inputLayoutDesc.NumElements = _countof(inputElementDescs);
	//
	//#pragma region ブレンドモード
	//	// BlendState
	//	enum BlendMode {
	//		//!< ブレンド無し
	//		kBlendModeNone,
	//		//!< 通常aブレンド。デフォルト。Src * SrcA + Dest * (1 - SrcA)
	//		kBlendModeNormal,
	//		//!< 加算。Src * SrcA + Dest * 1
	//		kBlendModeAdd,
	//		//!< 減算。Dest * 1 - Src * SrcA
	//		kBlendModeSubtract,
	//		//!< 乗算。Src * 0 + Dest * SrcA
	//		kBlendModeMultiply,
	//		//!< スクリーン。Src * (1 - DestA) + Dest * 1
	//		kBlendModeScreen,
	//		//!< 利用してはいけない
	//		kCountOfBlendMode,
	//	};
	//
	//	int currentBlendMode = kBlendModeNormal;
	//
	//	// BlendStateの設定を配列で用意しておく
	//	D3D12_BLEND_DESC blendDescs[kCountOfBlendMode] = {};
	//	// ブレンド無し
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeNone].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeNone].RenderTarget[0].BlendEnable = FALSE;
	//
	//	// 通常のアルファブレンド
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeNormal].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeNormal].RenderTarget[0].BlendEnable = TRUE;
	//	blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	//	blendDescs[kBlendModeNormal].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeNormal].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // 1.0f - ソースのアルファ値
	//	blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	//	blendDescs[kBlendModeNormal].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeNormal].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f
	//
	//	// 加算ブレンド
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeAdd].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeAdd].RenderTarget[0].BlendEnable = TRUE;
	//	blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	//	blendDescs[kBlendModeAdd].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeAdd].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
	//	blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	//	blendDescs[kBlendModeAdd].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeAdd].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f
	//
	//	// 減算ブレンド
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendEnable = TRUE;
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 減算
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f
	//
	//	// 乗算ブレンド
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendEnable = TRUE;
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO; // 0.0f
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // ソースカラー値
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f
	//
	//	// スクリーンブレンド
	//	// すべての色要素を書き込む
	//	blendDescs[kBlendModeScreen].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//	blendDescs[kBlendModeScreen].RenderTarget[0].BlendEnable = TRUE;
	//	blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // デストカラー値
	//	blendDescs[kBlendModeScreen].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeScreen].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // ソースアルファ値
	//	blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
	//	blendDescs[kBlendModeScreen].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
	//	blendDescs[kBlendModeScreen].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f
	//#pragma endregion
	//
	//	// RasterizerState
	//	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//	// 裏面をカリング
	//	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//	// 三角形の中を塗りつぶす
	//	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//
	//	// Shaderをコンパイルする
	//	ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	//	assert(vertexShaderBlob != nullptr);
	//	ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	//	assert(pixelShaderBlob != nullptr);
	//
	//	// PSOを生成する
	//
	//	// DepthStencilStateの設定
	//	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//	// Depthの機能を有効化する
	//	depthStencilDesc.DepthEnable = true;
	//	// 書き込みします
	//	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//	// 比較関数はLessEqual。つまり、近ければ描画される
	//	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//
	//	// ベースのPSO設定
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescBase{};
	//
	//	// **共通設定**を一度だけセット
	//	psoDescBase.pRootSignature = rootSignature.Get(); // RootSignature
	//	psoDescBase.InputLayout = inputLayoutDesc; // InputLayout
	//	psoDescBase.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() }; // VertexShader
	//	psoDescBase.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() }; // PixelShader
	//	psoDescBase.RasterizerState = rasterizerDesc; // RasterizerState
	//	// 書き込むRTVの情報
	//	psoDescBase.NumRenderTargets = 1;
	//	psoDescBase.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	psoDescBase.SampleDesc.Count = 1;
	//	// DepthStencilの設定
	//	psoDescBase.DepthStencilState = depthStencilDesc;
	//	psoDescBase.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//	// 利用するトポロジ（形状）のタイプ。三角形
	//	psoDescBase.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//	// どのように画面に色を打ち込むかの設定（気にしなくていい）
	//	psoDescBase.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//
	//	// ループで各ブレンドモードのPSOを生成
	//
	//	// 外部で宣言（クラスのメンバー変数など）
	//	ComPtr<ID3D12PipelineState> pPsoArray[kCountOfBlendMode];
	//
	//	for (int i = 0; i < kCountOfBlendMode; ++i) {
	//		if (i == kCountOfBlendMode) {
	//			continue;
	//		};
	//
	//		// 設定をベースからコピー
	//		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = psoDescBase;
	//
	//		// ブレンド設定だけを、現在のインデックス(i)に対応するものに切り替える
	//		psoDesc.BlendState = blendDescs[i];
	//
	//		// PSO生成
	//		// 結果を配列 m_pPsoArray[i] に格納
	//		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPsoArray[i]));
	//		assert(SUCCEEDED(hr));
	//	}

#pragma region DirectInputの初期化

	// DirectInputの初期化
	Input* input = new Input();
	input->Initialize(window);

#pragma endregion

#pragma region DebugCameraの初期化

	// DebugCameraの初期化
	DebugCamera debugCamera;
	debugCamera.Initialize();

#pragma endregion

	//// 描画初期化処理 (IMGUI)

	//// モデル読み込み
	//ModelData modelData = LoadObjFile("resources", "plane.obj");

	//// 実際に頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());
	////ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * totalUniqueVertexCount);
	//// 頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	//// 書き込むためのアドレスを取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 書き込むためのアドレスを取得
	//std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());// 頂点データをリソースにコピー

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//// リソースの先頭アドレスから使う
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点6つ分のサイズ
	//vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	////vertexBufferView.SizeInBytes = sizeof(VertexData) * totalUniqueVertexCount;
	//// １頂点当たりのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData);


	//// Sprite用の頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);
	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//// リソースの先頭アドレスから使う
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点6つ分のサイズ
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//// １頂点当たりのサイズ
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);


	//// スプライト
	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	//// 頂点0: 左下 (0,0)
	//vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	//vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };

	//// 頂点1: 左上 (640,0)
	//vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	//vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };

	//// 頂点2: 右下 (0,360)
	//vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	//vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };

	//// 頂点3: 右上 (640,360)
	//vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	//vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	//vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };

	//// IBVを作成する
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);

	//// インデックスバッファビューを作成する
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	//// リソースの先頭アドレスから使う
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズはインデックス6つ分のサイズ
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//// インデックスはuint32_tとする
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	//// インデックスリソースにデータを書き込む
	//uint32_t* indexDataSprite = nullptr;
	//indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	//indexDataSprite[0] = 0;
	//indexDataSprite[1] = 1;
	//indexDataSprite[2] = 2;
	//indexDataSprite[3] = 1;
	//indexDataSprite[4] = 3;
	//indexDataSprite[5] = 2;


	//// 球
	//// マテリアル用のリソースを作る。
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = CreateBufferResource(device, sizeof(Material));

	//// マテリアルにデータを書き込む
	//Material* materialData = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//// 色の書き込み
	//materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//materialData->enableLighting = true;
	//materialData->uvTransform = MakeIdentity4x4();

	//// sprite
	//// マテリアル用のリソースを作る。
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(device, sizeof(Material));

	//// マテリアルにデータを書き込む
	//Material* materialDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	//// 色の書き込み
	//materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//materialDataSprite->enableLighting = false;
	//materialDataSprite->uvTransform = MakeIdentity4x4();

	//// 平行光源
	//// データを書き込む
	//Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
	//DirectionalLight* directionalLightData = nullptr;
	//// 書き込むためのアドレスを取得
	//directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//// デフォルト値はとりあえず以下のようにしておく
	//directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	//directionalLightData->intensity = 1.0f;

	//// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
	//// データを書き込む
	//TransformationMatrix* transformationMatrixDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	//// 単位行列を書き込んでおく
	//transformationMatrixDataSprite->WVP = MakeIdentity4x4();
	//transformationMatrixDataSprite->World = MakeIdentity4x4();


	//// WVP用のリソースを作る。Matrix4x4　1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource = CreateBufferResource(device, sizeof(TransformationMatrix));
	//// データを書き込む
	//TransformationMatrix* wvpData = nullptr;
	//// 書き込むためのアドレスを取得
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//// 単位行列を書き込んでおく
	//wvpData->WVP = MakeIdentity4x4();
	//wvpData->World = MakeIdentity4x4();


	//// Transform変数を作る
	////三角形
	//Transform transform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	////スプライト
	//Transform transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	//// UVTransform用の変数を用意
	//Transform uvTransformSprite{
	//	{1.0f, 1.0f, 1.0f},
	//	{0.0f, 0.0f, 0.0f},
	//	{0.0f, 0.0f, 0.0f}
	//};

	//Matrix4x4 uvTransformMatrix = MakeAffineMatrix(uvTransformSprite.scale, uvTransformSprite.rotate, uvTransformSprite.translate);
	//materialDataSprite->uvTransform = uvTransformMatrix;

	//// リソースの読み込みとアップロード
	//DirectX::ScratchImage mipImages1 = LoadTexture("resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata1 = mipImages1.GetMetadata();
	//ComPtr<ID3D12Resource> textureResource1 = CreateTextureResource(device, metadata1);

	//ComPtr<ID3D12Resource> intermediateResource1 = UploadTextureData(textureResource1, mipImages1, device, commandList);

	//DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
	//const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	//ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);

	//ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);


	//// commandListをcloseし、commandQueue->ExecuteCommandListsを使いキックする
	//commandList->Close();
	//ComPtr<ID3D12CommandList> commandLists[] = { commandList };
	//commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists->GetAddressOf());

	//// 実行を待つ
	//fenceValue++;
	//commandQueue->Signal(fence.Get(), fenceValue);
	//if (fence->GetCompletedValue() < fenceValue) {
	//	fence->SetEventOnCompletion(fenceValue, fenceEvent);
	//	WaitForSingleObject(fenceEvent, INFINITE);
	//}

	//// 実行が完了したので、allcatorとcommandListをresetして次のコマンドを積めるようにする
	//commandAllocator->Reset();
	//commandList->Reset(commandAllocator.Get(), nullptr);


	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1{};
	//srvDesc1.Format = metadata1.format;
	//srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc1.Texture2D.MipLevels = UINT(metadata1.mipLevels);

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);


	//// SRVを作成するDescriptorHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU1 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU1 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);

	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	//// 先頭はImGuiが使っているのでその次を使う
	//textureSrvHandleCPU1.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU1.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//textureSrvHandleCPU2.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU2.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//// SRVの生成
	//device->CreateShaderResourceView(textureResource1.Get(), &srvDesc1, textureSrvHandleCPU1);

	//device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

	///// 変数の宣言と初期化
	//bool useMonsterBall = true;


	//// カメラの設定
	///*Transform cameraTransform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -10.0f } };
	//Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);*/
	//Matrix4x4 currentViewMatrix = debugCamera.GetViewMatrix();


	//// 三角形
	//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	//Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WindowWrapper::kClientWidth) / float(WindowWrapper::kClientHeight), 0.1f, 100.0f);
	//Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(currentViewMatrix, projectionMatrix));
	//wvpData->WVP = worldViewProjectionMatrix; // World-View-Projection行列をWVPメンバーに入れる
	//wvpData->World = worldMatrix;           // 純粋なワールド行列をWorldメンバーに入れる



	//// スプライト
	//Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	//Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
	//Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WindowWrapper::kClientWidth), float(WindowWrapper::kClientHeight), 0.0f, 100.0f);
	//Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
	//transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite; // World-View-Projection行列をWVPメンバーに入れる
	//transformationMatrixDataSprite->World = worldMatrixSprite;           // 純粋なワールド行列をWorldメンバーに入れる

	////スプライトの表示
	//bool showSprite = false;

	//// サウンド用の宣言
	//AudioManager audioManager;
	//// 1. AudioManager を初期化
	//if (!audioManager.Initialize()) {
	//	// 初期化失敗時の処理
	//	return 1;
	//}

	//// 2. 音声ファイルをロード
	//if (!audioManager.LoadSound("alarm1", "resources/Alarm01.wav")) {
	//	// ロード失敗時の処理
	//	return 1;
	//}

	// ウィンドウのxボタンが押されるまでループ
	// 1. ウィンドウメッセージ処理
	while (true) {
		// Windowのメッセージ処理
		if (window->ProcessMessage()) {
			// ゲームループを抜ける
			break;
		}

		//// 2. DIrectX更新処理

		//// フレームの開始を告げる
		//ImGui_ImplDX12_NewFrame();
		//ImGui_ImplWin32_NewFrame();
		//ImGui::NewFrame();

		//// 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
		////ImGui::ShowDemoWindow();
		//ImGui::Begin("Settings");
		//// デバッグウィンドウ
		//ImGui::Text("Camera");

		////ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f);
		////ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f);

		//ImGui::Separator();
		//ImGui::Text("Material");

		//ImGui::SliderAngle("rotate.x", &transform.rotate.x);
		//ImGui::SliderAngle("rotate.y", &transform.rotate.y);
		//ImGui::SliderAngle("rotate.z", &transform.rotate.z);

		//ImGui::ColorEdit4("color", &materialData->color.x);

		//ImGui::Combo("BlendMode", &currentBlendMode, "None\0Normal\0Add\0Subtractive\0Multiply\0Screen\0");

		//ImGui::Checkbox("useMonsterBall", &useMonsterBall);

		//ImGui::Separator();
		//ImGui::Text("Sprite");

		//ImGui::SliderFloat3("translateSprite", &transformSprite.translate.x, 0.0f, 1000.0f);
		//ImGui::ColorEdit4("colorSprite", &materialDataSprite->color.x);
		//ImGui::Checkbox("showSprite", &showSprite);

		//ImGui::Separator();
		//ImGui::Text("Light");

		//ImGui::ColorEdit4("colorLight", &directionalLightData->color.x);
		//ImGui::DragFloat3("directionLight", &directionalLightData->direction.x, 0.01f);
		//ImGui::DragFloat("intensityLight", &directionalLightData->intensity, 0.01f);
		//ImGui::CheckboxFlags("enableLighting", reinterpret_cast<uint32_t*>(&materialData->enableLighting), 1 << 0);

		//ImGui::Separator();
		//ImGui::Text("UV");

		//ImGui::DragFloat2("uvTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		//ImGui::DragFloat2("uvScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		//ImGui::SliderAngle("uvRotate", &uvTransformSprite.rotate.z);

		//if (ImGui::Button("Play Sound")) {
		//	// サウンドの再生
		//	audioManager.PlaySound("alarm1");
		//}

		//ImGui::End();

		//// ゲームの処理-----------------------------------------------------------------------------------
		//// inputを更新
		//input->Update();

		//// キーボードの入力処理テスト(サウンド再生)
		//if (input->IsKeyTriggered(DIK_SPACE)) { // スペースキーが押されているか
		//	// サウンドの再生
		//	audioManager.PlaySound("alarm1");
		//}
		//if (input->IsKeyDown(DIK_Z)) { // Zキーが押されている間は左に回転
		//	transform.rotate.y -= 0.02f;
		//}
		//if (input->IsKeyDown(DIK_C)) { // Cキーが押されている間は右に回転
		//	transform.rotate.y += 0.02f;
		//}
		//if (input->IsKeyReleased(DIK_X)) { // Rキーが離されたら回転をリセット
		//	transform.rotate.y = 0.0f;
		//}
		//audioManager.CleanupFinishedVoices(); // 完了した音声のクリーンアップ

		////transform.rotate.y += 0.01f;

		//// カメラの更新
		//debugCamera.Update(*input);
		//currentViewMatrix = debugCamera.GetViewMatrix();

		//// 三角形
		//worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		//worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(currentViewMatrix, projectionMatrix));
		//wvpData->WVP = worldViewProjectionMatrix;
		//wvpData->World = worldMatrix;

		//// スプライト
		//uvTransformMatrix = MakeAffineMatrix(uvTransformSprite.scale, uvTransformSprite.rotate, uvTransformSprite.translate);
		//materialDataSprite->uvTransform = uvTransformMatrix;

		//worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		//worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
		//transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
		//transformationMatrixDataSprite->World = worldMatrixSprite;

		////ここまで----------------------------------------------------------------------------------------

		//// ImGuiの内部コマンドを生成する
		//ImGui::Render();

		//// 3. グラフィックコマンド

		//// 描画の処理-------------------------------------------------------------------------------------

		// 描画前処理
		dxBase->PreDraw();

		//// コマンドを積む
		//// 三角形

		//// RootSignatureを設定。PSOに設定しているけど別途設定が必要
		//commandList->SetGraphicsRootSignature(rootSignature.Get());

		//if (currentBlendMode >= 0 && currentBlendMode < kCountOfBlendMode) {
		//	// PSOを設定
		//	commandList->SetPipelineState(pPsoArray[currentBlendMode].Get());
		//} else {
		//	// 不正な値の場合
		//	commandList->SetPipelineState(pPsoArray[kBlendModeNormal].Get());
		//}

		////commandList->IASetIndexBuffer(&indexBufferView); // IBVを設定

		//commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
		//// 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//// マテリアルCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//// WVP用のCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
		//// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
		//commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU1);

		//// DirectionalLightのCBufferの場所を設定 (PS b1, rootParameter[3]に対応)
		//commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress()); // directionalLightResourceはライトのCBV

		//// 描画！（DrawCall/ドローコール）。３頂点で１つのインスタンス。インスタンスについては今後
		//commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
		////commandList->DrawIndexedInstanced(totalIndexCount, 1, 0, 0, 0); // 引数を totalIndexCount に変更

		//// スプライト
		//commandList->IASetIndexBuffer(&indexBufferViewSprite); // IBVを設定
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite); // VBVを設定
		//// マテリアルCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
		////TransformationMatrixのCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
		//// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
		//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU1);

		//// 描画！（DrawCall/ドローコール）
		//if (showSprite) {
		//	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
		//}


		////ここまで-ImGui_ImplDX12_Init()--------------------------------------------------------------------------------------

		//// 実際のcommandListのImGuiの描画コマンドを積む
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

		// 描画後処理
		dxBase->PostDraw();
	}

	//// 出力ウィンドウへの文字出力
	//OutputDebugStringA("Hello,DirectX!\n");


	//// 解放処理
	//CloseHandle(fenceEvent);

	if (input) {
		delete input;
		input = nullptr;
	}

	//// ImGuiの終了処理。詳細はさして重要ではないので開設は省略する。
	//// こういうもんである。初期化と逆順に行う
	//ImGui_ImplDX12_Shutdown();
	//ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext();

	// DerectXの解放
	delete dxBase;

	// WindowsAPIの終了処理
	window->Finalize();

	// WindowsAPI解放
	delete window;
	window = nullptr;

	return 0;
}