/**
 * HYPERVISOR CHEAT - CS2 Offsets
 * Auto-updated from cs2-dumper
 * 
 * Last update: 2025-12-21
 * Source: https://github.com/a2x/cs2-dumper
 */

#pragma once

#include <cstdint>

namespace offsets {

// ============================================
// client.dll - Main offsets
// ============================================
namespace client_dll {
    constexpr uintptr_t dwCSGOInput = 0x1E3C150;
    constexpr uintptr_t dwEntityList = 0x1D13CE8;
    constexpr uintptr_t dwGameEntitySystem = 0x1FB89D0;
    constexpr uintptr_t dwGameEntitySystem_highestEntityIndex = 0x20F0;
    constexpr uintptr_t dwGameRules = 0x1E31410;
    constexpr uintptr_t dwGlobalVars = 0x1BE41C0;
    constexpr uintptr_t dwGlowManager = 0x1E2E2B8;
    constexpr uintptr_t dwLocalPlayerController = 0x1E1DC18;
    constexpr uintptr_t dwLocalPlayerPawn = 0x1BEEF28;
    constexpr uintptr_t dwPlantedC4 = 0x1E36BE8;
    constexpr uintptr_t dwPrediction = 0x1BEEE40;
    constexpr uintptr_t dwSensitivity = 0x1E2ED08;
    constexpr uintptr_t dwSensitivity_sensitivity = 0x50;
    constexpr uintptr_t dwViewAngles = 0x1E3C800;
    constexpr uintptr_t dwViewMatrix = 0x1E323D0;
    constexpr uintptr_t dwViewRender = 0x1E32F48;
    constexpr uintptr_t dwWeaponC4 = 0x1DCF190;
}

// ============================================
// engine2.dll
// ============================================
namespace engine2 {
    constexpr uintptr_t dwBuildNumber = 0x5F13E4;
    constexpr uintptr_t dwNetworkGameClient = 0x8EB538;
    constexpr uintptr_t dwNetworkGameClient_clientTickCount = 0x390;
    constexpr uintptr_t dwNetworkGameClient_deltaTick = 0x23C;
    constexpr uintptr_t dwNetworkGameClient_localPlayer = 0xE8;
    constexpr uintptr_t dwNetworkGameClient_maxClients = 0x230;
    constexpr uintptr_t dwNetworkGameClient_serverTickCount = 0x23C;
    constexpr uintptr_t dwNetworkGameClient_signOnState = 0x220;
    constexpr uintptr_t dwWindowHeight = 0x8EF844;
    constexpr uintptr_t dwWindowWidth = 0x8EF840;
}

// ============================================
// C_BaseEntity
// ============================================
namespace entity {
    constexpr uintptr_t m_pGameSceneNode = 0x330;  // CGameSceneNode*
    constexpr uintptr_t m_iHealth = 0x34C;          // int32
    constexpr uintptr_t m_iTeamNum = 0x3EB;         // uint8
    constexpr uintptr_t m_fFlags = 0x3EC;           // uint32
}

// ============================================
// CCSPlayerController
// ============================================
namespace controller {
    constexpr uintptr_t m_sSanitizedPlayerName = 0x850;  // CUtlString
    constexpr uintptr_t m_hPlayerPawn = 0x8FC;           // CHandle
    constexpr uintptr_t m_bPawnIsAlive = 0x904;          // bool
}

// ============================================
// C_BasePlayerPawn
// ============================================
namespace pawn {
    constexpr uintptr_t m_vOldOrigin = 0x15A0;  // Vector
}

// ============================================
// C_CSPlayerPawn (extends C_BasePlayerPawn)
// ============================================
namespace player {
    // EntitySpottedState для разных классов:
    // C_CSPlayerPawn -> 0x1170
    // C_PlantedC4 -> 0x13F0
    constexpr uintptr_t m_entitySpottedState = 0x1170;  // EntitySpottedState_t
    constexpr uintptr_t m_bSpotted = 0x8;               // относительно m_entitySpottedState
}

// ============================================
// CGameSceneNode
// ============================================
namespace scene {
    constexpr uintptr_t m_vecAbsOrigin = 0xD0;  // Vector
}

// ============================================
// CSkeletonInstance
// ============================================
namespace skeleton {
    constexpr uintptr_t m_modelState = 0x190;   // CModelState
    constexpr uintptr_t m_boneArray = 0x80;     // + modelState -> кости
}

// ============================================
// Bone IDs
// ============================================
namespace bones {
    constexpr int head = 6;
    constexpr int neck = 5;
    constexpr int spine_2 = 4;
    constexpr int spine_1 = 3;
    constexpr int spine = 2;
    constexpr int pelvis = 0;
    constexpr int left_shoulder = 8;
    constexpr int left_elbow = 9;
    constexpr int left_hand = 10;
    constexpr int right_shoulder = 13;
    constexpr int right_elbow = 14;
    constexpr int right_hand = 15;
    constexpr int left_hip = 22;
    constexpr int left_knee = 23;
    constexpr int left_foot = 24;
    constexpr int right_hip = 25;
    constexpr int right_knee = 26;
    constexpr int right_foot = 27;
}

} // namespace offsets
