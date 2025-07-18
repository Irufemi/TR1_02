#include "MapManager.h"

#include <Novice.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

MapManager::MapManager(const std::string& spreadsheetId,
    const std::string& sheetRange,
    const std::string& apiKey,
    const std::string& cacheFile,
    int tileSize,
    int yOffset)
    : spreadsheetId_(spreadsheetId)
    , sheetRange_(sheetRange)
    , apiKey_(apiKey)
    , cacheFile_(cacheFile)
    , tileSize_(tileSize)
    , yOffset_(yOffset) {
}

MapManager::~MapManager() {
    curl_global_cleanup();
}

void MapManager::Initialize() {
    curl_global_init(CURL_GLOBAL_ALL);
    isOnline_ = CheckOnlineStatus();
    if (isOnline_) {
        mapData_ = LoadFromSheet();
        SaveToJson(mapData_);
    } else {
        mapData_ = LoadFromJson();
    }
}

void MapManager::Update(const char keys[256], const char preKeys[256]) {
    // Oキーでオンライン切替
    if (keys[DIK_O] && !preKeys[DIK_O]) {
        isOnline_ = CheckOnlineStatus();
    }
    // Uキーで更新
    if (keys[DIK_U] && !preKeys[DIK_U]) {
        if (isOnline_) {
            mapData_ = LoadFromSheet();
            SaveToJson(mapData_);
        } else {
            mapData_ = LoadFromJson();
        }
    }
}

void MapManager::Draw() const {
    // オンライン状態表示
    Novice::ScreenPrintf(10, 10, isOnline_ ? "Online" : "Offline");
    // マップ描画
    for (int y = 0; y < static_cast<int>(mapData_.size()); ++y) {
        for (int x = 0; x < static_cast<int>(mapData_[y].size()); ++x) {
            int tile = mapData_[y][x];
            switch (tile) {
            case 1:
                Novice::DrawBox(x * tileSize_, 30 + y * tileSize_, tileSize_, tileSize_, 0, BLUE, kFillModeSolid);
                break;
            case 2:
                Novice::DrawBox(x * tileSize_, 30 + y * tileSize_, tileSize_, tileSize_, 0, RED, kFillModeSolid);
                break;
            default:
                break;
            }
        }
    }
}

size_t MapManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto realSize = size * nmemb;
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

std::vector<std::vector<int>> MapManager::LoadFromSheet() const {
    std::vector<std::vector<int>> data;
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    if (curl) {
        std::string url = "https://sheets.googleapis.com/v4/spreadsheets/" + spreadsheetId_
            + "/values/" + sheetRange_ + "?key=" + apiKey_;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) {
            try {
                auto j = json::parse(readBuffer);
                for (auto& row : j["values"]) {
                    std::vector<int> rowData;
                    for (auto& cell : row) {
                        rowData.push_back(std::stoi(cell.get<std::string>()));
                    }
                    data.push_back(rowData);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
    }
    return data;
}

std::vector<std::vector<int>> MapManager::LoadFromJson() const {
    std::vector<std::vector<int>> data;
    if (!std::filesystem::exists(cacheFile_)) return data;
    std::ifstream ifs(cacheFile_);
    if (ifs) {
        json j;
        ifs >> j;
        if (j.contains("values")) {
            data = j["values"].get<std::vector<std::vector<int>>>();
        }
    }
    return data;
}

void MapManager::SaveToJson(const std::vector<std::vector<int>>& data) const {
    json j;
    j["values"] = data;
    std::ofstream ofs(cacheFile_);
    ofs << j.dump();
}
