/**
 * HYPERVISOR CHEAT - Internal ESP Implementation
 * Direct memory reading (no syscalls - we're inside the process!)
 */

#include "../include/esp.hpp"
#include <cmath>
#include <algorithm>

#ifdef HAS_IMGUI
#include <imgui.h>
#endif

namespace esp {

// ============================================
// Globals
// ============================================

Config g_config;

static uintptr_t g_clientBase = 0;
static uintptr_t g_engineBase = 0;
static int g_localTeam = 0;
static float g_viewMatrix[16] = {0};
static std::vector<PlayerInfo> g_players;

// ============================================
// Module Helpers
// ============================================

uintptr_t GetClientBase() {
    if (!g_clientBase) {
        g_clientBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
    }
    return g_clientBase;
}

uintptr_t GetEngineBase() {
    if (!g_engineBase) {
        g_engineBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("engine2.dll"));
    }
    return g_engineBase;
}

// ============================================
// World to Screen
// ============================================

bool WorldToScreen(const Vector3& world, Vector2& screen) {
    // Get window size
    HWND hwnd = FindWindowA("SDL_app", nullptr);
    if (!hwnd) return false;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    float screenW = static_cast<float>(rect.right);
    float screenH = static_cast<float>(rect.bottom);
    
    // W2S calculation
    float w = g_viewMatrix[3] * world.x + 
              g_viewMatrix[7] * world.y + 
              g_viewMatrix[11] * world.z + 
              g_viewMatrix[15];
    
    if (w < 0.001f) return false;
    
    float invW = 1.0f / w;
    
    float x = g_viewMatrix[0] * world.x + 
              g_viewMatrix[4] * world.y + 
              g_viewMatrix[8] * world.z + 
              g_viewMatrix[12];
    
    float y = g_viewMatrix[1] * world.x + 
              g_viewMatrix[5] * world.y + 
              g_viewMatrix[9] * world.z + 
              g_viewMatrix[13];
    
    x *= invW;
    y *= invW;
    
    screen.x = (screenW / 2.0f) * (1.0f + x);
    screen.y = (screenH / 2.0f) * (1.0f - y);
    
    return true;
}

// ============================================
// Initialize / Shutdown
// ============================================

void Initialize() {
    g_players.reserve(64);
}

void Shutdown() {
    g_players.clear();
}

// ============================================
// Update (Read Game Data)
// ============================================

