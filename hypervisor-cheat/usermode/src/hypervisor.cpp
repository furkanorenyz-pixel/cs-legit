/**
 * HYPERVISOR CHEAT - Usermode Interface Implementation
 * Communication with Ring -1 hypervisor via VMCALL
 */

#include "../include/hypervisor.hpp"
#include <iostream>
#include <cstring>

namespace hv {

// ============================================
// VMCALL Implementation (uses vmcall.asm)
// ============================================

uint64_t Hypervisor::Vmcall(uint64_t number, uint64_t p1, uint64_t p2, uint64_t p3) {
    __try {
        // DoVmcall is implemented in vmcall.asm
        // It handles the VMCALL instruction and parameter setup
        return DoVmcall(number, p1, p2, p3);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // VMCALL failed - no hypervisor or not ours
        // This happens if:
        // - No hypervisor is running
        // - The hypervisor doesn't handle our magic number
        // - VMCALL caused #UD (undefined opcode) - shouldn't happen on VT-x capable CPU
        return VMCALL_ERROR;
    }
}

// ============================================
// Initialization
// ============================================

bool Hypervisor::Initialize() {
    if (m_active) {
        return true;  // Already initialized
    }
    
    std::cout << "[HV] Pinging hypervisor..." << std::endl;
    
    // Ping hypervisor
    uint64_t result = Vmcall(VMCALL_PING, 0, 0, 0);
    
    if (result != 1) {
        std::cout << "[HV] Hypervisor not responding (result=" << result << ")" << std::endl;
        return false;
    }
    
    std::cout << "[HV] Hypervisor responded!" << std::endl;
    
    // Get info
    HV_INFO info = {};
    result = Vmcall(VMCALL_GET_INFO, reinterpret_cast<uint64_t>(&info), sizeof(info), 0);
    
    if (result != VMCALL_SUCCESS) {
        std::cout << "[HV] Failed to get hypervisor info (result=" << result << ")" << std::endl;
        return false;
    }
    
    // Verify signature
    if (memcmp(info.Signature, HV_SIGNATURE, 7) != 0) {
        std::cout << "[HV] Invalid hypervisor signature!" << std::endl;
        return false;
    }
    
    m_info = info;
    m_active = true;
    
    std::cout << "[HV] Connected to hypervisor!" << std::endl;
    std::cout << "[HV] Version: " << (info.Version >> 16) << "." 
              << ((info.Version >> 8) & 0xFF) << "." 
              << (info.Version & 0xFF) << std::endl;
    
    return true;
}

void Hypervisor::Shutdown() {
    m_active = false;
}

// ============================================
// Process Operations
// ============================================

std::optional<uint64_t> Hypervisor::GetProcessCr3(uint32_t pid) {
    if (!m_active) return std::nullopt;
    
    HV_PROCESS_REQUEST req = {};
    req.ProcessId = pid;
    
    uint64_t result = Vmcall(VMCALL_GET_PROCESS, reinterpret_cast<uint64_t>(&req), 0, 0);
    
    if (result != VMCALL_SUCCESS || req.Status != HV_SUCCESS) {
        return std::nullopt;
    }
    
    return req.Cr3;
}

std::optional<uint64_t> Hypervisor::GetModuleBase(uint32_t pid, const char* moduleName) {
    if (!m_active) return std::nullopt;
    
    HV_MODULE_REQUEST req = {};
    req.ProcessId = pid;
    strncpy_s(req.ModuleName, moduleName, sizeof(req.ModuleName) - 1);
    
    uint64_t result = Vmcall(VMCALL_GET_MODULE, reinterpret_cast<uint64_t>(&req), 0, 0);
    
    if (result != VMCALL_SUCCESS || req.Status != HV_SUCCESS) {
        return std::nullopt;
    }
    
    return req.BaseAddress;
}

uint32_t Hypervisor::FindProcess(const char* processName) {
    if (!m_active) return 0;
    
    HV_PROCESS_REQUEST req = {};
    strncpy_s(req.Name, processName, sizeof(req.Name) - 1);
    
    uint64_t result = Vmcall(VMCALL_FIND_PROCESS, reinterpret_cast<uint64_t>(&req), 0, 0);
    
    if (result != VMCALL_SUCCESS || req.Status != HV_SUCCESS) {
        return 0;
    }
    
    return req.ProcessId;
}

// ============================================
// Memory Operations
// ============================================

bool Hypervisor::ReadMemory(uint64_t cr3, uint64_t address, void* buffer, size_t size) {
    if (!m_active) return false;
    if (!buffer || size == 0) return false;
    
    HV_READ_REQUEST req = {};
    req.ProcessCr3 = cr3;
    req.VirtualAddress = address;
    req.BufferAddress = reinterpret_cast<uint64_t>(buffer);
    req.Size = size;
    
    uint64_t result = Vmcall(VMCALL_READ_VIRTUAL, reinterpret_cast<uint64_t>(&req), 0, 0);
    
    return (result == VMCALL_SUCCESS && req.Status == HV_SUCCESS);
}

bool Hypervisor::WriteMemory(uint64_t cr3, uint64_t address, const void* buffer, size_t size) {
    if (!m_active) return false;
    if (!buffer || size == 0) return false;
    
    // For safety, we don't implement write in the basic version
    // Could be added later for features like aim assist
    
    return false;
}

// ============================================
// Game-Specific
// ============================================

bool Hypervisor::ReadGameData(uint32_t pid, GAME_DATA* data) {
    if (!m_active) return false;
    if (!data) return false;
    
    uint64_t result = Vmcall(VMCALL_READ_GAME_DATA, pid, reinterpret_cast<uint64_t>(data), 0);
    
    return (result == VMCALL_SUCCESS);
}

} // namespace hv
