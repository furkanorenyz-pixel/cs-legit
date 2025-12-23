/**
 * LAUNCHER API CLIENT - Implementation
 */

#include "api.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace api {

// ============================================
// JSON Helpers (Simple)
// ============================================

std::string Client::ExtractString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    
    if (pos > 0 && json[pos-1] != '"') return "";
    
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    
    return json.substr(pos, end - pos);
}

int Client::ExtractInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0;
    
    pos += searchKey.length();
    while (pos < json.length() && json[pos] == ' ') pos++;
    
    std::string num;
    while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-')) {
        num += json[pos++];
    }
    
    return num.empty() ? 0 : std::stoi(num);
}

bool Client::ExtractBool(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;
    
    pos += searchKey.length();
    while (pos < json.length() && json[pos] == ' ') pos++;
    
    return json.substr(pos, 4) == "true";
}

// ============================================
// HWID
// ============================================

std::string Client::GetHWID() {
#ifdef _WIN32
    std::stringstream ss;
    
    // CPU ID
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    ss << std::hex << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    
    // Volume serial
    DWORD volumeSerial = 0;
    GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0);
    ss << volumeSerial;
    
    // Computer name
    char computerName[256] = {0};
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);
    
    std::string combined = ss.str() + computerName;
    size_t hash = std::hash<std::string>{}(combined);
    
    std::stringstream result;
    result << std::hex << std::uppercase << hash;
    return result.str();
#else
    return "NO-HWID";
#endif
}

// ============================================
// Session Management
// ============================================

void Client::SaveSession() {
    std::ofstream file("session.dat");
    if (file.is_open()) {
        file << m_token << "\n";
        file << m_user.id << "\n";
        file << m_user.username << "\n";
        file << (m_user.is_admin ? "1" : "0") << "\n";
        file.close();
    }
}

bool Client::LoadSession() {
    std::ifstream file("session.dat");
    if (!file.is_open()) return false;
    
    std::getline(file, m_token);
    
    std::string line;
    if (std::getline(file, line)) m_user.id = std::stoi(line);
    if (std::getline(file, line)) m_user.username = line;
    if (std::getline(file, line)) m_user.is_admin = (line == "1");
    
    file.close();
    
    // Verify token is still valid
    return !m_token.empty() && VerifyToken();
}

void Client::ClearSession() {
    m_token.clear();
    m_user = UserInfo();
    fs::remove("session.dat");
}

// ============================================
// HTTP Implementation
// ============================================

std::string Client::HttpGet(const std::wstring& path) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"CheatLauncher/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return "";
    
    HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST, SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    DWORD flags = USE_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Add auth header
    if (!m_token.empty()) {
        std::wstring authHeader = L"Authorization: Bearer " + 
            std::wstring(m_token.begin(), m_token.end());
        WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }
    
    // Add HWID header
    std::string hwid = GetHWID();
    std::wstring hwidHeader = L"X-HWID: " + std::wstring(hwid.begin(), hwid.end());
    WinHttpAddRequestHeaders(hRequest, hwidHeader.c_str(), (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);
    
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        
        DWORD size = 0;
        do {
            size = 0;
            WinHttpQueryDataAvailable(hRequest, &size);
            
            if (size > 0) {
                char* buffer = new char[size + 1];
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer, size, &downloaded);
                buffer[downloaded] = 0;
                result += buffer;
                delete[] buffer;
            }
        } while (size > 0);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}

std::string Client::HttpPost(const std::wstring& path, const std::string& body) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"CheatLauncher/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return "";
    
    HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST, SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    DWORD flags = USE_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Headers
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);
    
    if (!m_token.empty()) {
        std::wstring authHeader = L"Authorization: Bearer " + 
            std::wstring(m_token.begin(), m_token.end());
        WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }
    
    std::string hwid = GetHWID();
    std::wstring hwidHeader = L"X-HWID: " + std::wstring(hwid.begin(), hwid.end());
    WinHttpAddRequestHeaders(hRequest, hwidHeader.c_str(), (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);
    
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        
        DWORD size = 0;
        do {
            size = 0;
            WinHttpQueryDataAvailable(hRequest, &size);
            
            if (size > 0) {
                char* buffer = new char[size + 1];
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer, size, &downloaded);
                buffer[downloaded] = 0;
                result += buffer;
                delete[] buffer;
            }
        } while (size > 0);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}

bool Client::HttpDownload(const std::wstring& path, const std::string& savePath, ProgressCallback progress) {
    HINTERNET hSession = WinHttpOpen(L"CheatLauncher/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return false;
    
    HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST, SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    DWORD flags = USE_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Auth headers
    if (!m_token.empty()) {
        std::wstring authHeader = L"Authorization: Bearer " + 
            std::wstring(m_token.begin(), m_token.end());
        WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }
    
    std::string hwid = GetHWID();
    std::wstring hwidHeader = L"X-HWID: " + std::wstring(hwid.begin(), hwid.end());
    WinHttpAddRequestHeaders(hRequest, hwidHeader.c_str(), (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);
    
    bool success = false;
    
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        
        // Get content length
        DWORD contentLength = 0;
        DWORD headerSize = sizeof(contentLength);
        WinHttpQueryHeaders(hRequest, 
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &headerSize, WINHTTP_NO_HEADER_INDEX);
        
        std::ofstream file(savePath, std::ios::binary);
        if (!file.is_open()) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }
        
        DWORD totalDownloaded = 0;
        DWORD size = 0;
        
        do {
            size = 0;
            WinHttpQueryDataAvailable(hRequest, &size);
            
            if (size > 0) {
                char* buffer = new char[size];
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer, size, &downloaded);
                file.write(buffer, downloaded);
                delete[] buffer;
                
                totalDownloaded += downloaded;
                
                if (progress && contentLength > 0) {
                    DownloadProgress p;
                    p.downloaded = totalDownloaded;
                    p.total = contentLength;
                    p.percent = (float)totalDownloaded / contentLength * 100.0f;
                    progress(p);
                }
            }
        } while (size > 0);
        
        file.close();
        success = true;
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return success;
}

