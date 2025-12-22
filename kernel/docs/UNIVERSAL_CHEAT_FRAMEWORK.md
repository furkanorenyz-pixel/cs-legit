# 🎮 UNIVERSAL KERNEL CHEAT FRAMEWORK

Универсальная архитектура для создания читов любой игры с защитой Ring -3.

---

## 📊 Полная схема

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                           BOOT TIME (Ring -3)                                  ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │  UEFI Bootkit (KernelBypassBootkit.efi)                             │    ║
║   │  ├─ Хук ExitBootServices                                            │    ║
║   │  ├─ Патч DSE (Driver Signature Enforcement)                         │    ║
║   │  ├─ Патч PatchGuard (KPP)                                           │    ║
║   │  └─ Инжект kernel payload                                           │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                    ↓                                          ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │  Windows Boot (DSE DISABLED)                                        │    ║
║   │  └─ Можно загружать НЕПОДПИСАННЫЕ драйверы                          │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
                                     ↓
╔═══════════════════════════════════════════════════════════════════════════════╗
║                          KERNEL MODE (Ring 0)                                  ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │  Kernel Driver (GameBypass.sys)                                     │    ║
║   │  ├─ Memory Operations                                               │    ║
║   │  │   ├─ MmCopyVirtualMemory() - читаем память процесса             │    ║
║   │  │   ├─ Physical Memory Read - через CR3                           │    ║
║   │  │   └─ MDL Mapping - маппим страницы напрямую                      │    ║
║   │  │                                                                  │    ║
║   │  ├─ Process Operations                                              │    ║
║   │  │   ├─ Hide Process (DKOM)                                         │    ║
║   │  │   ├─ Protect Process (ObRegisterCallbacks)                       │    ║
║   │  │   └─ Elevate Handle                                              │    ║
║   │  │                                                                  │    ║
║   │  ├─ Anti-Cheat Bypass                                               │    ║
║   │  │   ├─ Remove Callbacks (PsSetCreateProcessNotifyRoutine)          │    ║
║   │  │   ├─ Hide Driver (unlink from PsLoadedModuleList)                │    ║
║   │  │   └─ Spoof HWID                                                  │    ║
║   │  │                                                                  │    ║
║   │  └─ Communication                                                   │    ║
║   │       └─ IOCTL Interface для usermode                               │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
                                     ↓
╔═══════════════════════════════════════════════════════════════════════════════╗
║                          USER MODE (Ring 3)                                    ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │  Cheat Application                                                  │    ║
║   │  ├─ Kernel Interface                                                │    ║
║   │  │   └─ DeviceIoControl() → драйвер                                 │    ║
║   │  │                                                                  │    ║
║   │  ├─ Game SDK                                                        │    ║
║   │  │   ├─ Offsets (auto-update)                                       │    ║
║   │  │   ├─ Structs (Entity, Player, Weapon)                            │    ║
║   │  │   └─ Pattern Scanner                                             │    ║
║   │  │                                                                  │    ║
║   │  ├─ Features                                                        │    ║
║   │  │   ├─ ESP (Visual)                                                │    ║
║   │  │   ├─ Aimbot                                                      │    ║
║   │  │   └─ Misc                                                        │    ║
║   │  │                                                                  │    ║
║   │  └─ Render                                                          │    ║
║   │       ├─ DirectX Overlay                                            │    ║
║   │       └─ ImGui Menu                                                 │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

---

## 🗂️ Структура проекта (Universal)

