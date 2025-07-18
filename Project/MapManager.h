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
        // 上位32bit に first、下位32bit に second を配置
        return (static_cast<size_t>(p.first) << 32) ^ static_cast<unsigned int>(p.second);
    }
};

// マップチャンクを表す構造体（非同期読み込み用）
struct MapChunk {
    int chunkX = 0;
    int chunkY = 0;
    TileData tiles;
    bool loaded = false;
    std::future<TileData> loaderFuture;  // 非同期読み込み結果
};

class MapManager {
public:
    // コンストラクタ: スプレッドシートID、シート名、APIキー、タイルサイズ、Yオフセット、ビュー距離
    MapManager(const std::string& spreadsheetId,
        const std::string& sheetName,
        const std::string& apiKey,
        int tileSize,
        int yOffset,
        int viewDistanceChunks = 1);
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

    // シートから範囲読み込み（チャンク単位）
    TileData LoadFromSheet(int cx, int cy) const;
    // キャッシュ読み書き
    TileData LoadChunkCache(int cx, int cy) const;
    void SaveChunkCache(int cx, int cy, const TileData& data) const;

    // 非同期読み込み管理
    void PollLoadedChunks();
    void EnqueueChunkLoad(int cx, int cy);

    // 列番号から Google スプレッドシート形式の列文字列 (A, B, ..., Z, AA, AB, ...)
    static std::string ColIndexToName(int index);

    // メンバ変数
    std::string spreadsheetId_;
    std::string sheetName_;
    std::string apiKey_;
    bool isOnline_ = false;
    int tileSize_;
    int yOffset_;
    int viewDistanceChunks_;
    // ハッシュ関数 PairHash を使用するように変更
    std::unordered_map<std::pair<int, int>, MapChunk, PairHash> chunks_;

    static constexpr int kChunkWidth = 16;
    static constexpr int kChunkHeight = 16;
};
