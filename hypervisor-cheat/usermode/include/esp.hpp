/**
 * HYPERVISOR CHEAT - ESP (Wallhack)
 * Visual ESP features for CS2
 */

#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <string>

#include "../../common/types.h"
#include "../../common/offsets.h"

namespace esp {

// ============================================
// Configuration
// ============================================

struct Color {
    float r, g, b, a;
    
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    static Color Red()    { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static Color Green()  { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static Color Blue()   { return Color(0.0f, 0.5f, 1.0f, 1.0f); }
    static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 1.0f); }
    static Color White()  { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static Color Orange() { return Color(1.0f, 0.5f, 0.0f, 1.0f); }
};

struct ESPConfig {
    // Master toggle
    bool enabled = true;
    
    // Features
    bool boxEnabled = true;
    bool healthBarEnabled = true;
    bool nameEnabled = true;
    bool distanceEnabled = true;
    bool headDotEnabled = false;
    bool snaplineEnabled = false;
    
    // Style
    int boxStyle = 0;  // 0 = 2D, 1 = 2D Corner, 2 = 3D
    float boxThickness = 1.5f;
    float maxDistance = 500.0f;
    
    // Colors
    Color enemyColor = Color::Red();
    Color enemyVisibleColor = Color::Yellow();
    Color teamColor = Color::Blue();
    Color healthBarColor = Color::Green();
    Color nameColor = Color::White();
};

extern ESPConfig g_config;

// ============================================
// Player Data (cached from hypervisor)
// ============================================

struct PlayerCache {
    uint32_t index;
    uint32_t health;
    uint32_t team;
    bool alive;
    bool visible;
    bool enemy;
    
    VECTOR3 position;
    VECTOR3 headPosition;
    
    std::string name;
    float distance;
    
    // Screen coordinates (calculated)
    VECTOR2 screenPos;
    VECTOR2 screenHead;
    float screenHeight;
    float screenWidth;
    bool onScreen;
};

// ============================================
// ESP Class
// ============================================

class ESP {
public:
    static ESP& Get() {
        static ESP instance;
        return instance;
    }
    
    // Initialization
    bool Initialize(HWND overlayWindow);
    void Shutdown();
    
    // Main loop
    void Update();  // Read data from hypervisor
    void Render();  // Draw ESP
    
    // Accessors
    const std::vector<PlayerCache>& GetPlayers() const { return m_players; }
    ESPConfig& Config() { return g_config; }
    
private:
    ESP() = default;
    
    // Data reading
    void ReadPlayers();
    void ReadViewMatrix();
    bool WorldToScreen(const VECTOR3& world, VECTOR2& screen);
    
    // Rendering
    void DrawPlayer(const PlayerCache& player);
    void DrawBox2D(const PlayerCache& player, const Color& color);
    void DrawBox2DCorner(const PlayerCache& player, const Color& color);
    void DrawHealthBar(const PlayerCache& player);
    void DrawName(const PlayerCache& player);
    void DrawDistance(const PlayerCache& player);
    void DrawHeadDot(const PlayerCache& player);
    void DrawSnapline(const PlayerCache& player);
    
    // Drawing primitives (implemented in render)
    void DrawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness = 1.0f);
    void DrawRect(float x, float y, float w, float h, const Color& color, float thickness = 1.0f);
    void DrawFilledRect(float x, float y, float w, float h, const Color& color);
    void DrawCircle(float x, float y, float radius, const Color& color, float thickness = 1.0f);
    void DrawText(float x, float y, const char* text, const Color& color);
    
    // State
    bool m_initialized = false;
    HWND m_window = nullptr;
    
    // Game data
    uint32_t m_gamePid = 0;
    uint64_t m_gameCr3 = 0;
    uint64_t m_clientBase = 0;
    
    uint32_t m_localPlayerIndex = 0;
    uint32_t m_localTeam = 0;
    
    float m_viewMatrix[16] = {0};
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    
    std::vector<PlayerCache> m_players;
};

// ============================================
// Convenience
// ============================================

inline bool Init(HWND window) {
    return ESP::Get().Initialize(window);
}

inline void Update() {
    ESP::Get().Update();
}

inline void Render() {
    ESP::Get().Render();
}

inline ESPConfig& Config() {
    return ESP::Get().Config();
}

} // namespace esp

