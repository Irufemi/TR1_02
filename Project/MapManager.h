#pragma once

#include <Novice.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <future>
#include <chrono>

using json = nlohmann::json;
using TileData = std::vector<std::vector<int>>;

// ペア<int,int> 用のハッシュ関数
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const noexcept {
        return (static_cast<size_t>(p.first) << 32) ^ static_cast<unsigned int>(p.second);
    }
};

// マップチャンクを表す構造体（非同期読み込み用）
struct MapChunk {
    int chunkX = 0;
    int chunkY = 0;
    TileData tiles;
    bool loaded = false;
    std::future<TileData> loaderFuture;
};

class MapManager {
public:
    // コンストラクタ: スプレッドシートID、シート名、APIキー、タイルサイズ、Yオフセット、ビュー距離、キャッシュディレクトリ
    MapManager(const std::string& spreadsheetId,
        const std::string& sheetName,
        const std::string& apiKey,
        int tileSize,
        int yOffset,
        int viewDistanceChunks = 1,
        const std::string& cacheDir = "cache");
    ~MapManager();

    // 初期化（オンラインチェック＋初期チャンク読み込み開始）
    void Initialize(int startPlayerTileX, int startPlayerTileY);
    // 入力処理 (Oキー:オンライン切替, Uキー:チャンク再読み込み)
    void Update(const char keys[256], const char preKeys[256], int playerTileX, int playerTileY);
    // 描画
    void Draw() const;

private:
    // ネットワーク
    bool CheckOnlineStatus() const;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // シート読み込み／キャッシュI/O
    TileData LoadFromSheet(int cx, int cy) const;
    TileData LoadChunkCache(int cx, int cy) const;
    void SaveChunkCache(int cx, int cy, const TileData& data) const;

    // 非同期読み込み管理
    void PollLoadedChunks();
    void EnqueueChunkLoad(int cx, int cy);

    // 列番号からGoogleシート列文字列
    static std::string ColIndexToName(int index);

    // メンバ変数
    std::string spreadsheetId_;
    std::string sheetName_;
    std::string apiKey_;
    bool isOnline_ = false;
    int tileSize_;
    int yOffset_;
    int viewDistanceChunks_;
    std::string cacheDir_;
    std::unordered_map<std::pair<int, int>, MapChunk, PairHash> chunks_;

    static constexpr int kChunkWidth = 16;
    static constexpr int kChunkHeight = 16;
};