```
kernel-cheat-framework/
├── bootkit/                        # Ring -3 UEFI
│   ├── src/
│   │   ├── bootkit.c              # DXE driver
│   │   ├── patches.c              # DSE/PG patches
│   │   └── safety.c               # VM detection, rollback
│   └── KernelBypassPkg.dsc
│
├── driver/                         # Ring 0 Kernel
│   ├── src/
│   │   ├── main.c                 # DriverEntry, IOCTL dispatch
│   │   ├── memory.c               # Memory read/write
│   │   ├── process.c              # Process manipulation
│   │   ├── callbacks.c            # AC callback removal
│   │   └── hwid.c                 # HWID spoof
│   ├── include/
│   │   └── ioctl.h                # IOCTL codes
│   └── driver.vcxproj
│
├── usermode/                       # Ring 3 Library
│   ├── include/
│   │   └── kernel_interface.hpp   # C++ API
│   └── src/
│       └── interface.cpp
│
├── games/                          # Game-specific modules
│   ├── cs2/
│   │   ├── sdk/
│   │   │   ├── offsets.hpp        # Auto-updated offsets
│   │   │   ├── entity.hpp         # Entity structs
│   │   │   ├── player.hpp         # Player structs
│   │   │   └── patterns.hpp       # Signature patterns
│   │   ├── features/
│   │   │   ├── esp.cpp
│   │   │   ├── aimbot.cpp
│   │   │   └── misc.cpp
│   │   └── cs2_cheat.cpp          # Main
│   │
│   ├── valorant/                   # Same structure
│   ├── apex/
│   ├── fortnite/
│   └── rust/
│
├── render/                         # Universal render
│   ├── overlay.cpp
│   ├── menu.cpp                   # ImGui
│   └── draw.cpp                   # Primitives
│
├── common/                         # Shared code
│   ├── math.hpp
│   ├── config.hpp
│   └── utils.hpp
│
└── tools/
    ├── offset_dumper/             # Auto offset tool
    ├── driver_loader/             # Load unsigned driver
    └── hwid_changer/              # HWID spoof tool
```

---

## 🎯 Добавление новой игры

### Шаг 1: Создать SDK
```cpp
// games/newgame/sdk/offsets.hpp
namespace offsets {
    // Найти через ReClass/Cheat Engine
    constexpr uintptr_t dwEntityList = 0x...;
    constexpr uintptr_t dwLocalPlayer = 0x...;
    constexpr uintptr_t dwViewMatrix = 0x...;
}

// games/newgame/sdk/entity.hpp
struct Entity {
    Vector3 position;      // offset: 0x...
    int health;            // offset: 0x...
    int team;              // offset: 0x...
};
```

### Шаг 2: Добавить Pattern Scanner
```cpp
// games/newgame/sdk/patterns.hpp
namespace patterns {
    // Паттерны которые не меняются между версиями
    constexpr auto EntityList = "48 8B 0D ?? ?? ?? ?? 48 85 C9 74";
    constexpr auto LocalPlayer = "48 89 05 ?? ?? ?? ?? 48 85 C0";
    constexpr auto ViewMatrix = "48 8D 0D ?? ?? ?? ?? 48 C1 E0 06";
}
```

### Шаг 3: Реализовать Features
```cpp
// games/newgame/features/esp.cpp
class NewGameESP : public BaseESP {
public:
    void Render() override {
        for (auto& entity : GetEntities()) {
            if (!entity.IsValid()) continue;
            if (entity.IsTeammate()) continue;
            
            Vector2 screen;
            if (WorldToScreen(entity.position, screen)) {
                DrawBox(screen, entity.IsVisible() ? Green : Red);
                DrawHealth(screen, entity.health);
                DrawName(screen, entity.name);
            }
        }
    }
};
```

### Шаг 4: Зарегистрировать в меню
```cpp
// games/newgame/newgame_cheat.cpp
void RegisterFeatures() {
    Menu::AddTab("ESP", []() {
        ImGui::Checkbox("Enable", &config.esp.enabled);
        ImGui::Checkbox("Box", &config.esp.box);
        ImGui::Checkbox("Health", &config.esp.health);
        ImGui::Checkbox("Name", &config.esp.name);
    });
    
    Menu::AddTab("Aimbot", []() {
        ImGui::Checkbox("Enable", &config.aim.enabled);
        ImGui::SliderFloat("Smooth", &config.aim.smooth, 1.0f, 20.0f);
        ImGui::Combo("Bone", &config.aim.bone, "Head\0Chest\0Pelvis\0");
    });
}
```

---

## 🔄 Auto Offset System