void Update() {
    if (!g_config.enabled) return;
    
    g_players.clear();
    
    uintptr_t client = GetClientBase();
    if (!client) return;
    
    // Read view matrix
    uintptr_t viewMatrixAddr = client + offsets::client::dwViewMatrix;
    for (int i = 0; i < 16; i++) {
        g_viewMatrix[i] = Read<float>(viewMatrixAddr + i * 4);
    }
    
    // Read local player controller
    uintptr_t localController = Read<uintptr_t>(client + offsets::client::dwLocalPlayerController);
    if (!localController) return;
    
    // Get local team
    uintptr_t localPawn = Read<uintptr_t>(client + offsets::client::dwLocalPlayerPawn);
    if (localPawn) {
        g_localTeam = Read<uint8_t>(localPawn + offsets::entity::m_iTeamNum);
    }
    
    // Get local position for distance
    Vector3 localPos = {0, 0, 0};
    if (localPawn) {
        uintptr_t sceneNode = Read<uintptr_t>(localPawn + offsets::entity::m_pGameSceneNode);
        if (sceneNode) {
            localPos = Read<Vector3>(sceneNode + offsets::scene::m_vecAbsOrigin);
        }
    }
    
    // Read entity list
    uintptr_t entityList = Read<uintptr_t>(client + offsets::client::dwEntityList);
    if (!entityList) return;
    
    // Iterate players
    for (int i = 1; i <= 64; i++) {
        // Get list entry
        uintptr_t listEntry = Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9)) + 16);
        if (!listEntry) continue;
        
        // Get controller
        uintptr_t controller = Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
        if (!controller || controller == localController) continue;
        
        // Check alive
        bool isAlive = Read<uint8_t>(controller + offsets::controller::m_bPawnIsAlive);
        if (!isAlive) continue;
        
        // Get pawn handle
        uint32_t pawnHandle = Read<uint32_t>(controller + offsets::controller::m_hPlayerPawn);
        if (!pawnHandle) continue;
        
        // Get pawn list entry
        uintptr_t pawnListEntry = Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
        if (!pawnListEntry) continue;
        
        // Get pawn
        uintptr_t pawn = Read<uintptr_t>(pawnListEntry + 120 * (pawnHandle & 0x1FF));
        if (!pawn) continue;
        
        PlayerInfo player = {};
        player.index = i;
        player.alive = true;
        
        // Health
        player.health = Read<int>(pawn + offsets::entity::m_iHealth);
        if (player.health <= 0 || player.health > 100) continue;
        
        // Team
        player.team = Read<uint8_t>(pawn + offsets::entity::m_iTeamNum);
        player.enemy = (player.team != g_localTeam);
        
        // Position
        uintptr_t sceneNode = Read<uintptr_t>(pawn + offsets::entity::m_pGameSceneNode);
        if (!sceneNode) continue;
        
        player.position = Read<Vector3>(sceneNode + offsets::scene::m_vecAbsOrigin);
        
        // Head position (approximate)
        player.headPosition = player.position;
        player.headPosition.z += 72.0f;
        
        // Try to get bones
        uintptr_t skeletonInstance = Read<uintptr_t>(sceneNode + offsets::skeleton::m_modelState);
        if (skeletonInstance) {
            uintptr_t boneArray = Read<uintptr_t>(skeletonInstance + offsets::skeleton::m_boneArray);
            if (boneArray) {
                // Read head bone
                Vector3 headBone = Read<Vector3>(boneArray + offsets::bones::head * 32);
                if (headBone.x != 0 || headBone.y != 0 || headBone.z != 0) {
                    player.headPosition = headBone;
                }
                
                // Read all bones for skeleton
                if (g_config.skeletonEnabled) {
                    for (int b = 0; b < 28; b++) {
                        player.bones[b] = Read<Vector3>(boneArray + b * 32);
                    }
                }
            }
        }
        
        // Name
        uintptr_t namePtr = Read<uintptr_t>(controller + offsets::controller::m_sSanitizedPlayerName);
        if (namePtr) {
            char nameBuffer[32] = {};
            for (int j = 0; j < 31; j++) {
                nameBuffer[j] = Read<char>(namePtr + j);
                if (nameBuffer[j] == 0) break;
            }
            player.name = nameBuffer;
        }
        
        // Distance
        player.distance = (localPos - player.position).Length() / 100.0f;
        if (player.distance > g_config.maxDistance) continue;
        
        // Visibility (spotted)
        uintptr_t spottedState = pawn + offsets::player::m_entitySpottedState;
        player.visible = Read<uint8_t>(spottedState + offsets::player::m_bSpotted);
        
        // World to screen
        player.onScreen = WorldToScreen(player.position, player.screenPos);
        
        Vector2 headScreen;
        if (WorldToScreen(player.headPosition, headScreen)) {
            player.screenHead = headScreen;
            player.screenHeight = player.screenPos.y - headScreen.y;
            player.screenWidth = player.screenHeight / 2.4f;
        }
        
        g_players.push_back(player);
    }
}

// ============================================
// Render
// ============================================

