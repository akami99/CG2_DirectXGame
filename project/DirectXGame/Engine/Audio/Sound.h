#pragma once

#include <string> // std::string のために必要
#include <vector>       // std::vector のために必要
#include <wrl/client.h> // ComPtr を使うため
#include <xaudio2.h> // WAVEFORMATEX, IXAudio2, IXAudio2SourceVoice, XAUDIO2_BUFFER のために必要

// チャンクヘッダ
struct ChunkHeader {
  char id[4];   //!< チャンク毎のID
  int32_t size; //!< チャンクのサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
  ChunkHeader chunk; //!< "RIFF"
  char type[4];      //!< "WAVE"
};

// FMTチャンク
struct FormatChunk {
  ChunkHeader chunk; //!< "fmt"
  WAVEFORMATEX fmt;  //!< 波形フォーマット
};

// Soundクラス
class Sound {
public:
  // コンストラクタ
  Sound();

  // デストラクタ
  ~Sound();

  // コピーコンストラクタとコピー代入演算子を禁止
  // サウンドデータはユニークなリソースなので、コピーは避けるのが一般的
  Sound(const Sound &) = delete;
  Sound &operator=(const Sound &) = delete;

  // ムーブコンストラクタとムーブ代入演算子を定義
  Sound(Sound &&other) noexcept;
  Sound &operator=(Sound &&other) noexcept;

  // 読み込み関数 (WAV, MP3, AACなど対応)
  bool Load(const std::string &filePath);

  // 音声データを解放する
  void Unload();

  // ゲッター

  // WaveFormatEx のゲッター
  const WAVEFORMATEXTENSIBLE &GetWaveFormatEx() const { return wfex; }
  // バッファのゲッター
  const BYTE *GetBuffer() const { return buffer.data();
  }
  // バッファサイズのゲッター
  unsigned int GetBufferSize() const { return static_cast<unsigned int>(buffer.size()); }

private:
	WAVEFORMATEXTENSIBLE wfex;
  std::vector<BYTE> buffer; // 生ポインタから vector に変更
};