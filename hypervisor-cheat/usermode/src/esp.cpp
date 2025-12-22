/**
 * HYPERVISOR CHEAT - ESP Implementation
 * CS2 Wallhack with hypervisor-based memory reading
 */

#include "../include/esp.hpp"
#include "../include/hypervisor.hpp"
#include <cmath>
#include <algorithm>

namespace esp {

// ============================================
// Global Config
// ============================================

ESPConfig g_config;

// ============================================
// SIMD Math
// ============================================

static inline float Length(const VECTOR3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float Distance(const VECTOR3& a, const VECTOR3& b) {
    VECTOR3 diff = {a.x - b.x, a.y - b.y, a.z - b.z};
    return Length(diff);
}

// ============================================
// ESP Implementation
// ============================================

bool ESP::Initialize(HWND overlayWindow) {
    if (m_initialized) return true;
    
    m_window = overlayWindow;
    
    // Get window size
    RECT rect;
    GetClientRect(m_window, &rect);
    m_screenWidth = rect.right - rect.left;
    m_screenHeight = rect.bottom - rect.top;
    
    // Find CS2 process via hypervisor
    m_gamePid = hv::Hypervisor::Get().FindProcess("cs2.exe");
    
    if (m_gamePid == 0) {
        return false;
    }
    
    // Get CR3
    auto cr3 = hv::Hypervisor::Get().GetProcessCr3(m_gamePid);
    if (!cr3.has_value()) {
        return false;
    }
    m_gameCr3 = cr3.value();
    
    // Get client.dll base
    auto clientBase = hv::Hypervisor::Get().GetModuleBase(m_gamePid, "client.dll");
    if (!clientBase.has_value()) {
        return false;
    }
    m_clientBase = clientBase.value();
    
    m_initialized = true;
    m_players.reserve(64);
    
    return true;
}

void ESP::Shutdown() {
    m_initialized = false;
    m_players.clear();
}

// ============================================
// Update
// ============================================

void ESP::Update() {
    if (!m_initialized || !g_config.enabled) return;
    
    ReadViewMatrix();
    ReadPlayers();
}

void ESP::ReadViewMatrix() {
    uint64_t viewMatrixAddr = m_clientBase + offsets::client_dll::dwViewMatrix;
    
    hv::Hypervisor::Get().ReadMemory(
        m_gameCr3,
        viewMatrixAddr,
        m_viewMatrix,
        sizeof(m_viewMatrix)
    );
}

void ESP::ReadPlayers() {
    m_players.clear();
    
    // Read entity list
    uint64_t entityList = hv::Read<uint64_t>(
        m_gameCr3,
        m_clientBase + offsets::client_dll::dwEntityList
    );
    
    if (!entityList) return;
    
    // Read local player controller
    uint64_t localController = hv::Read<uint64_t>(
        m_gameCr3,
        m_clientBase + offsets::client_dll::dwLocalPlayerController
    );
    
    if (!localController) return;
    
    // Get local team
    m_localTeam = hv::Read<uint8_t>(
        m_gameCr3,
        localController + offsets::entity::m_iTeamNum
    );
    
    // Get local player pawn for position
    uint64_t localPawn = hv::Read<uint64_t>(
        m_gameCr3,
        m_clientBase + offsets::client_dll::dwLocalPlayerPawn
    );
    
    VECTOR3 localPos = {0, 0, 0};
    if (localPawn) {
        uint64_t localSceneNode = hv::Read<uint64_t>(
            m_gameCr3,
            localPawn + offsets::entity::m_pGameSceneNode
        );
        if (localSceneNode) {
            localPos = hv::Read<VECTOR3>(
                m_gameCr3,
                localSceneNode + offsets::scene::m_vecAbsOrigin
            );
        }
    }
    
    // Iterate players (max 64)
    for (int i = 1; i <= 64; i++) {
        // Get list entry
        uint64_t listEntry = hv::Read<uint64_t>(
            m_gameCr3,
            entityList + (8 * ((i & 0x7FFF) >> 9)) + 16
        );
        
        if (!listEntry) continue;
        
        // Get controller
        uint64_t controller = hv::Read<uint64_t>(
            m_gameCr3,
            listEntry + 120 * (i & 0x1FF)
        );
        
        if (!controller || controller == localController) continue;
        
        // Check if alive
        bool isAlive = hv::Read<uint8_t>(
            m_gameCr3,
            controller + offsets::controller::m_bPawnIsAlive
        );
        
        if (!isAlive) continue;
        
        // Get pawn handle
        uint32_t pawnHandle = hv::Read<uint32_t>(
            m_gameCr3,
            controller + offsets::controller::m_hPlayerPawn
        );
        
        if (!pawnHandle) continue;
        
        // Get pawn list entry
        uint64_t pawnListEntry = hv::Read<uint64_t>(
            m_gameCr3,
            entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16
        );
        
        if (!pawnListEntry) continue;
        
        // Get pawn
        uint64_t pawn = hv::Read<uint64_t>(
            m_gameCr3,
            pawnListEntry + 120 * (pawnHandle & 0x1FF)
        );
        
        if (!pawn) continue;
        
        // Build player cache
        PlayerCache player = {};
        player.index = i;
        player.alive = true;
        
        // Health
        player.health = hv::Read<int32_t>(m_gameCr3, pawn + offsets::entity::m_iHealth);
        if (player.health <= 0 || player.health > 100) continue;
        
        // Team
        player.team = hv::Read<uint8_t>(m_gameCr3, pawn + offsets::entity::m_iTeamNum);
        player.enemy = (player.team != m_localTeam);
        
        // Skip teammates if needed (could be config option)
        // if (!player.enemy) continue;
        
        // Position
        uint64_t sceneNode = hv::Read<uint64_t>(
            m_gameCr3,
            pawn + offsets::entity::m_pGameSceneNode
        );
        
        if (!sceneNode) continue;
        
        player.position = hv::Read<VECTOR3>(
            m_gameCr3,
            sceneNode + offsets::scene::m_vecAbsOrigin
        );
        
        // Head position (from bones or estimated)
        player.headPosition = player.position;
        player.headPosition.z += 72.0f;  // Approximate head height
        
        // Try to get actual bone position
        uint64_t skeletonInstance = hv::Read<uint64_t>(m_gameCr3, sceneNode + 0x170);
        if (skeletonInstance) {
            uint64_t boneArray = hv::Read<uint64_t>(m_gameCr3, skeletonInstance + 0x1F0);
            if (boneArray) {
                // Bone 6 = head
                VECTOR3 headBone = hv::Read<VECTOR3>(m_gameCr3, boneArray + 6 * 32);
                if (headBone.x != 0 || headBone.y != 0 || headBone.z != 0) {
                    player.headPosition = headBone;
                }
            }
        }
        
        // Name
        uint64_t nameAddr = hv::Read<uint64_t>(
            m_gameCr3,
            controller + offsets::controller::m_sSanitizedPlayerName
        );
        
        if (nameAddr) {
            char nameBuffer[32] = {};
            hv::Hypervisor::Get().ReadMemory(
                m_gameCr3,
                nameAddr,
                nameBuffer,
                sizeof(nameBuffer) - 1
            );
            player.name = nameBuffer;
        }
        
        // Distance
        player.distance = Distance(localPos, player.position) / 100.0f;  // To meters
        
        if (player.distance > g_config.maxDistance) continue;
        
        // Visibility (spotted check)
        uint64_t spottedState = pawn + offsets::player::m_entitySpottedState;
        player.visible = hv::Read<uint8_t>(
            m_gameCr3,
            spottedState + offsets::player::m_bSpotted
        );
        
        // World to screen
        player.onScreen = WorldToScreen(player.position, player.screenPos);
        
        VECTOR2 headScreen;
        if (WorldToScreen(player.headPosition, headScreen)) {
            player.screenHead = headScreen;
            player.screenHeight = player.screenPos.y - headScreen.y;
            player.screenWidth = player.screenHeight / 2.4f;
        }
        
        m_players.push_back(player);
    }
}

bool ESP::WorldToScreen(const VECTOR3& world, VECTOR2& screen) {
    // W2S calculation
    float w = m_viewMatrix[3] * world.x + 
              m_viewMatrix[7] * world.y + 
              m_viewMatrix[11] * world.z + 
              m_viewMatrix[15];
    
    if (w < 0.001f) return false;
    
    float invW = 1.0f / w;
    
    float x = m_viewMatrix[0] * world.x + 
              m_viewMatrix[4] * world.y + 
              m_viewMatrix[8] * world.z + 
              m_viewMatrix[12];
    
    float y = m_viewMatrix[1] * world.x + 
              m_viewMatrix[5] * world.y + 
              m_viewMatrix[9] * world.z + 
              m_viewMatrix[13];
    
    x *= invW;
    y *= invW;
    
    screen.x = (m_screenWidth / 2.0f) * (1.0f + x);
    screen.y = (m_screenHeight / 2.0f) * (1.0f - y);
    
    return true;
}

// ============================================
// Render
// ============================================

void ESP::Render() {
    if (!m_initialized || !g_config.enabled) return;
    
    for (const auto& player : m_players) {
        if (!player.onScreen) continue;
        DrawPlayer(player);
    }
}

void ESP::DrawPlayer(const PlayerCache& player) {
    // Determine color
    Color color = player.enemy ? 
        (player.visible ? g_config.enemyVisibleColor : g_config.enemyColor) :
        g_config.teamColor;
    
    // Draw enabled features
    if (g_config.boxEnabled) {
        switch (g_config.boxStyle) {
            case 0: DrawBox2D(player, color); break;
            case 1: DrawBox2DCorner(player, color); break;
            default: DrawBox2D(player, color); break;
        }
    }
    
    if (g_config.healthBarEnabled) {
        DrawHealthBar(player);
    }
    
    if (g_config.nameEnabled) {
        DrawName(player);
    }
    
    if (g_config.distanceEnabled) {
        DrawDistance(player);
    }
    
    if (g_config.headDotEnabled) {
        DrawHeadDot(player);
    }
    
    if (g_config.snaplineEnabled) {
        DrawSnapline(player);
    }
}

void ESP::DrawBox2D(const PlayerCache& player, const Color& color) {
    float x = player.screenHead.x - player.screenWidth / 2;
    float y = player.screenHead.y;
    float w = player.screenWidth;
    float h = player.screenHeight;
    
    DrawRect(x, y, w, h, color, g_config.boxThickness);
}

void ESP::DrawBox2DCorner(const PlayerCache& player, const Color& color) {
    float x = player.screenHead.x - player.screenWidth / 2;
    float y = player.screenHead.y;
    float w = player.screenWidth;
    float h = player.screenHeight;
    float cornerLength = w / 4;
    
    // Top left
    DrawLine(x, y, x + cornerLength, y, color, g_config.boxThickness);
    DrawLine(x, y, x, y + cornerLength, color, g_config.boxThickness);
    
    // Top right
    DrawLine(x + w, y, x + w - cornerLength, y, color, g_config.boxThickness);
    DrawLine(x + w, y, x + w, y + cornerLength, color, g_config.boxThickness);
    
    // Bottom left
    DrawLine(x, y + h, x + cornerLength, y + h, color, g_config.boxThickness);
    DrawLine(x, y + h, x, y + h - cornerLength, color, g_config.boxThickness);
    
    // Bottom right
    DrawLine(x + w, y + h, x + w - cornerLength, y + h, color, g_config.boxThickness);
    DrawLine(x + w, y + h, x + w, y + h - cornerLength, color, g_config.boxThickness);
}

void ESP::DrawHealthBar(const PlayerCache& player) {
    float x = player.screenHead.x - player.screenWidth / 2 - 6;
    float y = player.screenHead.y;
    float w = 4;
    float h = player.screenHeight;
    
    // Background
    DrawFilledRect(x - 1, y - 1, w + 2, h + 2, Color(0, 0, 0, 0.7f));
    
    // Health percentage
    float healthPercent = player.health / 100.0f;
    float barHeight = h * healthPercent;
    float barY = y + h - barHeight;
    
    // Color gradient (green to red)
    Color healthColor;
    if (healthPercent > 0.5f) {
        float t = (healthPercent - 0.5f) * 2;
        healthColor = Color(1.0f - t, 1.0f, 0.0f, 1.0f);
    } else {
        float t = healthPercent * 2;
        healthColor = Color(1.0f, t, 0.0f, 1.0f);
    }
    
    DrawFilledRect(x, barY, w, barHeight, healthColor);
}

void ESP::DrawName(const PlayerCache& player) {
    if (player.name.empty()) return;
    
    float x = player.screenHead.x;
    float y = player.screenHead.y - 14;
    
    DrawText(x, y, player.name.c_str(), g_config.nameColor);
}

void ESP::DrawDistance(const PlayerCache& player) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0fm", player.distance);
    
    float x = player.screenPos.x;
    float y = player.screenPos.y + 4;
    
    DrawText(x, y, buffer, g_config.nameColor);
}

void ESP::DrawHeadDot(const PlayerCache& player) {
    DrawCircle(player.screenHead.x, player.screenHead.y, 3.0f, 
               player.enemy ? g_config.enemyColor : g_config.teamColor);
}

void ESP::DrawSnapline(const PlayerCache& player) {
    float startX = (float)m_screenWidth / 2;
    float startY = (float)m_screenHeight;
    
    DrawLine(startX, startY, player.screenPos.x, player.screenPos.y,
             player.enemy ? g_config.enemyColor : g_config.teamColor);
}

// ============================================
// Drawing Primitives (Stubs - implement with your renderer)
// ============================================

void ESP::DrawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness) {
    // TODO: Implement with your DirectX renderer or ImGui
    // Example with ImGui:
    // ImGui::GetBackgroundDrawList()->AddLine(
    //     ImVec2(x1, y1), ImVec2(x2, y2),
    //     IM_COL32(color.r * 255, color.g * 255, color.b * 255, color.a * 255),
    //     thickness
    // );
}

void ESP::DrawRect(float x, float y, float w, float h, const Color& color, float thickness) {
    // TODO: Implement
}

void ESP::DrawFilledRect(float x, float y, float w, float h, const Color& color) {
    // TODO: Implement
}

void ESP::DrawCircle(float x, float y, float radius, const Color& color, float thickness) {
    // TODO: Implement
}

void ESP::DrawText(float x, float y, const char* text, const Color& color) {
    // TODO: Implement
}

} // namespace esp

