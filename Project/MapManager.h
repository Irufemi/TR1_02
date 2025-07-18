#pragma once
#include <vector>
#include <string>

class MapManager{
public:
    // コンストラクタ: シートID、範囲、APIキー、キャッシュファイル名、タイルサイズ、Yオフセット
    MapManager(const std::string& spreadsheetId,
        const std::string& sheetRange,
        const std::string& apiKey,
        const std::string& cacheFile,
        int tileSize,
        int yOffset);
    ~MapManager();

    // 初期化（オンラインチェック＋データ取得）
    void Initialize();
    // 入力処理 (Oキー:オンライン切替, Uキー:更新)
    void Update(const char keys[256], const char preKeys[256]);
    // 描画
    void Draw() const;

private:
    // 内部ロード/保存
    std::vector<std::vector<int>> LoadFromSheet() const;
    std::vector<std::vector<int>> LoadFromJson() const;
    void SaveToJson(const std::vector<std::vector<int>>& data) const;

    // オンライン状態チェック
    bool CheckOnlineStatus() const;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // メンバ変数
    std::string spreadsheetId_;
    std::string sheetRange_;
    std::string apiKey_;
    std::string cacheFile_;
    std::vector<std::vector<int>> mapData_;
    bool isOnline_ = false;
    int tileSize_;
    int yOffset_;

};

