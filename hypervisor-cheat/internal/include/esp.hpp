/**
 * HYPERVISOR CHEAT - Internal ESP
 * Direct memory access ESP (no VMCALL needed - we're inside!)
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>

#include "../../common/offsets.h"

namespace esp {

// ============================================
// Config
// ============================================

struct Config {
    // Master
    bool enabled = true;
    
    // Features
    bool boxEnabled = true;
    bool healthBarEnabled = true;
    bool nameEnabled = true;
    bool distanceEnabled = true;
    bool skeletonEnabled = false;
    bool headDotEnabled = false;
    bool snaplineEnabled = false;
    
    // Style
    int boxStyle = 0;  // 0 = 2D, 1 = Corner
    float boxThickness = 1.5f;
    float maxDistance = 500.0f;
    
    // Colors (ImVec4 compatible)
    float enemyColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float enemyVisibleColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
    float teamColor[4] = {0.0f, 0.5f, 1.0f, 1.0f};
    float healthColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float nameColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float skeletonColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

extern Config g_config;

// ============================================
// Structures
// ============================================

struct Vector3 {
    float x, y, z;
    
    Vector3 operator-(const Vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    
    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }
};

struct Vector2 {
    float x, y;
};

struct PlayerInfo {
    int index;
    int health;
    int team;
    bool alive;
    bool visible;
    bool enemy;
    
    Vector3 position;
    Vector3 headPosition;
    Vector3 bones[28];  // All bones
    
    std::string name;
    float distance;
    
    // Screen
    Vector2 screenPos;
    Vector2 screenHead;
    float screenHeight;
    float screenWidth;
    bool onScreen;
};

// ============================================
// Main Functions
// ============================================

void Initialize();
void Shutdown();

// Called every frame before rendering
void Update();

// Called to render ESP (from Present hook)
void Render();

// ============================================
// Internal Memory Reading (direct - no VMCALL!)
// ============================================

// Direct memory read - we're inside the process!
template<typename T>
inline T Read(uintptr_t address) {
    if (!address) return T{};
    __try {
        return *reinterpret_cast<T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

template<typename T>
inline void Write(uintptr_t address, const T& value) {
    if (!address) return;
    __try {
        *reinterpret_cast<T*>(address) = value;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// Module base
uintptr_t GetClientBase();
uintptr_t GetEngineBase();

} // namespace esp

