#pragma once
/**
 * LAUNCHER API CLIENT
 * Connects to the cheat server for auth, updates, and downloads
 */

#include <string>
#include <vector>
#include <optional>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#include <winhttp.h>
#include <intrin.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace api {

// ============================================
// Configuration
// ============================================

// Server URL - change this to your server
inline const wchar_t* SERVER_HOST = L"138.124.0.8";
inline const int SERVER_PORT = 80;
inline const bool USE_HTTPS = false;

// ============================================
// Data Structures
// ============================================

struct UserInfo {
    int id = 0;
    std::string username;
    bool is_admin = false;
};

struct GameInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string latest_version;
    bool has_license = false;
};

struct LoginResult {
    bool success = false;
    std::string error;
    std::string token;
    UserInfo user;
};

struct DownloadProgress {
    size_t downloaded = 0;
    size_t total = 0;
    float percent = 0.0f;
};

using ProgressCallback = std::function<void(const DownloadProgress&)>;

// ============================================
// API Client
// ============================================

class Client {
public:
    static Client& Get() {
        static Client instance;
        return instance;
    }

    // State
    bool IsLoggedIn() const { return !m_token.empty(); }
    const UserInfo& GetUser() const { return m_user; }
    const std::string& GetToken() const { return m_token; }
    
    // Save/Load session
    void SaveSession();
    bool LoadSession();
    void ClearSession();
    
    // Auth
    LoginResult Login(const std::string& username, const std::string& password);
    bool VerifyToken();
    void Logout();
    
    // Games
    std::vector<GameInfo> GetGames();
    bool HasLicense(const std::string& gameId);
    
    // Downloads
    bool DownloadCheat(const std::string& gameId, const std::string& savePath, 
                       ProgressCallback progress = nullptr);
    bool CheckUpdate(const std::string& gameId, const std::string& currentVersion);
    std::string GetLatestVersion(const std::string& gameId);
    
    // Offsets
    std::string GetOffsets(const std::string& gameId);
    
    // HWID
    static std::string GetHWID();

private:
    Client() = default;
    
    std::string m_token;
    UserInfo m_user;
    std::vector<GameInfo> m_cachedGames;
    
    // HTTP
    std::string HttpGet(const std::wstring& path);
    std::string HttpPost(const std::wstring& path, const std::string& body);
    bool HttpDownload(const std::wstring& path, const std::string& savePath, ProgressCallback progress);
    
    // JSON helpers
    static std::string ExtractString(const std::string& json, const std::string& key);
    static int ExtractInt(const std::string& json, const std::string& key);
    static bool ExtractBool(const std::string& json, const std::string& key);
};

} // namespace api

