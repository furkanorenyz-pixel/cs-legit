/**
 * HYPERVISOR CHEAT - ESP Header
 * Configuration and rendering for ESP overlay
 */

#pragma once

namespace esp {

// ============================================
// Configuration
// ============================================

struct Config {
    // Master toggle
    bool enabled = true;
    
    // Features
    bool boxEnabled = true;
    bool healthBarEnabled = true;
    bool nameEnabled = true;
    bool distanceEnabled = true;
    bool skeletonEnabled = false;
    bool headDotEnabled = true;
    bool snaplineEnabled = false;
    bool teamCheck = true;
    
    // Box settings
    int boxStyle = 1;  // 0 = 2D, 1 = Corner
    float boxThickness = 1.5f;
    
    // Range
    float maxDistance = 500.0f;
    
    // Colors (RGBA)
    float enemyColor[4] = {1.0f, 0.3f, 0.3f, 1.0f};
    float enemyVisibleColor[4] = {0.3f, 1.0f, 0.3f, 1.0f};
    float teamColor[4] = {0.3f, 0.5f, 1.0f, 1.0f};
    float skeletonColor[4] = {1.0f, 1.0f, 1.0f, 0.8f};
};

// Global config
extern Config g_config;

// ============================================
// Functions
// ============================================

void Initialize();
void Update();
void Render();
void Shutdown();

} // namespace esp
