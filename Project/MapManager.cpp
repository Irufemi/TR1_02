#include "MapManager.h"
#include <fstream>
#include <iostream>

MapManager::MapManager(const std::string& spreadsheetId,
    const std::string& sheetName,
    const std::string& apiKey,
    int tileSize,
    int yOffset,
    int viewDistanceChunks,
    const std::string& cacheDir)
    : spreadsheetId_(spreadsheetId)
    , sheetName_(sheetName)
    , apiKey_(apiKey)
    , tileSize_(tileSize)
    , yOffset_(yOffset)
    , viewDistanceChunks_(viewDistanceChunks)
    , cacheDir_(cacheDir) {
    // キャッシュディレクトリを作成
    std::filesystem::create_directories(cacheDir_);
}

MapManager::~MapManager() {
    curl_global_cleanup();
}

void MapManager::Initialize(int startPlayerTileX, int startPlayerTileY) {
    curl_global_init(CURL_GLOBAL_ALL);
    isOnline_ = CheckOnlineStatus();
    int cx = startPlayerTileX / kChunkWidth;
    int cy = startPlayerTileY / kChunkHeight;
    for (int dy = -viewDistanceChunks_; dy <= viewDistanceChunks_; ++dy) {
        for (int dx = -viewDistanceChunks_; dx <= viewDistanceChunks_; ++dx) {
            EnqueueChunkLoad(cx + dx, cy + dy);
        }
    }
}

void MapManager::Update(const char keys[256], const char preKeys[256], int playerTileX, int playerTileY) {
    if (keys[DIK_O] && !preKeys[DIK_O]) {
        isOnline_ = CheckOnlineStatus();
    }
    if (keys[DIK_U] && !preKeys[DIK_U]) {
        chunks_.clear();
    }
    int cx = playerTileX / kChunkWidth;
    int cy = playerTileY / kChunkHeight;
    for (int dy = -viewDistanceChunks_; dy <= viewDistanceChunks_; ++dy) {
        for (int dx = -viewDistanceChunks_; dx <= viewDistanceChunks_; ++dx) {
            EnqueueChunkLoad(cx + dx, cy + dy);
        }
    }
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        int dx = it->first.first - cx;
        int dy = it->first.second - cy;
        if (abs(dx) > viewDistanceChunks_ || abs(dy) > viewDistanceChunks_) {
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
    PollLoadedChunks();
}

void MapManager::Draw(int offsetX, int offsetY) const {
    Novice::ScreenPrintf(10, 10, isOnline_ ? "Online" : "Offline");
    for (const auto& kv : chunks_) {
        const auto& chunk = kv.second;
        if (!chunk.loaded) continue;
        for (int y = 0; y < (int)chunk.tiles.size(); ++y) {
            for (int x = 0; x < (int)chunk.tiles[y].size(); ++x) {
                int tile = chunk.tiles[y][x];
                int mapTileX = chunk.chunkX * kChunkWidth + x;
                int mapTileY = chunk.chunkY * kChunkHeight + y;
                int screenX = mapTileX * tileSize_ - offsetX;
                int screenY = mapTileY * tileSize_ - offsetY + yOffset_;
                if (tile == 1) Novice::DrawBox(screenX, screenY, tileSize_, tileSize_, 0, BLUE, kFillModeSolid);
                if (tile == 2) Novice::DrawBox(screenX, screenY, tileSize_, tileSize_, 0, RED, kFillModeSolid);
                if (tile == 3) Novice::DrawBox(screenX, screenY, tileSize_, tileSize_, 0, GREEN, kFillModeSolid);
            }
        }
    }
}

size_t MapManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), realSize);
    return realSize;
}

bool MapManager::CheckOnlineStatus() const {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

std::string MapManager::ColIndexToName(int index) {
    std::string name;
    while (index >= 0) {
        int rem = index % 26;
        name.insert(name.begin(), static_cast<char>('A' + rem));
        index = index / 26 - 1;
    }
    return name;
}

TileData MapManager::LoadFromSheet(int cx, int cy) const {
    std::vector<std::vector<int>> data;
    CURL* curl = curl_easy_init();
    std::string buffer;
    if (curl) {
        std::string startC = ColIndexToName(cx * kChunkWidth);
        std::string endC = ColIndexToName(cx * kChunkWidth + (kChunkWidth - 1));
        int startR = cy * kChunkHeight + 1;
        int endR = startR + (kChunkHeight - 1);
        std::string range = sheetName_ + "!" + startC + std::to_string(startR)
            + ":" + endC + std::to_string(endR);
        std::string url = "https://sheets.googleapis.com/v4/spreadsheets/" + spreadsheetId_
            + "/values/" + range + "?key=" + apiKey_;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        if (curl_easy_perform(curl) == CURLE_OK) {
            auto j = json::parse(buffer);
            for (auto& row : j["values"]) {
                std::vector<int> rowData;
                for (auto& cell : row) {
                    rowData.push_back(std::stoi(cell.get<std::string>()));
                }
                data.push_back(rowData);
            }
        }
        curl_easy_cleanup(curl);
    }
    return data;
}

TileData MapManager::LoadChunkCache(int cx, int cy) const {
    TileData data;
    std::filesystem::path dir(cacheDir_);
    std::filesystem::path path = dir / ("chunk_" + std::to_string(cx) + "_" + std::to_string(cy) + ".json");
    if (!std::filesystem::exists(path)) return data;
    std::ifstream ifs(path);
    json j; ifs >> j;
    data = j.get<TileData>();
    return data;
}

void MapManager::SaveChunkCache(int cx, int cy, const TileData& data) const {
    std::filesystem::path dir(cacheDir_);
    std::filesystem::path path = dir / ("chunk_" + std::to_string(cx) + "_" + std::to_string(cy) + ".json");
    std::ofstream ofs(path);
    ofs << json(data).dump();
}

void MapManager::EnqueueChunkLoad(int cx, int cy) {
    auto key = std::make_pair(cx, cy);
    auto& chunk = chunks_[key];
    if (chunk.loaded || chunk.loaderFuture.valid()) return;
    chunk.chunkX = cx;
    chunk.chunkY = cy;
    if (isOnline_) {
        chunk.loaderFuture = std::async(std::launch::async, [this, cx, cy]() {
            return LoadFromSheet(cx, cy);
            });
    } else {
        chunk.loaderFuture = std::async(std::launch::async, [this, cx, cy]() {
            return LoadChunkCache(cx, cy);
            });
    }
}

void MapManager::PollLoadedChunks() {
    for (auto& kv : chunks_) {
        auto& chunk = kv.second;
        if (chunk.loaded || !chunk.loaderFuture.valid()) continue;
        if (chunk.loaderFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            TileData data = chunk.loaderFuture.get();
            SaveChunkCache(chunk.chunkX, chunk.chunkY, data);
            chunk.tiles = std::move(data);
            chunk.loaded = true;
        }
    }
}
