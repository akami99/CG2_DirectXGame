#include "AudioManager.h" // AudioManager.h をインクルード
#include <cassert>        // アサートのために必要

// PlayingVoice 構造体のデストラクタの実装
AudioManager::PlayingVoice::~PlayingVoice() {
    if (pVoice) {
        pVoice->DestroyVoice(); // ボイスを破棄
    }
    if (pCallback) {
        delete pCallback; // コールバックオブジェクトを解放
    }
}

// PlayingVoice 構造体のムーブコンストラクタの実装
AudioManager::PlayingVoice::PlayingVoice(PlayingVoice&& other) noexcept
    : pVoice(other.pVoice), pCallback(other.pCallback) {
    other.pVoice = nullptr;
    other.pCallback = nullptr;
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

        // other のリソースを無効化
        other.pVoice = nullptr;
        other.pCallback = nullptr;
    }
    return *this;
}

// InternalVoiceCallback クラスのコンストラクタの実装
AudioManager::InternalVoiceCallback::InternalVoiceCallback(AudioManager* manager, std::list<PlayingVoice>::iterator it)
    : audioManager(manager), voiceIterator(it) {}

// InternalVoiceCallback クラスの OnBufferEnd メソッドの実装
void STDMETHODCALLTYPE AudioManager::InternalVoiceCallback::OnBufferEnd(void* pBufferContext) {
    // AudioManagerのクリーンアップ関数を呼び出す
    audioManager->NotifyVoiceEnd(voiceIterator);
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
        assert(0 && "Failed to create XAudio2 object!");
        return false;
    }

    // マスターボイスの生成
    hr = xAudio2->CreateMasteringVoice(&masterVoice);
    if (FAILED(hr)) {
        assert(0 && "Failed to create mastering voice!");
        Shutdown(); // 失敗したらXAudio2も解放
        return false;
    }
    return true;
}

// XAudio2 エンジンをシャットダウンする
void AudioManager::Shutdown() {
    // すべての再生中ボイスを停止・解放
    // playingVoices リストの各要素のデストラクタが pVoice と pCallback を解放します。
    // そのため、ここではリストをクリアするだけで十分です。
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
        assert(0 && "Sound with this name already exists!");
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
    if (it != sounds.end()) {
        return &(it->second);
    }
    return nullptr; // 見つからなかった場合
}

// 名前を指定してサウンドを再生する
void AudioManager::PlaySound(const std::string& name) {
    Sound* sound = GetSound(name);
    if (!sound) {
        assert(0 && "Sound not found for playback!");
        return;
    }

    IXAudio2SourceVoice* pSourceVoice = nullptr;

    // コールバックインスタンスの作成と PlayingVoice のリストへの追加
    // まずリストに空の PlayingVoice 要素を追加し、そのイテレータを取得
    playingVoices.emplace_back();
    auto it = std::prev(playingVoices.end());

    // InternalVoiceCallback オブジェクトを動的に生成
    InternalVoiceCallback* callback = new InternalVoiceCallback(this, it);

    // SourceVoice を作成し、コールバックを設定
    HRESULT hr = xAudio2->CreateSourceVoice(&pSourceVoice, &sound->GetWaveFormatEx(), 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback);
    if (FAILED(hr)) {
        assert(0 && "Failed to create source voice!");
        // エラー時はリストに追加した PlayingVoice を削除。デストラクタが pVoice と callback を解放します。
        playingVoices.erase(it);
        return;
    }

    // PlayingVoice 構造体にボイスとコールバックを設定
    it->pVoice = pSourceVoice;
    it->pCallback = callback;

    XAUDIO2_BUFFER buf{};
    buf.pAudioData = sound->GetBuffer();
    buf.AudioBytes = sound->GetBufferSize();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    hr = pSourceVoice->SubmitSourceBuffer(&buf);
    if (FAILED(hr)) {
        assert(0 && "Failed to submit source buffer!");
        playingVoices.erase(it); // エラー時はリストから削除
        return;
    }

    hr = pSourceVoice->Start(0);
    if (FAILED(hr)) {
        assert(0 && "Failed to start source voice!");
        playingVoices.erase(it); // エラー時はリストから削除
        return;
    }
}

// コールバックから呼ばれる、ボイスをリストから削除するメソッド
void AudioManager::NotifyVoiceEnd(std::list<PlayingVoice>::iterator voiceIterator) {
    // voiceIterator が指す PlayingVoice オブジェクトをリストから削除します。
    // PlayingVoice のデストラクタが、関連付けられた IXAudio2SourceVoice と InternalVoiceCallback を解放します。
    playingVoices.erase(voiceIterator);
}