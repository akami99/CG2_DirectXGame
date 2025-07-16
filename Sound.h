#pragma once

#include <xaudio2.h> // WAVEFORMATEX, IXAudio2, IXAudio2SourceVoice, XAUDIO2_BUFFER のために必要
#include <string>     // std::string のために必要
#include <memory>     // std::unique_ptr を使うため (今回は直接使わないが、将来的な拡張のために残しておく)
#include <wrl/client.h> // ComPtr を使うため (今回は直接使わないが、Play メソッド内で IXAudio2SourceVoice の管理に使う場合がある)

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

class Sound {
public:
    // コンストラクタ
    Sound();

    // デストラクタ
    ~Sound();

    // コピーコンストラクタとコピー代入演算子を禁止
    // サウンドデータはユニークなリソースなので、コピーは避けるのが一般的
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    // ムーブコンストラクタとムーブ代入演算子を定義
    Sound(Sound&& other) noexcept;
    Sound& operator=(Sound&& other) noexcept;

    // WAVファイルをロードする
    bool LoadWave(const std::string& filePath);

    // 音声データを解放する
    void Unload();

    //// 音声データを再生する
    //// IXAudio2* は AudioManager から渡される必要がある
    //void Play(Microsoft::WRL::ComPtr<IXAudio2> xAudio2) const;

    // ゲッター
    const WAVEFORMATEX& GetWaveFormatEx() const;
    BYTE* GetBuffer() const;
    unsigned int GetBufferSize() const;

private:
    WAVEFORMATEX wfex;
    BYTE* pBuffer;
    unsigned int bufferSize;
};