// ============================================
// Auth
// ============================================

LoginResult Client::Login(const std::string& username, const std::string& password) {
    std::string body = "{\"username\":\"" + username + "\",\"password\":\"" + password + 
                       "\",\"hwid\":\"" + GetHWID() + "\"}";
    
    std::string response = HttpPost(L"/api/auth/login", body);
    
    LoginResult result;
    result.success = false;
    
    if (response.empty()) {
        result.error = "Connection failed";
        return result;
    }
    
    if (response.find("\"token\"") != std::string::npos) {
        result.success = true;
        result.token = ExtractString(response, "token");
        result.user.id = ExtractInt(response, "id");
        result.user.username = ExtractString(response, "username");
        result.user.is_admin = ExtractBool(response, "is_admin");
        
        m_token = result.token;
        m_user = result.user;
        
        SaveSession();
    } else {
        result.error = ExtractString(response, "error");
        if (result.error.empty()) result.error = "Login failed";
    }
    
    return result;
}

LoginResult Client::Register(const std::string& username, const std::string& password,
                             const std::string& licenseKey) {
    std::string body = "{\"username\":\"" + username + "\",\"password\":\"" + password + 
                       "\",\"license_key\":\"" + licenseKey + "\"}";
    
    std::string response = HttpPost(L"/api/auth/register", body);
    
    LoginResult result;
    result.success = false;
    
    if (response.empty()) {
        result.error = "Connection failed";
        return result;
    }
    
    if (ExtractBool(response, "success")) {
        result.success = true;
        // Auto-login after registration
        return Login(username, password);
    } else {
        result.error = ExtractString(response, "error");
        if (result.error.empty()) result.error = "Registration failed";
    }
    
    return result;
}

bool Client::ActivateLicense(const std::string& licenseKey) {
    std::string body = "{\"license_key\":\"" + licenseKey + "\"}";
    std::string response = HttpPost(L"/api/auth/activate", body);
    return ExtractBool(response, "success");
}

bool Client::VerifyToken() {
    std::string body = "{\"token\":\"" + m_token + "\"}";
    std::string response = HttpPost(L"/api/auth/verify", body);
    return ExtractBool(response, "valid");
}

void Client::Logout() {
    ClearSession();
}

// ============================================
// Games
// ============================================

std::vector<GameInfo> Client::GetGames() {
    m_cachedGames.clear();
    
    std::string response = HttpGet(L"/api/games");
    
    if (response.empty()) return m_cachedGames;
    
    // Simple parsing for games array
    size_t pos = 0;
    while ((pos = response.find("{\"id\":", pos)) != std::string::npos) {
        size_t end = response.find("}", pos);
        if (end == std::string::npos) break;
        
        std::string gameJson = response.substr(pos, end - pos + 1);
        
        GameInfo game;
        game.id = ExtractString(gameJson, "id");
        game.name = ExtractString(gameJson, "name");
        game.description = ExtractString(gameJson, "description");
        game.latest_version = ExtractString(gameJson, "latest_version");
        game.has_license = ExtractInt(gameJson, "has_license") == 1 || 
                          ExtractBool(gameJson, "has_license");
        
        if (!game.id.empty()) {
            m_cachedGames.push_back(game);
        }
        
        pos = end + 1;
    }
    
    return m_cachedGames;
}

bool Client::HasLicense(const std::string& gameId) {
    for (const auto& game : m_cachedGames) {
        if (game.id == gameId) return game.has_license;
    }
    return false;
}

// ============================================
// Downloads
// ============================================

bool Client::DownloadCheat(const std::string& gameId, const std::string& savePath, 
                           ProgressCallback progress) {
    std::wstring path = L"/api/download/" + std::wstring(gameId.begin(), gameId.end()) + L"/latest";
    return HttpDownload(path, savePath, progress);
}

bool Client::CheckUpdate(const std::string& gameId, const std::string& currentVersion) {
    std::wstring path = L"/api/games/" + std::wstring(gameId.begin(), gameId.end()) + 
                        L"/status?current_version=" + std::wstring(currentVersion.begin(), currentVersion.end());
    std::string response = HttpGet(path);
    return ExtractBool(response, "needs_update");
}

std::string Client::GetLatestVersion(const std::string& gameId) {
    for (const auto& game : m_cachedGames) {
        if (game.id == gameId) return game.latest_version;
    }
    return "";
}

// ============================================
// Offsets
// ============================================

std::string Client::GetOffsets(const std::string& gameId) {
    std::wstring path = L"/api/offsets/" + std::wstring(gameId.begin(), gameId.end());
    return HttpGet(path);
}

} // namespace api

