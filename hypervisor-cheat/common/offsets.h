/**
 * HYPERVISOR CHEAT - CS2 Offsets
 * Auto-updated from cs2-dumper
 * 
 * Last update: 2024-12-22
 * Game version: CS2
 */

#pragma once

namespace offsets {

// ============================================
// client.dll
// ============================================
namespace client {
    constexpr auto dwEntityList = 0x1A146C8;
    constexpr auto dwLocalPlayerController = 0x1A6ADF8;
    constexpr auto dwLocalPlayerPawn = 0x1890CC8;
    constexpr auto dwViewMatrix = 0x1A81350;
    constexpr auto dwViewAngles = 0x1A8C4E0;
    constexpr auto dwGlobalVars = 0x1890998;
}

// ============================================
// C_BaseEntity
// ============================================
namespace entity {
    constexpr auto m_iHealth = 0x344;
    constexpr auto m_iTeamNum = 0x3E3;
    constexpr auto m_pGameSceneNode = 0x328;
    constexpr auto m_fFlags = 0x3EC;
    constexpr auto m_vecAbsVelocity = 0x3F0;
}

// ============================================
// C_BasePlayerPawn
// ============================================
namespace pawn {
    constexpr auto m_vOldOrigin = 0x1324;
    constexpr auto m_iIDEntIndex = 0x15B4;
}

// ============================================
// CCSPlayerController
// ============================================
namespace controller {
    constexpr auto m_hPlayerPawn = 0x80C;
    constexpr auto m_sSanitizedPlayerName = 0x768;
    constexpr auto m_bPawnIsAlive = 0x814;
}

// ============================================
// C_CSPlayerPawnBase
// ============================================
namespace player {
    constexpr auto m_ArmorValue = 0x1648;
    constexpr auto m_flFlashDuration = 0x1454;
    constexpr auto m_flFlashBangTime = 0x1458;
    constexpr auto m_iShotsFired = 0x1488;
    constexpr auto m_aimPunchAngle = 0x17A0;
    constexpr auto m_aimPunchAngleVel = 0x17AC;
    constexpr auto m_bGunGameImmunity = 0x13EC;
    constexpr auto m_fMolotovDamageTime = 0x13F0;
    constexpr auto m_entitySpottedState = 0x23B0;
    constexpr auto m_bSpotted = 0x8; // + entitySpottedState
}

// ============================================
// CGameSceneNode
// ============================================
namespace scene {
    constexpr auto m_vecAbsOrigin = 0xD0;
    constexpr auto m_angAbsRotation = 0xDC;
}

// ============================================
// CSkeletonInstance
// ============================================
namespace skeleton {
    constexpr auto m_modelState = 0x170;
    constexpr auto m_boneArray = 0x80; // + modelState
}

// ============================================
// Bone IDs
// ============================================
namespace bones {
    constexpr auto head = 6;
    constexpr auto neck = 5;
    constexpr auto spine = 4;
    constexpr auto pelvis = 0;
    constexpr auto left_shoulder = 8;
    constexpr auto left_elbow = 9;
    constexpr auto left_hand = 10;
    constexpr auto right_shoulder = 13;
    constexpr auto right_elbow = 14;
    constexpr auto right_hand = 15;
    constexpr auto left_hip = 22;
    constexpr auto left_knee = 23;
    constexpr auto left_foot = 24;
    constexpr auto right_hip = 25;
    constexpr auto right_knee = 26;
    constexpr auto right_foot = 27;
}

} // namespace offsets

