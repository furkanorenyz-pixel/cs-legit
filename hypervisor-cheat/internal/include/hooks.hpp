/**
 * HYPERVISOR CHEAT - Internal Hooks
 * VMT Hooking for Present + ImGui
 */

#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace hooks {

// ============================================
// VMT Hook Class (Shadow VMT)
// ============================================

class VMTHook {
public:
    VMTHook() = default;
    ~VMTHook() { Unhook(); }

    bool Init(void* pInterface) {
        if (!pInterface) return false;

        m_ppBase = reinterpret_cast<uintptr_t**>(pInterface);
        m_pOriginalVmt = *m_ppBase;

        // Count methods
        while (m_pOriginalVmt[m_iMethodCount]) {
            m_iMethodCount++;
            if (m_iMethodCount > 1000) break; // Safety
        }

        // Allocate shadow VMT
        m_pShadowVmt = new uintptr_t[m_iMethodCount + 1];
        
        // Copy original VMT
        for (size_t i = 0; i <= m_iMethodCount; i++) {
            m_pShadowVmt[i] = m_pOriginalVmt[i];
        }

        // Replace VMT pointer
        *m_ppBase = m_pShadowVmt;

        return true;
    }

    void Unhook() {
        if (m_ppBase && m_pOriginalVmt) {
            *m_ppBase = m_pOriginalVmt;
        }
        if (m_pShadowVmt) {
            delete[] m_pShadowVmt;
            m_pShadowVmt = nullptr;
        }
        m_ppBase = nullptr;
        m_pOriginalVmt = nullptr;
    }

    template<typename T>
    T GetOriginal(size_t index) {
        return reinterpret_cast<T>(m_pOriginalVmt[index]);
    }

    void Hook(size_t index, void* pHook) {
        if (m_pShadowVmt && index < m_iMethodCount) {
            m_pShadowVmt[index] = reinterpret_cast<uintptr_t>(pHook);
        }
    }

private:
    uintptr_t** m_ppBase = nullptr;
    uintptr_t* m_pOriginalVmt = nullptr;
    uintptr_t* m_pShadowVmt = nullptr;
    size_t m_iMethodCount = 0;
};

// ============================================
// Global Hook State
// ============================================

// Present hook
using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
inline PresentFn oPresent = nullptr;
inline VMTHook g_SwapChainHook;

// ============================================
// Functions
// ============================================

bool Initialize();
void Shutdown();

// Hooked functions
HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

} // namespace hooks

