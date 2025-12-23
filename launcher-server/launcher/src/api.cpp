/**
 * Cheat Launcher - API Client Implementation
 * Uses WinHTTP on Windows
 */

#include "../include/api.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <winhttp.h>
#include <intrin.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <sstream>
#include <fstream>
#include <algorithm>

namespace launcher {

// ============================================
// JSON Helpers (Simple implementation)
// For production, use nlohmann/json or rapidjson
// ============================================

static std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    
    if (json[pos-1] != '"') return "";
    
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    
    return json.substr(pos, end - pos);
}

static int ExtractJsonInt(const std::string& json, const std::string& key) {
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

static bool ExtractJsonBool(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;
    
    pos += searchKey.length();
    while (pos < json.length() && json[pos] == ' ') pos++;
    
    return json.substr(pos, 4) == "true";
}

// ============================================
// HWID Generation
// ============================================

std::string ApiClient::GetHWID() {
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
    
    // Simple hash of all values
    std::string combined = ss.str() + computerName;
    size_t hash = std::hash<std::string>{}(combined);
    
    std::stringstream result;
    result << std::hex << std::uppercase << hash;
    return result.str();
#else
    return "LINUX-HWID-NOT-IMPL";
#endif
}

// ============================================
// HTTP Implementation (WinHTTP)
// ============================================

#ifdef _WIN32

std::string ApiClient::HttpGet(const std::string& endpoint) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"CheatLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return "";
    
    // Parse URL
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 1024;
    
    std::wstring fullUrl(m_serverUrl.begin(), m_serverUrl.end());
    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    fullUrl += wEndpoint;
    
    WinHttpCrackUrl(fullUrl.c_str(), (DWORD)fullUrl.length(), 0, &urlComp);
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Add auth header if logged in
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

std::string ApiClient::HttpPost(const std::string& endpoint, const std::string& body) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"CheatLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return "";
    
    // Parse URL
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 1024;
    
    std::wstring fullUrl(m_serverUrl.begin(), m_serverUrl.end());
    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    fullUrl += wEndpoint;
    
    WinHttpCrackUrl(fullUrl.c_str(), (DWORD)fullUrl.length(), 0, &urlComp);
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
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

bool ApiClient::HttpDownload(const std::string& endpoint, const std::string& savePath,
                             ProgressCallback progress) {
    // Similar to HttpGet but saves to file
    // Implementation omitted for brevity - same pattern as HttpGet
    // but writes chunks to file and calls progress callback
    
    std::string data = HttpGet(endpoint);
    if (data.empty()) return false;
    
    std::ofstream file(savePath, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(data.c_str(), data.size());
    file.close();
    
    if (progress) {
        progress({data.size(), data.size(), 100.0f});
    }
    
    return true;
}

#else
// Linux implementation with libcurl would go here
std::string ApiClient::HttpGet(const std::string& endpoint) { return ""; }
std::string ApiClient::HttpPost(const std::string& endpoint, const std::string& body) { return ""; }
bool ApiClient::HttpDownload(const std::string& endpoint, const std::string& savePath,
                             ProgressCallback progress) { return false; }
#endif

// ============================================
// API Methods
// ============================================

LoginResult ApiClient::Login(const std::string& username, const std::string& password) {
    std::string body = "{\"username\":\"" + username + "\",\"password\":\"" + password + 
                       "\",\"hwid\":\"" + GetHWID() + "\"}";
    
    std::string response = HttpPost("/api/auth/login", body);
    
    LoginResult result;
    result.success = false;
    
    if (response.empty()) {
        result.error = "Connection failed";
        return result;
    }
    
    if (response.find("\"token\"") != std::string::npos) {
        result.success = true;
        result.token = ExtractJsonString(response, "token");
        result.user.id = ExtractJsonInt(response, "id");
        result.user.username = ExtractJsonString(response, "username");
        result.user.is_admin = ExtractJsonBool(response, "is_admin");
        
        m_token = result.token;
    } else {
        result.error = ExtractJsonString(response, "error");
        if (result.error.empty()) result.error = "Login failed";
    }
    
    return result;
}

LoginResult ApiClient::Register(const std::string& username, const std::string& password,
                                const std::string& licenseKey) {
    std::string body = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"";
    if (!licenseKey.empty()) {
        body += ",\"license_key\":\"" + licenseKey + "\"";
    }
    body += ",\"hwid\":\"" + GetHWID() + "\"}";
    
    std::string response = HttpPost("/api/auth/register", body);
    
    LoginResult result;
    result.success = false;
    
    if (response.empty()) {
        result.error = "Connection failed";
        return result;
    }
    
    if (response.find("\"token\"") != std::string::npos) {
        result.success = true;
        result.token = ExtractJsonString(response, "token");
        result.user.id = ExtractJsonInt(response, "id");
        result.user.username = ExtractJsonString(response, "username");
        result.user.is_admin = ExtractJsonBool(response, "is_admin");
        
        m_token = result.token;
    } else {
        result.error = ExtractJsonString(response, "error");
        if (result.error.empty()) result.error = "Registration failed";
    }
    
    return result;
}

bool ApiClient::VerifyToken() {
    std::string body = "{\"token\":\"" + m_token + "\"}";
    std::string response = HttpPost("/api/auth/verify", body);
    return ExtractJsonBool(response, "valid");
}

std::vector<GameInfo> ApiClient::GetGames() {
    std::vector<GameInfo> games;
    
    std::string response = HttpGet("/api/games");
    
    // Simple parsing - for production use proper JSON library
    // This is a simplified example
    
    return games;
}

std::optional<GameInfo> ApiClient::GetGame(const std::string& gameId) {
    std::string response = HttpGet("/api/games/" + gameId);
    
    if (response.empty() || response.find("\"error\"") != std::string::npos) {
        return std::nullopt;
    }
    
    GameInfo game;
    game.id = ExtractJsonString(response, "id");
    game.name = ExtractJsonString(response, "name");
    game.description = ExtractJsonString(response, "description");
    game.latest_version = ExtractJsonString(response, "latest_version");
    game.has_license = ExtractJsonBool(response, "has_license");
    
    return game;
}

std::vector<VersionInfo> ApiClient::GetVersions(const std::string& gameId) {
    std::vector<VersionInfo> versions;
    HttpGet("/api/games/" + gameId + "/versions");
    // Parse response...
    return versions;
}

bool ApiClient::CheckUpdate(const std::string& gameId, const std::string& currentVersion) {
    std::string response = HttpGet("/api/games/" + gameId + "/status?current_version=" + currentVersion);
    return ExtractJsonBool(response, "needs_update");
}

bool ApiClient::DownloadCheat(const std::string& gameId, const std::string& version,
                              const std::string& savePath, ProgressCallback progress) {
    return HttpDownload("/api/download/" + gameId + "/" + version, savePath, progress);
}

bool ApiClient::DownloadLatest(const std::string& gameId, const std::string& savePath,
                               ProgressCallback progress) {
    return HttpDownload("/api/download/" + gameId + "/latest", savePath, progress);
}

bool ApiClient::DownloadFile(const std::string& gameId, const std::string& fileType,
                             const std::string& savePath, std::function<void(size_t, size_t)> progress) {
    // Wrapper that converts to ProgressCallback
    ProgressCallback cb = nullptr;
    if (progress) {
        cb = [progress](const DownloadProgress& p) {
            progress(p.downloaded, p.total);
        };
    }
    return HttpDownload("/api/download/" + gameId + "/" + fileType, savePath, cb);
}

std::string ApiClient::GetOffsets(const std::string& gameId) {
    return HttpGet("/api/offsets/" + gameId);
}

std::string ApiClient::GetOffsetsHash(const std::string& gameId) {
    std::string response = HttpGet("/api/offsets/" + gameId + "/hash");
    return ExtractJsonString(response, "hash");
}

} // namespace launcher