```cpp
// common/offset_manager.hpp
class OffsetManager {
public:
    bool Initialize(const char* gameName) {
        // 1. Try fetch from cloud
        if (FetchFromCloud(gameName)) {
            Log("✓ Offsets loaded from cloud");
            return true;
        }
        
        // 2. Fallback to pattern scan
        if (ScanPatterns(gameName)) {
            Log("✓ Offsets found via patterns");
            return true;
        }
        
        // 3. Use cached
        Log("⚠ Using cached offsets");
        return LoadCached(gameName);
    }
    
private:
    bool FetchFromCloud(const char* game) {
        // GitHub raw JSON
        std::string url = fmt::format(
            "https://raw.githubusercontent.com/{}/offsets/{}.json",
            REPO_NAME, game
        );
        
        auto json = HttpGet(url);
        if (json.empty()) return false;
        
        ParseOffsets(json);
        SaveCache(game, json);
        return true;
    }
    
    bool ScanPatterns(const char* game) {
        auto patterns = GetPatternsForGame(game);
        
        for (auto& [name, pattern, offset] : patterns) {
            uintptr_t addr = PatternScan(pattern);
            if (addr) {
                SetOffset(name, ResolveRIP(addr, offset));
            }
        }
        
        return ValidateOffsets();
    }
};
```

---

## 🛡️ Anti-Detection Features

### Kernel Driver
```cpp
// Скрываем драйвер от MmGetSystemRoutineAddress
void HideDriver(PDRIVER_OBJECT driver) {
    // Unlink from PsLoadedModuleList
    PKLDR_DATA_TABLE_ENTRY entry = (PKLDR_DATA_TABLE_ENTRY)driver->DriverSection;
    
    PLIST_ENTRY prev = entry->InLoadOrderLinks.Blink;
    PLIST_ENTRY next = entry->InLoadOrderLinks.Flink;
    
    prev->Flink = next;
    next->Blink = prev;
    
    // Clear entry
    entry->InLoadOrderLinks.Flink = entry;
    entry->InLoadOrderLinks.Blink = entry;
}

// Удаляем callbacks анти-чита
void RemoveAntiCheatCallbacks() {
    // PsSetCreateProcessNotifyRoutine callbacks
    // PsSetLoadImageNotifyRoutine callbacks
    // ObRegisterCallbacks
    // CmRegisterCallback (registry)
}
```

### Usermode
```cpp
// Скрываем overlay окно
void HideOverlayWindow(HWND hwnd) {
    // Remove from EnumWindows
    SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    
    // Random window title
    SetWindowText(hwnd, GenerateRandomTitle());
    
    // Make click-through
    SetWindowLong(hwnd, GWL_EXSTYLE, 
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
}
```

---

## 🎮 Поддерживаемые игры (Template)

| Игра | Anti-Cheat | Сложность | Статус |
|------|------------|-----------|--------|
| CS2 | VAC | ⭐⭐ | ✅ Ready |
| Valorant | Vanguard | ⭐⭐⭐⭐⭐ | 🔧 WIP |
| Apex Legends | EAC | ⭐⭐⭐ | 📋 Planned |
| Fortnite | EAC | ⭐⭐⭐ | 📋 Planned |
| PUBG | BattlEye | ⭐⭐⭐⭐ | 📋 Planned |
| Rust | EAC | ⭐⭐⭐ | 📋 Planned |
| Escape from Tarkov | BattlEye | ⭐⭐⭐⭐ | 📋 Planned |
| Rainbow Six | BattlEye | ⭐⭐⭐⭐ | 📋 Planned |

---

## 🚀 Quick Start для новой игры

```bash
# 1. Скопировать template
cp -r games/template games/newgame

# 2. Заполнить offsets
# Использовать ReClass64, Cheat Engine, IDA Pro

# 3. Найти patterns
# Использовать x64dbg, IDA Pro

# 4. Реализовать features
# ESP → Aimbot → Misc

# 5. Тестировать
# Всегда в VM сначала!

# 6. Добавить в CI/CD
# .github/workflows/newgame.yml
```

---

## ⚠️ Важно

1. **Всегда тестируй в VM** — Bootkit может сломать систему
2. **Kernel driver** — один баг = BSOD
3. **Anti-cheat updates** — паттерны ломаются, нужно обновлять
4. **Legal** — это образовательный проект

---

## 📚 Ресурсы

- [UnknownCheats](https://unknowncheats.me) — форум
- [GuidedHacking](https://guidedhacking.com) — туториалы
- [cs2-dumper](https://github.com/a2x/cs2-dumper) — оффсеты CS2
- [ReClass.NET](https://github.com/ReClassNET) — reverse engineering
- [x64dbg](https://x64dbg.com) — отладчик


