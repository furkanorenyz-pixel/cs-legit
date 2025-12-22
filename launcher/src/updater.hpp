/**
 * UPDATER - Server-based update system
 * Загрузка и обновление читов с сервера
 */

#pragma once

#include <Windows.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <json/json.h>  // Используем простой JSON парсер или nlohmann/json

#pragma comment(lib, "wininet.lib")

namespace updater {

// ============================================
// Config
// ============================================

struct UpdateInfo {
    std::string game;
    std::string type;  // "external" or "internal"
    std::string currentVersion;
    std::string latestVersion;
    std::string downloadUrl;
    bool updateAvailable;
    std::string description;
};

struct FileInfo {
    std::string filename;
    size_t size;
    std::string hash;
    std::string url;
};

// ============================================
// HTTP Client
// ============================================

class HttpClient {
public:
    static std::string Get(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("CheatLauncher/1.0", 
            INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
        
        if (!hInternet) return "";
        
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), 
            nullptr, 0, INTERNET_FLAG_RELOAD, 0);
        
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }
        
        std::string result;
        char buffer[4096];
        DWORD bytesRead;
        
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.append(buffer, bytesRead);
        }
        
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return result;
    }
    
    static bool DownloadFile(const std::string& url, const std::string& filepath) {
        HINTERNET hInternet = InternetOpenA("CheatLauncher/1.0", 
            INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
        
        if (!hInternet) return false;
        
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), 
            nullptr, 0, INTERNET_FLAG_RELOAD, 0);
        
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return false;
        }
        
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
        
        char buffer[8192];
        DWORD bytesRead;
        
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            file.write(buffer, bytesRead);
        }
        
        file.close();
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return true;
    }
};

// ============================================
// Update Checker
// ============================================

class UpdateChecker {
private:
    std::string m_serverUrl;
    
public:
    UpdateChecker(const std::string& serverUrl) : m_serverUrl(serverUrl) {
        // Убрать trailing slash
        if (!m_serverUrl.empty() && m_serverUrl.back() == '/') {
            m_serverUrl.pop_back();
        }
    }
    
    UpdateInfo CheckUpdate(const std::string& game, const std::string& type, 
                          const std::string& currentVersion = "") {
        UpdateInfo info = {};
        info.game = game;
        info.type = type;
        info.currentVersion = currentVersion;
        info.updateAvailable = false;
        
        std::string url = m_serverUrl + "/api/check-update?game=" + game + 
                         "&type=" + type;
        if (!currentVersion.empty()) {
            url += "&currentVersion=" + currentVersion;
        }
        
        std::string response = HttpClient::Get(url);
        if (response.empty()) {
            return info;
        }
        
        // Парсинг JSON (упрощенный, лучше использовать библиотеку)
        // Ищем updateAvailable
        if (response.find("\"updateAvailable\":true") != std::string::npos) {
            info.updateAvailable = true;
            
            // Извлекаем latestVersion
            size_t pos = response.find("\"latestVersion\":\"");
            if (pos != std::string::npos) {
                pos += 17;
                size_t end = response.find("\"", pos);
                if (end != std::string::npos) {
                    info.latestVersion = response.substr(pos, end - pos);
                }
            }
            
            // Извлекаем downloadUrl
            pos = response.find("\"downloadUrl\":\"");
            if (pos != std::string::npos) {
                pos += 15;
                size_t end = response.find("\"", pos);
                if (end != std::string::npos) {
                    std::string relativeUrl = response.substr(pos, end - pos);
                    info.downloadUrl = m_serverUrl + relativeUrl;
                }
            }
        }
        
        return info;
    }
    
    bool DownloadUpdate(const UpdateInfo& info, const std::string& downloadDir) {
        if (!info.updateAvailable || info.downloadUrl.empty()) {
            return false;
        }
        
        // Получить список файлов
        std::string filesUrl = m_serverUrl + "/api/games/" + info.game + "/" + 
                               info.type + "/" + info.latestVersion + "/files";
        
        std::string filesResponse = HttpClient::Get(filesUrl);
        if (filesResponse.empty()) {
            return false;
        }
        
        // Создать директорию для версии
        std::string versionDir = downloadDir + "\\" + info.latestVersion;
        CreateDirectoryA(versionDir.c_str(), nullptr);
        
        // Парсим список файлов и загружаем
        // Упрощенный парсинг - в реальности используйте JSON библиотеку
        size_t pos = 0;
        while ((pos = filesResponse.find("\"filename\":\"", pos)) != std::string::npos) {
            pos += 12;
            size_t end = filesResponse.find("\"", pos);
            if (end == std::string::npos) break;
            
            std::string filename = filesResponse.substr(pos, end - pos);
            std::string fileUrl = info.downloadUrl + filename;
            std::string filepath = versionDir + "\\" + filename;
            
            if (HttpClient::DownloadFile(fileUrl, filepath)) {
                // Файл загружен
            }
            
            pos = end;
        }
        
        return true;
    }
};

} // namespace updater

