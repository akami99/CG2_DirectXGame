#include "../Audio/Sound.h" // Sound.h をインクルード
#include "Logger.h"

#include <format>
#include <cassert>          // assert のために必要

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

using Microsoft::WRL::ComPtr;

// コンストラクタの実装
Sound::Sound() {
  // メンバー変数を初期化
  wfex = {};
}

// デストラクタの実装
Sound::~Sound() {
}

// ムーブコンストラクタの実装
Sound::Sound(Sound &&other) noexcept
    : wfex(other.wfex),
      buffer(std::move(other.buffer)) // vector の中身を効率よく移動
{
  // other のリソースを無効化
  other.wfex = {};
}

// ムーブ代入演算子の実装
Sound &Sound::operator=(Sound &&other) noexcept {
  if (this != &other) {
    Unload(); // 既存のリソースを解放

    // other のリソースをこのオブジェクトに移動
    wfex = other.wfex;
    buffer = std::move(other.buffer);

    // other のリソースを無効化
    other.wfex = {};
  }
  return *this;
}

// WAVファイルをロードする
bool Sound::Load(const std::string &filePath) {
  // 既存のデータを解放
  Unload();

  // --- パスの調整を最初に行う ---
  std::string fullPath = filePath;
  // 既にパスが含まれているかチェックし、無ければフォルダ名を追加
  if (filePath.find("Resources/Assets/Sounds/") == std::string::npos &&
      filePath.find("resources/assets/sounds/") == std::string::npos) {
      fullPath = "Resources/Assets/Sounds/" + filePath;
  }

  // 1. 文字列をワイド文字列に変換 (MF用)
  // fullPath を使うように変更
  std::wstring wFilePath(fullPath.begin(), fullPath.end());

  // 2. ソースリーダーの作成
  ComPtr<IMFSourceReader> pReader;
  HRESULT hr =
      MFCreateSourceReaderFromURL(wFilePath.c_str(), nullptr, &pReader);
  if (FAILED(hr))
    return false;

  // 3. 出力フォーマットの設定 (非圧縮PCMを指定)
  ComPtr<IMFMediaType> pMediaType;
  MFCreateMediaType(&pMediaType);
  pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  pMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
  pReader->SetCurrentMediaType(
      static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr,
      pMediaType.Get());

  // 4. 実際に適用されたフォーマットの取得
  ComPtr<IMFMediaType> pOutputMediaType;
  pReader->GetCurrentMediaType(
      static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
      &pOutputMediaType);

  WAVEFORMATEX *pWfex = nullptr;
  UINT32 cbFormat = 0;
  hr = MFCreateWaveFormatExFromMFMediaType(pOutputMediaType.Get(), &pWfex, &cbFormat);

  if (SUCCEEDED(hr)) {
      // 構造体全体を安全にコピー（サイズに応じて）
      memcpy(&wfex, pWfex, (cbFormat > sizeof(wfex)) ? sizeof(wfex) : cbFormat);

      // --- ここからロガーによるログ出力 ---
      std::string logMsg = std::format(
          "[Audio Load] File: {} | SampleRate: {} Hz | Channels: {} | Bits: {}\n",
          filePath,                  // 読み込んだファイル名
          wfex.Format.nSamplesPerSec, // サンプリングレート (例: 44100, 48000)
          wfex.Format.nChannels,      // チャンネル数 (例: 1, 2)
          wfex.Format.wBitsPerSample  // ビット数 (例: 16)
      );
      Logger::Log(logMsg);
      // --- ここまで ---

      CoTaskMemFree(pWfex);
  }

  // 5. reserveによる最適化 (全体の時間を取得して計算)
  PROPVARIANT var;
  PropVariantInit(&var); // 初期化を確実に行う
  hr = pReader->GetPresentationAttribute(
      static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);

  if (SUCCEEDED(hr)) {
      // duration は 100ナノ秒単位の整数 (1秒 = 10,000,000 unit)
      LONGLONG duration = var.uhVal.QuadPart;

      // wfex が WAVEFORMATEXTENSIBLE の場合、Format.nAvgBytesPerSec を参照します
      // 精度を保つため double で計算してから size_t にキャストします
      size_t estimatedSize = static_cast<size_t>(
          (static_cast<double>(duration) / 10000000.0) * wfex.Format.nAvgBytesPerSec);

      // 余裕を持って少し多めに確保するか、計算通りに確保
      if (estimatedSize > 0) {
          buffer.reserve(estimatedSize);
      }
      PropVariantClear(&var);
  }

  // 6. データの読み込みループ
  while (true) {
    DWORD flags = 0;
    ComPtr<IMFSample> pSample;
    hr = pReader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr,
        &flags, nullptr, &pSample);

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
      break; // 終端
    if (FAILED(hr) || !pSample)
      continue;

    // サンプルからバッファを取り出す
    ComPtr<IMFMediaBuffer> pMediaBuffer;
    pSample->ConvertToContiguousBuffer(&pMediaBuffer);

    BYTE *pData = nullptr;
    DWORD currentLength = 0;
    pMediaBuffer->Lock(&pData, nullptr, &currentLength);

    // vectorの末尾にデータを追加
    buffer.insert(buffer.end(), pData, pData + currentLength);

    pMediaBuffer->Unlock();
  }

  return true;
}

// 音声データを解放する
void Sound::Unload() {
  buffer.clear();
  wfex = {}; // フォーマット情報もリセット
}
