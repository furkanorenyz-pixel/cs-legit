/**
 * HYPERVISOR CHEAT - Usermode Interface Implementation
 * Communication with Ring -1 hypervisor via VMCALL
 */

#include "../include/hypervisor.hpp"
#include <iostream>
#include <intrin.h>

namespace hv {

// ============================================
// VMCALL Implementation
// ============================================

uint64_t Hypervisor::Vmcall(uint64_t number, uint64_t p1, uint64_t p2, uint64_t p3) {
    // Combine number with magic for verification
    uint64_t raxValue = number | ((uint64_t)VMCALL_MAGIC << 32);
    
    uint64_t result = 0;
    
    // Execute VMCALL
    // RAX = number | magic
    // RBX = param1
    // RCX = param2  
    // RDX = param3
    // Result returned in RAX
    
    __try {
        // VMCALL instruction
        // Will cause VM exit to our hypervisor
        // If no hypervisor present, causes #UD
        
        __asm {
            mov rax, raxValue
            mov rbx, p1
            mov rcx, p2
            mov rdx, p3
            
            ; VMCALL opcode: 0F 01 C1
            db 0x0F, 0x01, 0xC1
            
            mov result, rax
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // VMCALL failed - no hypervisor or not ours
        return VMCALL_ERROR;
    }
    
    return result;
}

// ============================================
// Initialization
// ============================================

bool Hypervisor::Initialize() {
    if (m_active) {
        return true;  // Already initialized
    }
    
    // Ping hypervisor
    uint64_t result = Vmcall(VMCALL_PING, 0, 0, 0);
    
    if (result != 1) {
        std::cerr << "[HV] Hypervisor not responding!" << std::endl;
        return false;
    }
    
    // Get info
    HV_INFO info = {};
    result = Vmcall(VMCALL_GET_INFO, (uint64_t)&info, sizeof(info), 0);
    
    if (result != VMCALL_SUCCESS) {
        std::cerr << "[HV] Failed to get hypervisor info!" << std::endl;
        return false;
    }
    
    // Verify signature
    if (memcmp(info.Signature, HV_SIGNATURE, 8) != 0) {
        std::cerr << "[HV] Invalid hypervisor signature!" << std::endl;
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
    
    uint64_t result = Vmcall(VMCALL_GET_PROCESS, (uint64_t)&req, 0, 0);
    
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
    
    uint64_t result = Vmcall(VMCALL_GET_MODULE, (uint64_t)&req, 0, 0);
    
    if (result != VMCALL_SUCCESS || req.Status != HV_SUCCESS) {
        return std::nullopt;
    }
    
    return req.BaseAddress;
}

uint32_t Hypervisor::FindProcess(const char* processName) {
    if (!m_active) return 0;
    
    HV_PROCESS_REQUEST req = {};
    strncpy_s(req.Name, processName, sizeof(req.Name) - 1);
    
    uint64_t result = Vmcall(VMCALL_FIND_PROCESS, (uint64_t)&req, 0, 0);
    
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
    req.BufferAddress = (uint64_t)buffer;
    req.Size = size;
    
    uint64_t result = Vmcall(VMCALL_READ_VIRTUAL, (uint64_t)&req, 0, 0);
    
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
    
    uint64_t result = Vmcall(VMCALL_READ_GAME_DATA, pid, (uint64_t)data, 0);
    
    return (result == VMCALL_SUCCESS);
}

} // namespace hv