void Render() {
#ifdef HAS_IMGUI
    if (!g_config.enabled) return;
    
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
    for (const auto& player : g_players) {
        if (!player.onScreen) continue;
        
        // Choose color
        ImU32 color;
        if (player.enemy) {
            if (player.visible) {
                color = IM_COL32(
                    static_cast<int>(g_config.enemyVisibleColor[0] * 255),
                    static_cast<int>(g_config.enemyVisibleColor[1] * 255),
                    static_cast<int>(g_config.enemyVisibleColor[2] * 255),
                    static_cast<int>(g_config.enemyVisibleColor[3] * 255)
                );
            } else {
                color = IM_COL32(
                    static_cast<int>(g_config.enemyColor[0] * 255),
                    static_cast<int>(g_config.enemyColor[1] * 255),
                    static_cast<int>(g_config.enemyColor[2] * 255),
                    static_cast<int>(g_config.enemyColor[3] * 255)
                );
            }
        } else {
            color = IM_COL32(
                static_cast<int>(g_config.teamColor[0] * 255),
                static_cast<int>(g_config.teamColor[1] * 255),
                static_cast<int>(g_config.teamColor[2] * 255),
                static_cast<int>(g_config.teamColor[3] * 255)
            );
        }
        
        float x = player.screenHead.x - player.screenWidth / 2;
        float y = player.screenHead.y;
        float w = player.screenWidth;
        float h = player.screenHeight;
        
        // Box
        if (g_config.boxEnabled) {
            if (g_config.boxStyle == 0) {
                // 2D Box
                draw->AddRect(
                    ImVec2(x, y),
                    ImVec2(x + w, y + h),
                    color,
                    0.0f,
                    0,
                    g_config.boxThickness
                );
            } else {
                // Corner Box
                float cornerLen = w / 4;
                
                // Top left
                draw->AddLine(ImVec2(x, y), ImVec2(x + cornerLen, y), color, g_config.boxThickness);
                draw->AddLine(ImVec2(x, y), ImVec2(x, y + cornerLen), color, g_config.boxThickness);
                
                // Top right
                draw->AddLine(ImVec2(x + w, y), ImVec2(x + w - cornerLen, y), color, g_config.boxThickness);
                draw->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + cornerLen), color, g_config.boxThickness);
                
                // Bottom left
                draw->AddLine(ImVec2(x, y + h), ImVec2(x + cornerLen, y + h), color, g_config.boxThickness);
                draw->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - cornerLen), color, g_config.boxThickness);
                
                // Bottom right
                draw->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - cornerLen, y + h), color, g_config.boxThickness);
                draw->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - cornerLen), color, g_config.boxThickness);
            }
        }
        
        // Health bar
        if (g_config.healthBarEnabled) {
            float barX = x - 6;
            float barW = 4;
            float healthPercent = player.health / 100.0f;
            float barH = h * healthPercent;
            float barY = y + h - barH;
            
            // Background
            draw->AddRectFilled(
                ImVec2(barX - 1, y - 1),
                ImVec2(barX + barW + 1, y + h + 1),
                IM_COL32(0, 0, 0, 180)
            );
            
            // Health color gradient
            ImU32 healthColor;
            if (healthPercent > 0.5f) {
                float t = (healthPercent - 0.5f) * 2;
                healthColor = IM_COL32(static_cast<int>((1.0f - t) * 255), 255, 0, 255);
            } else {
                float t = healthPercent * 2;
                healthColor = IM_COL32(255, static_cast<int>(t * 255), 0, 255);
            }
            
            draw->AddRectFilled(
                ImVec2(barX, barY),
                ImVec2(barX + barW, y + h),
                healthColor
            );
        }
        
        // Name
        if (g_config.nameEnabled && !player.name.empty()) {
            ImVec2 textSize = ImGui::CalcTextSize(player.name.c_str());
            float textX = player.screenHead.x - textSize.x / 2;
            float textY = y - textSize.y - 2;
            
            // Shadow
            draw->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 255), player.name.c_str());
            // Text
            draw->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), player.name.c_str());
        }
        
        // Distance
        if (g_config.distanceEnabled) {
            char distText[16];
            snprintf(distText, sizeof(distText), "%.0fm", player.distance);
            
            ImVec2 textSize = ImGui::CalcTextSize(distText);
            float textX = player.screenPos.x - textSize.x / 2;
            float textY = y + h + 2;
            
            draw->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 255), distText);
            draw->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), distText);
        }
        
        // Head dot
        if (g_config.headDotEnabled) {
            draw->AddCircleFilled(
                ImVec2(player.screenHead.x, player.screenHead.y),
                3.0f,
                color
            );
        }
        
        // Snapline
        if (g_config.snaplineEnabled) {
            HWND hwnd = FindWindowA("SDL_app", nullptr);
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            draw->AddLine(
                ImVec2(rect.right / 2.0f, static_cast<float>(rect.bottom)),
                ImVec2(player.screenPos.x, player.screenPos.y),
                color,
                1.0f
            );
        }
        
        // Skeleton
        if (g_config.skeletonEnabled) {
            ImU32 skelColor = IM_COL32(
                static_cast<int>(g_config.skeletonColor[0] * 255),
                static_cast<int>(g_config.skeletonColor[1] * 255),
                static_cast<int>(g_config.skeletonColor[2] * 255),
                static_cast<int>(g_config.skeletonColor[3] * 255)
            );
            
            // Bone connections
            int connections[][2] = {
                {offsets::bones::head, offsets::bones::neck},
                {offsets::bones::neck, offsets::bones::spine_2},
                {offsets::bones::spine_2, offsets::bones::spine_1},
                {offsets::bones::spine_1, offsets::bones::pelvis},
                {offsets::bones::neck, offsets::bones::left_shoulder},
                {offsets::bones::left_shoulder, offsets::bones::left_elbow},
                {offsets::bones::left_elbow, offsets::bones::left_hand},
                {offsets::bones::neck, offsets::bones::right_shoulder},
                {offsets::bones::right_shoulder, offsets::bones::right_elbow},
                {offsets::bones::right_elbow, offsets::bones::right_hand},
                {offsets::bones::pelvis, offsets::bones::left_hip},
                {offsets::bones::left_hip, offsets::bones::left_knee},
                {offsets::bones::left_knee, offsets::bones::left_foot},
                {offsets::bones::pelvis, offsets::bones::right_hip},
                {offsets::bones::right_hip, offsets::bones::right_knee},
                {offsets::bones::right_knee, offsets::bones::right_foot},
            };
            
            for (auto& conn : connections) {
                Vector2 screen1, screen2;
                if (WorldToScreen(player.bones[conn[0]], screen1) &&
                    WorldToScreen(player.bones[conn[1]], screen2)) {
                    draw->AddLine(
                        ImVec2(screen1.x, screen1.y),
                        ImVec2(screen2.x, screen2.y),
                        skelColor,
                        1.5f
                    );
                }
            }
        }
    }
#else
    // No ImGui - rendering requires HAS_IMGUI
    (void)g_players;
#endif
}

} // namespace esp

