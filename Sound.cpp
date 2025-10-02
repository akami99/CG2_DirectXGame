#include "Sound.h" // Sound.h をインクルード
#include <fstream>   // ファイル操作のために必要
#include <cassert>   // assert のために必要
#include <cstring>   // strncmp のために必要

// コンストラクタの実装
Sound::Sound() : pBuffer(nullptr), bufferSize(0) {
    // メンバー変数を初期化
    wfex = {};
}

// デストラクタの実装
Sound::~Sound() {
    Unload(); // メモリを解放
}

// ムーブコンストラクタの実装
Sound::Sound(Sound&& other) noexcept
    : wfex(other.wfex), pBuffer(other.pBuffer), bufferSize(other.bufferSize) {
    // other のリソースを無効化
    other.pBuffer = nullptr;
    other.bufferSize = 0;
    other.wfex = {};
}

// ムーブ代入演算子の実装
Sound& Sound::operator=(Sound&& other) noexcept {
    if (this != &other) {
        Unload(); // 既存のリソースを解放

        // other のリソースをこのオブジェクトに移動
        wfex = other.wfex;
        pBuffer = other.pBuffer;
        bufferSize = other.bufferSize;

        // other のリソースを無効化
        other.pBuffer = nullptr;
        other.bufferSize = 0;
        other.wfex = {};
    }
    return *this;
}

// WAVファイルをロードする
bool Sound::LoadWave(const std::string& filePath) {
    // 既存のデータを解放
    Unload();

    std::ifstream file;
    file.open(filePath, std::ios_base::binary);
    if (!file.is_open()) {
        assert(0 && "Failed to open WAV file!");
        return false;
    }

    RiffHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (strncmp(riff.chunk.id, "RIFF", 4) != 0 || strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0 && "Not a valid RIFF WAVE file!");
        file.close();
        return false;
    }

    FormatChunk format = {};
    file.read(reinterpret_cast<char*>(&format), sizeof(ChunkHeader));
    if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
        assert(0 && "Format chunk not found!");
        file.close();
        return false;
    }

    // fmt チャンクのサイズが WAVEFORMATEX 構造体のサイズ以下であることを確認
    // WAVEFORMATEX は固定サイズではないため、実際のフォーマットデータのサイズを読み込む
    // ここでは、format.chunk.size が WAVEFORMATEX のサイズを超えないことをアサート
    // 厳密には、WAVEFORMATEX の後に拡張データがある場合もあるが、ここでは簡易的にチェック
    assert(format.chunk.size <= sizeof(format.fmt));
    file.read(reinterpret_cast<char*>(&format.fmt), format.chunk.size);
    wfex = format.fmt;

    ChunkHeader data;
    file.read(reinterpret_cast<char*>(&data), sizeof(data));
    // "JUNK" チャンクをスキップ
    if (strncmp(data.id, "JUNK", 4) == 0) {
        file.seekg(data.size, std::ios_base::cur); // JUNK チャンクのサイズ分だけシーク
        file.read(reinterpret_cast<char*>(&data), sizeof(data)); // 次のチャンクを読み込む
    }

    if (strncmp(data.id, "data", 4) != 0) {
        assert(0 && "Data chunk not found!");
        file.close();
        return false;
    }

    bufferSize = data.size;
    pBuffer = new BYTE[bufferSize]; // 音声データ用のメモリを確保
    file.read(reinterpret_cast<char*>(pBuffer), bufferSize); // 音声データを読み込む

    file.close();
    return true;
}

// 音声データを解放する
void Sound::Unload() {
    if (pBuffer) {
        delete[] pBuffer; // 確保したメモリを解放
        pBuffer = nullptr;
    }
    bufferSize = 0;
    wfex = {}; // フォーマット情報もリセット
}

// WaveFormatEx のゲッター
const WAVEFORMATEX& Sound::GetWaveFormatEx() const {
    return wfex;
}

// バッファのゲッター
BYTE* Sound::GetBuffer() const {
    return pBuffer;
}

// バッファサイズのゲッター
unsigned int Sound::GetBufferSize() const {
    return bufferSize;
}