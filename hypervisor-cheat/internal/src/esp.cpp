/**
 * HYPERVISOR CHEAT - ESP Implementation
 * Reads game data and renders ESP overlays
 */

#include "../include/esp.hpp"
#include "../include/memory.hpp"
#include "../include/xorstr.hpp"
#include "../common/offsets.h"

#include <Windows.h>
#include <vector>
#include <string>
#include <cmath>
#include <mutex>

#ifdef HAS_IMGUI
#include <imgui.h>
#endif

namespace esp {

// ============================================
// Config
// ============================================

Config g_config;

// ============================================
// Structs
// ============================================

struct Vector3 {
    float x, y, z;
    
    Vector3 operator-(const Vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    
    float Length() const {
        return std::sqrt(x*x + y*y + z*z);
    }
};

struct Vector2 {
    float x, y;
};

struct Matrix4x4 {
    float m[4][4];
};

struct PlayerData {
    Vector3 position;
    Vector3 headPos;
    Vector2 screenPos;
    Vector2 screenHead;
    int health;
    int team;
    std::string name;
    bool isEnemy;
    bool isVisible;
    float distance;
    bool valid;
    
    // Skeleton bones
    Vector2 bones2D[28];
    bool bonesValid;
};

// ============================================
// Globals
// ============================================

static std::mutex g_dataMutex;
static std::vector<PlayerData> g_players;
static uintptr_t g_clientBase = 0;
static int g_localTeam = 0;
static Matrix4x4 g_viewMatrix;
static int g_screenWidth = 1920;
static int g_screenHeight = 1080;

// ============================================
// World to Screen
// ============================================

bool WorldToScreen(const Vector3& world, Vector2& screen) {
    const auto& m = g_viewMatrix.m;
    
    float w = m[3][0] * world.x + m[3][1] * world.y + m[3][2] * world.z + m[3][3];
    
    if (w < 0.001f) return false;
    
    float invW = 1.0f / w;
    
    float x = m[0][0] * world.x + m[0][1] * world.y + m[0][2] * world.z + m[0][3];
    float y = m[1][0] * world.x + m[1][1] * world.y + m[1][2] * world.z + m[1][3];
    
    x *= invW;
    y *= invW;
    
    screen.x = (g_screenWidth / 2.0f) * (1.0f + x);
    screen.y = (g_screenHeight / 2.0f) * (1.0f - y);
    
    return true;
}

// ============================================
// Read Player Data
// ============================================

bool ReadPlayerData(uintptr_t controller, uintptr_t entityList, PlayerData& data) {
    data.valid = false;
    
    // Read player pawn handle
    uint32_t pawnHandle = memory::Read<uint32_t>(controller + offsets::entity::m_hPlayerPawn);
    if (!pawnHandle) return false;
    
    // Get pawn from entity list
    uintptr_t listEntry = memory::Read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
    if (!listEntry) return false;
    
    uintptr_t pawn = memory::Read<uintptr_t>(listEntry + 0x78 * (pawnHandle & 0x1FF));
    if (!pawn) return false;
    
    // Check if alive
    int health = memory::Read<int>(pawn + offsets::entity::m_iHealth);
    if (health <= 0 || health > 100) return false;
    
    data.health = health;
    
    // Team
    data.team = memory::Read<int>(pawn + offsets::entity::m_iTeamNum);
    data.isEnemy = (data.team != g_localTeam);
    
    // Name
    char nameBuffer[128] = {};
    uintptr_t namePtr = memory::Read<uintptr_t>(controller + offsets::entity::m_sSanitizedPlayerName);
    if (namePtr) {
        memory::ReadBuffer(namePtr, nameBuffer, 127);
        data.name = nameBuffer;
    }
    
    // Position from game scene node
    uintptr_t sceneNode = memory::Read<uintptr_t>(pawn + offsets::entity::m_pGameSceneNode);
    if (!sceneNode) return false;
    
    data.position = memory::Read<Vector3>(sceneNode + offsets::scene::m_vecAbsOrigin);
    
    // Head position (approximate - bone offset)
    data.headPos = data.position;
    data.headPos.z += 75.0f; // Head height offset
    
    // Visibility check
    uintptr_t spottedState = pawn + offsets::player::m_entitySpottedState;
    data.isVisible = memory::Read<bool>(spottedState + offsets::player::m_bSpotted);
    
    // Read skeleton bones
    data.bonesValid = false;
    uintptr_t boneMatrix = memory::Read<uintptr_t>(sceneNode + offsets::skeleton::m_modelState + offsets::skeleton::m_boneArray);
    if (boneMatrix) {
        // Read bone positions
        struct BoneData { float x, y, z, w; };
        
        BoneData bones[28];
        if (memory::ReadBuffer(boneMatrix, bones, sizeof(bones))) {
            // Head bone for accurate head position
            data.headPos = {bones[offsets::bones::head].x, bones[offsets::bones::head].y, bones[offsets::bones::head].z};
            
            // Convert bones to screen
            bool anyValid = false;
            for (int i = 0; i < 28; ++i) {
                Vector3 boneWorld = {bones[i].x, bones[i].y, bones[i].z};
                if (WorldToScreen(boneWorld, data.bones2D[i])) {
                    anyValid = true;
                }
            }
            data.bonesValid = anyValid;
        }
    }
    
    // Screen conversion
    if (!WorldToScreen(data.position, data.screenPos)) return false;
    if (!WorldToScreen(data.headPos, data.screenHead)) return false;
    
    // Distance
    Vector3 localPos = memory::Read<Vector3>(g_clientBase + offsets::client_dll::dwLocalPlayerPawn);
    data.distance = (data.position - localPos).Length() / 100.0f; // To meters
    
    data.valid = true;
    return true;
}

// ============================================
// Update
// ============================================

void Update() {
    if (!g_config.enabled) return;
    
    if (!g_clientBase) {
        g_clientBase = memory::GetModuleBase(xorstr_("client.dll"));
        if (!g_clientBase) return;
    }
    
    // Get screen size
#ifdef HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    g_screenWidth = static_cast<int>(io.DisplaySize.x);
    g_screenHeight = static_cast<int>(io.DisplaySize.y);
#endif
    
    // Read view matrix
    g_viewMatrix = memory::Read<Matrix4x4>(g_clientBase + offsets::client_dll::dwViewMatrix);
    
    // Get local player
    uintptr_t localController = memory::Read<uintptr_t>(g_clientBase + offsets::client_dll::dwLocalPlayerController);
    if (localController) {
        g_localTeam = memory::Read<int>(localController + offsets::entity::m_iTeamNum);
    }
    
    // Get entity list
    uintptr_t entityList = memory::Read<uintptr_t>(g_clientBase + offsets::client_dll::dwEntityList);
    if (!entityList) return;
    
    std::vector<PlayerData> newPlayers;
    newPlayers.reserve(64);
    
    // Iterate through entities
    for (int i = 1; i < 64; ++i) {
        uintptr_t listEntry = memory::Read<uintptr_t>(entityList + (0x8 * (i & 0x7FFF) >> 9) + 0x10);
        if (!listEntry) continue;
        
        uintptr_t controller = memory::Read<uintptr_t>(listEntry + 0x78 * (i & 0x1FF));
        if (!controller) continue;
        
        PlayerData player;
        if (ReadPlayerData(controller, entityList, player)) {
            if (player.isEnemy || !g_config.teamCheck) {
                if (player.distance <= g_config.maxDistance) {
                    newPlayers.push_back(std::move(player));
                }
            }
        }
    }
    
    std::lock_guard<std::mutex> lock(g_dataMutex);
    g_players = std::move(newPlayers);
}

// ============================================
// Drawing Helpers
// ============================================

#ifdef HAS_IMGUI

ImU32 ToImColor(const float* color) {
    return IM_COL32(
        static_cast<int>(color[0] * 255),
        static_cast<int>(color[1] * 255),
        static_cast<int>(color[2] * 255),
        static_cast<int>(color[3] * 255)
    );
}

void DrawBox2D(ImDrawList* draw, const PlayerData& player, ImU32 color) {
    float height = std::abs(player.screenPos.y - player.screenHead.y);
    float width = height * 0.4f;
    
    float x = player.screenHead.x - width / 2;
    float y = player.screenHead.y;
    
    draw->AddRect(
        ImVec2(x, y),
        ImVec2(x + width, y + height),
        color,
        0.0f,
        0,
        g_config.boxThickness
    );
}

void DrawBoxCorner(ImDrawList* draw, const PlayerData& player, ImU32 color) {
    float height = std::abs(player.screenPos.y - player.screenHead.y);
    float width = height * 0.4f;
    
    float x = player.screenHead.x - width / 2;
    float y = player.screenHead.y;
    float cornerLen = height * 0.2f;
    
    // Top Left
    draw->AddLine(ImVec2(x, y), ImVec2(x + cornerLen, y), color, g_config.boxThickness);
    draw->AddLine(ImVec2(x, y), ImVec2(x, y + cornerLen), color, g_config.boxThickness);
    
    // Top Right
    draw->AddLine(ImVec2(x + width, y), ImVec2(x + width - cornerLen, y), color, g_config.boxThickness);
    draw->AddLine(ImVec2(x + width, y), ImVec2(x + width, y + cornerLen), color, g_config.boxThickness);
    
    // Bottom Left
    draw->AddLine(ImVec2(x, y + height), ImVec2(x + cornerLen, y + height), color, g_config.boxThickness);
    draw->AddLine(ImVec2(x, y + height), ImVec2(x, y + height - cornerLen), color, g_config.boxThickness);
    
    // Bottom Right
    draw->AddLine(ImVec2(x + width, y + height), ImVec2(x + width - cornerLen, y + height), color, g_config.boxThickness);
    draw->AddLine(ImVec2(x + width, y + height), ImVec2(x + width, y + height - cornerLen), color, g_config.boxThickness);
}

void DrawHealthBar(ImDrawList* draw, const PlayerData& player) {
    float height = std::abs(player.screenPos.y - player.screenHead.y);
    float width = height * 0.4f;
    
    float x = player.screenHead.x - width / 2 - 5;
    float y = player.screenHead.y;
    
    float healthPct = player.health / 100.0f;
    float barHeight = height * healthPct;
    
    // Background
    draw->AddRectFilled(
        ImVec2(x - 2, y),
        ImVec2(x, y + height),
        IM_COL32(0, 0, 0, 180)
    );
    
    // Health color gradient
    int r = static_cast<int>((1.0f - healthPct) * 255);
    int g = static_cast<int>(healthPct * 255);
    ImU32 healthColor = IM_COL32(r, g, 0, 255);
    
    draw->AddRectFilled(
        ImVec2(x - 2, y + height - barHeight),
        ImVec2(x, y + height),
        healthColor
    );
}

void DrawSkeleton(ImDrawList* draw, const PlayerData& player, ImU32 color) {
    if (!player.bonesValid) return;
    
    auto drawBone = [&](int from, int to) {
        draw->AddLine(
            ImVec2(player.bones2D[from].x, player.bones2D[from].y),
            ImVec2(player.bones2D[to].x, player.bones2D[to].y),
            color,
            1.5f
        );
    };
    
    using namespace offsets::bones;
    
    // Spine
    drawBone(head, neck);
    drawBone(neck, spine_2);
    drawBone(spine_2, spine_1);
    drawBone(spine_1, pelvis);
    
    // Left arm
    drawBone(neck, left_shoulder);
    drawBone(left_shoulder, left_elbow);
    drawBone(left_elbow, left_hand);
    
    // Right arm
    drawBone(neck, right_shoulder);
    drawBone(right_shoulder, right_elbow);
    drawBone(right_elbow, right_hand);
    
    // Left leg
    drawBone(pelvis, left_hip);
    drawBone(left_hip, left_knee);
    drawBone(left_knee, left_foot);
    
    // Right leg
    drawBone(pelvis, right_hip);
    drawBone(right_hip, right_knee);
    drawBone(right_knee, right_foot);
}

#endif // HAS_IMGUI

// ============================================
// Render
// ============================================

void Render() {
#ifdef HAS_IMGUI
    if (!g_config.enabled) return;
    
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
    std::lock_guard<std::mutex> lock(g_dataMutex);
    
    for (const auto& player : g_players) {
        if (!player.valid) continue;
        
        ImU32 color = player.isVisible ? 
            ToImColor(g_config.enemyVisibleColor) : 
            ToImColor(g_config.enemyColor);
        
        // Box ESP
        if (g_config.boxEnabled) {
            if (g_config.boxStyle == 0) {
                DrawBox2D(draw, player, color);
            } else {
                DrawBoxCorner(draw, player, color);
            }
        }
        
        // Health bar
        if (g_config.healthBarEnabled) {
            DrawHealthBar(draw, player);
        }
        
        // Skeleton
        if (g_config.skeletonEnabled) {
            DrawSkeleton(draw, player, ToImColor(g_config.skeletonColor));
        }
        
        // Head dot
        if (g_config.headDotEnabled) {
            draw->AddCircleFilled(
                ImVec2(player.screenHead.x, player.screenHead.y),
                4.0f,
                color
            );
        }
        
        // Snaplines
        if (g_config.snaplineEnabled) {
            draw->AddLine(
                ImVec2(static_cast<float>(g_screenWidth) / 2, static_cast<float>(g_screenHeight)),
                ImVec2(player.screenPos.x, player.screenPos.y),
                IM_COL32(255, 255, 255, 100),
                1.0f
            );
        }
        
        // Text info
        float textY = player.screenHead.y - 15;
        float height = std::abs(player.screenPos.y - player.screenHead.y);
        float width = height * 0.4f;
        float textX = player.screenHead.x - width / 2;
        
        // Name
        if (g_config.nameEnabled && !player.name.empty()) {
            draw->AddText(
                ImVec2(textX, textY),
                IM_COL32(255, 255, 255, 255),
                player.name.c_str()
            );
            textY -= 12;
        }
        
        // Distance
        if (g_config.distanceEnabled) {
            char distText[32];
            snprintf(distText, sizeof(distText), "%.0fm", player.distance);
            draw->AddText(
                ImVec2(textX, player.screenPos.y + 2),
                IM_COL32(255, 255, 255, 200),
                distText
            );
        }
    }
    
    // Draw info
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "[HV] Players: %zu", g_players.size());
    draw->AddText(ImVec2(10, 10), IM_COL32(255, 165, 0, 255), infoText);
#endif
}

// ============================================
// Initialize / Shutdown
// ============================================

void Initialize() {
    g_config.enabled = true;
    g_config.boxEnabled = true;
    g_config.healthBarEnabled = true;
    g_config.nameEnabled = true;
    g_config.distanceEnabled = true;
    g_config.skeletonEnabled = false;
    g_config.headDotEnabled = true;
    g_config.snaplineEnabled = false;
    g_config.teamCheck = true;
    g_config.boxStyle = 1; // Corner box
    g_config.boxThickness = 1.5f;
    g_config.maxDistance = 500.0f;
    
    // Colors
    g_config.enemyColor[0] = 1.0f; g_config.enemyColor[1] = 0.3f; g_config.enemyColor[2] = 0.3f; g_config.enemyColor[3] = 1.0f;
    g_config.enemyVisibleColor[0] = 0.3f; g_config.enemyVisibleColor[1] = 1.0f; g_config.enemyVisibleColor[2] = 0.3f; g_config.enemyVisibleColor[3] = 1.0f;
    g_config.teamColor[0] = 0.3f; g_config.teamColor[1] = 0.5f; g_config.teamColor[2] = 1.0f; g_config.teamColor[3] = 1.0f;
    g_config.skeletonColor[0] = 1.0f; g_config.skeletonColor[1] = 1.0f; g_config.skeletonColor[2] = 1.0f; g_config.skeletonColor[3] = 0.8f;
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    g_players.clear();
}

} // namespace esp
