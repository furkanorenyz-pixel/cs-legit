#pragma once

/**
 * Cheat Launcher - API Client
 * Communicates with the backend server
 */

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace launcher {

// ============================================
// Data Structures
// ============================================

struct UserInfo {
    int id;
    std::string username;
    bool is_admin;
};

struct GameInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string latest_version;
    bool has_license;
    std::string license_expires;
};

struct VersionInfo {
    std::string version;
    std::string changelog;
    std::string created_at;
};

struct LoginResult {
    bool success;
    std::string error;
    std::string token;
    UserInfo user;
};

struct DownloadProgress {
    size_t downloaded;
    size_t total;
    float percent;
};

using ProgressCallback = std::function<void(const DownloadProgress&)>;

// ============================================
// API Client Class
// ============================================

class ApiClient {
public:
    static ApiClient& Get() {
        static ApiClient instance;
        return instance;
    }

    // Configuration
    void SetServerUrl(const std::string& url) { m_serverUrl = url; }
    void SetToken(const std::string& token) { m_token = token; }
    std::string GetToken() const { return m_token; }
    bool IsLoggedIn() const { return !m_token.empty(); }
    
    // Auth
    LoginResult Login(const std::string& username, const std::string& password);
    LoginResult Register(const std::string& username, const std::string& password, const std::string& licenseKey);
    bool VerifyToken();
    void Logout() { m_token.clear(); }
    
    // Games
    std::vector<GameInfo> GetGames();
    std::optional<GameInfo> GetGame(const std::string& gameId);
    std::vector<VersionInfo> GetVersions(const std::string& gameId);
    bool CheckUpdate(const std::string& gameId, const std::string& currentVersion);
    
    // Download
    bool DownloadCheat(const std::string& gameId, const std::string& version, 
                       const std::string& savePath, ProgressCallback progress = nullptr);
    bool DownloadLatest(const std::string& gameId, const std::string& savePath,
                        ProgressCallback progress = nullptr);
    bool DownloadFile(const std::string& gameId, const std::string& fileType,
                      const std::string& savePath, std::function<void(size_t, size_t)> progress = nullptr);
    
    // Offsets
    std::string GetOffsets(const std::string& gameId);
    std::string GetOffsetsHash(const std::string& gameId);
    
    // HWID
    static std::string GetHWID();

private:
    ApiClient() = default;
    
    std::string m_serverUrl = "http://localhost:3000";
    std::string m_token;
    
    // HTTP helpers (implemented with WinHTTP or libcurl)
    std::string HttpGet(const std::string& endpoint);
    std::string HttpPost(const std::string& endpoint, const std::string& body);
    bool HttpDownload(const std::string& endpoint, const std::string& savePath, ProgressCallback progress);
};

} // namespace launcher

