/**
 * Cheat Launcher - Main Entry Point
 * 
 * This is a console-based launcher. For production, you'd want a GUI
 * using ImGui, Qt, or Windows native controls.
 */

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../include/api.hpp"
#include "../include/injector.hpp"

namespace fs = std::filesystem;
using namespace launcher;

// ============================================
// Config
// ============================================

const std::string SERVER_URL = "http://your-server.com:3000";
const std::string CHEAT_DIR = "cheats";
const std::string CONFIG_FILE = "launcher.cfg";

// ============================================
// Helper Functions
// ============================================

void ClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void SetConsoleColor(int color) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
}

void PrintLogo() {
    SetConsoleColor(11); // Cyan
    std::cout << R"(
   ██████╗██╗  ██╗███████╗ █████╗ ████████╗
  ██╔════╝██║  ██║██╔════╝██╔══██╗╚══██╔══╝
  ██║     ███████║█████╗  ███████║   ██║   
  ██║     ██╔══██║██╔══╝  ██╔══██║   ██║   
  ╚██████╗██║  ██║███████╗██║  ██║   ██║   
   ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   
    )" << std::endl;
    SetConsoleColor(7); // Reset
    std::cout << "  Launcher v1.0.0" << std::endl;
    std::cout << "  ─────────────────────────────────────────\n" << std::endl;
}

void PrintError(const std::string& msg) {
    SetConsoleColor(12); // Red
    std::cout << "[ERROR] " << msg << std::endl;
    SetConsoleColor(7);
}

void PrintSuccess(const std::string& msg) {
    SetConsoleColor(10); // Green
    std::cout << "[OK] " << msg << std::endl;
    SetConsoleColor(7);
}

void PrintInfo(const std::string& msg) {
    SetConsoleColor(14); // Yellow
    std::cout << "[INFO] " << msg << std::endl;
    SetConsoleColor(7);
}

// Save token to config file
void SaveToken(const std::string& token) {
    std::ofstream file(CONFIG_FILE);
    if (file.is_open()) {
        file << token;
        file.close();
    }
}

// Load token from config file
std::string LoadToken() {
    std::ifstream file(CONFIG_FILE);
    if (file.is_open()) {
        std::string token;
        std::getline(file, token);
        file.close();
        return token;
    }
    return "";
}

// ============================================
// Menu Functions
// ============================================

bool DoLogin() {
    std::string username, password;
    
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Password: ";
    std::cin >> password;
    
    PrintInfo("Logging in...");
    
    auto result = ApiClient::Get().Login(username, password);
    
    if (result.success) {
        PrintSuccess("Welcome, " + result.user.username + "!");
        SaveToken(result.token);
        return true;
    } else {
        PrintError(result.error);
        return false;
    }
}

void ShowGames() {
    auto games = ApiClient::Get().GetGames();
    
    if (games.empty()) {
        PrintInfo("No games available.");
        return;
    }
    
    std::cout << "\n  Available Games:\n";
    std::cout << "  ─────────────────────────────────────────\n";
    
    int i = 1;
    for (const auto& game : games) {
        SetConsoleColor(game.has_license ? 10 : 8);
        std::cout << "  [" << i++ << "] " << game.name;
        if (game.has_license) {
            std::cout << " (v" << game.latest_version << ")";
        } else {
            std::cout << " [NO LICENSE]";
        }
        std::cout << std::endl;
    }
    SetConsoleColor(7);
    std::cout << std::endl;
}

void LaunchGame(const std::string& gameId, const std::string& processName) {
    auto& api = ApiClient::Get();
    
    // Get game info
    auto gameOpt = api.GetGame(gameId);
    if (!gameOpt.has_value()) {
        PrintError("Game not found.");
        return;
    }
    
    auto game = gameOpt.value();
    
    if (!game.has_license) {
        PrintError("No license for this game.");
        return;
    }
    
    // Create cheats directory
    fs::create_directories(CHEAT_DIR);
    
    std::string cheatPath = CHEAT_DIR + "/" + gameId + ".dll";
    std::string localVersion = "";
    
    // Check if we have a local copy
    if (fs::exists(cheatPath)) {
        // TODO: Read local version from a manifest file
        localVersion = "1.0.0"; // Placeholder
    }
    
    // Check for updates
    bool needsDownload = !fs::exists(cheatPath) || api.CheckUpdate(gameId, localVersion);
    
    if (needsDownload) {
        PrintInfo("Downloading latest version...");
        
        bool success = api.DownloadLatest(gameId, cheatPath, [](const DownloadProgress& p) {
            std::cout << "\r  Progress: " << (int)p.percent << "%    " << std::flush;
        });
        
        std::cout << std::endl;
        
        if (!success) {
            PrintError("Download failed!");
            return;
        }
        
        PrintSuccess("Download complete.");
    }
    
    // Check offsets update
    std::string offsetsPath = CHEAT_DIR + "/" + gameId + "_offsets.json";
    PrintInfo("Updating offsets...");
    
    std::string offsets = api.GetOffsets(gameId);
    if (!offsets.empty()) {
        std::ofstream offsetsFile(offsetsPath);
        offsetsFile << offsets;
        offsetsFile.close();
        PrintSuccess("Offsets updated.");
    }
    
    // Wait for game and inject
    PrintInfo("Waiting for " + processName + "...");
    
    auto result = Injector::WaitAndInject(processName, cheatPath, InjectionMethod::ManualMap);
    
    if (result.success) {
        PrintSuccess("Injected successfully! Module base: 0x" + 
                    std::to_string(result.moduleBase));
    } else {
        PrintError("Injection failed: " + result.error);
    }
}

void MainMenu() {
    while (true) {
        ClearScreen();
        PrintLogo();
        
        std::cout << "  [1] Counter-Strike 2\n";
        std::cout << "  [2] DayZ\n";
        std::cout << "  [3] Rust\n";
        std::cout << "  [4] View All Games\n";
        std::cout << "  [5] Settings\n";
        std::cout << "  [0] Exit\n\n";
        
        std::cout << "  Choice: ";
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                LaunchGame("cs2", "cs2.exe");
                break;
            case 2:
                LaunchGame("dayz", "DayZ_x64.exe");
                break;
            case 3:
                LaunchGame("rust", "RustClient.exe");
                break;
            case 4:
                ShowGames();
                break;
            case 5:
                // Settings menu
                break;
            case 0:
                return;
        }
        
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }
}

// ============================================
// Entry Point
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleTitleA("Cheat Launcher");
#endif
    
    ClearScreen();
    PrintLogo();
    
    // Initialize API
    ApiClient::Get().SetServerUrl(SERVER_URL);
    
    // Try to load saved token
    std::string savedToken = LoadToken();
    if (!savedToken.empty()) {
        ApiClient::Get().SetToken(savedToken);
        
        PrintInfo("Verifying session...");
        if (ApiClient::Get().VerifyToken()) {
            PrintSuccess("Session restored.");
            MainMenu();
            return 0;
        } else {
            PrintInfo("Session expired, please login again.");
        }
    }
    
    // Login flow
    while (true) {
        std::cout << "\n  [1] Login\n";
        std::cout << "  [2] Register\n";
        std::cout << "  [0] Exit\n\n";
        std::cout << "  Choice: ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                if (DoLogin()) {
                    MainMenu();
                    return 0;
                }
                break;
            case 2:
                // Register flow
                break;
            case 0:
                return 0;
        }
    }
    
    return 0;
}

