#include "AudioManager.h" // AudioManager.h をインクルード
#include <cassert>        // アサートのために必要

// PlayingVoice 構造体のデストラクタの実装
AudioManager::PlayingVoice::~PlayingVoice() {
    if (pVoice) {
        pVoice->DestroyVoice(); // ボイスを破棄
        pVoice = nullptr;
    }
    if (pCallback) {
        delete pCallback; // コールバックオブジェクトを解放
        pCallback = nullptr;
    }
}

// PlayingVoice 構造体のムーブコンストラクタの実装
AudioManager::PlayingVoice::PlayingVoice(PlayingVoice&& other) noexcept
    : pVoice(other.pVoice), pCallback(other.pCallback), isFinished(other.isFinished.load()) {
    other.pVoice = nullptr;
    other.pCallback = nullptr;
    other.isFinished = false;
}

// PlayingVoice 構造体のムーブ代入演算子の実装
AudioManager::PlayingVoice& AudioManager::PlayingVoice::operator=(PlayingVoice&& other) noexcept {
    if (this != &other) {
        // 既存のリソースを解放
        if (pVoice) pVoice->DestroyVoice();
        if (pCallback) delete pCallback;

        // other のリソースをこのオブジェクトに移動
        pVoice = other.pVoice;
        pCallback = other.pCallback;
        isFinished = other.isFinished.load();

        // other のリソースを無効化
        other.pVoice = nullptr;
        other.pCallback = nullptr;
        other.isFinished = false;
    }
    return *this;
}

// AudioManager のコンストラクタの実装
AudioManager::AudioManager() : masterVoice(nullptr) {
    // ComPtr は自動的に nullptr で初期化される
}

// AudioManager のデストラクタの実装
AudioManager::~AudioManager() {
    Shutdown(); // シャットダウン処理を呼び出す
}

// XAudio2 エンジンを初期化する
bool AudioManager::Initialize() {
    HRESULT hr;

    // XAudio2 オブジェクトの生成
    hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return false;
    }

    // マスターボイスの生成
    hr = xAudio2->CreateMasteringVoice(&masterVoice);
    if (FAILED(hr)) {
        Shutdown(); // 失敗したらXAudio2も解放
        return false;
    }
    return true;
}

// XAudio2 エンジンをシャットダウンする
void AudioManager::Shutdown() {
    // ミューテックスでロック
    std::lock_guard<std::mutex> lock(playingVoicesMutex);

    // すべての再生中ボイスを停止・解放
    // playingVoices リストの各要素のデストラクタが pVoice と pCallback を解放します。
    // そのため、ここではリストをクリアするだけで十分
    playingVoices.clear();

    // マスターボイスの解放
    if (masterVoice) {
        masterVoice->DestroyVoice();
        masterVoice = nullptr;
    }
    // IXAudio2 オブジェクトの解放 (ComPtr が自動で行う)
    xAudio2.Reset();

    // ロードされたすべてのサウンドデータを解放
    for (auto& pair : sounds) {
        pair.second.Unload();
    }
    sounds.clear();
}

// サウンドをロードし、名前を付けて管理する
bool AudioManager::LoadSound(const std::string& name, const std::string& filePath) {
    // 既に同じ名前のサウンドがロードされているかチェック
    if (sounds.count(name)) {
        return false;
    }

    Sound newSound;
    if (newSound.LoadWave(filePath)) {
        // ムーブセマンティクスで追加
        sounds.emplace(name, std::move(newSound));
        return true;
    }
    return false;
}

// ロードしたサウンドを名前で取得する
Sound* AudioManager::GetSound(const std::string& name) {
    auto it = sounds.find(name);
    return (it != sounds.end()) ? &(it->second) : nullptr;
}

// 名前を指定してサウンドを再生する
void AudioManager::PlaySound(const std::string& name) {
    Sound* sound = GetSound(name);
    if (!sound) {
        return;
    }

    // ミューテックスでロック
    std::lock_guard<std::mutex> lock(playingVoicesMutex);

    IXAudio2SourceVoice* pSourceVoice = nullptr;

    // 新しいPlayingVoiceを作成
    playingVoices.emplace_back();
    PlayingVoice& newVoice = playingVoices.back();

    // コールバックを作成（PlayingVoiceの終了フラグを渡す）
    newVoice.pCallback = new InternalVoiceCallback(&newVoice.isFinished);

    // SourceVoice を作成
    HRESULT hr = xAudio2->CreateSourceVoice(&newVoice.pVoice, &sound->GetWaveFormatEx(),
        0, XAUDIO2_DEFAULT_FREQ_RATIO, newVoice.pCallback);
    if (FAILED(hr)) {
        playingVoices.pop_back(); // 失敗時は削除
        return;
    }

    // バッファを送信
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = sound->GetBuffer();
    buf.AudioBytes = sound->GetBufferSize();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    hr = newVoice.pVoice->SubmitSourceBuffer(&buf);
    if (FAILED(hr)) {
        playingVoices.pop_back();
        return;
    }

    hr = newVoice.pVoice->Start(0);
    if (FAILED(hr)) {
        playingVoices.pop_back();
        return;
    }
}

// 終了したボイスのクリーンアップ
void AudioManager::CleanupFinishedVoices() {
    std::lock_guard<std::mutex> lock(playingVoicesMutex);

    // 終了フラグが立っているボイスを削除
    auto it = std::remove_if(playingVoices.begin(), playingVoices.end(),
        [](const PlayingVoice& voice) {
            return voice.isFinished.load();
        });

    playingVoices.erase(it, playingVoices.end());
}