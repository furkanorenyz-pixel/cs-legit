# 🛡️ Система безопасности Hypervisor Cheat

## Обзор

Hypervisor работает на самом низком уровне системы. **Одна ошибка = BSOD или брик.**
Поэтому мы реализовали многоуровневую систему защиты.

---

## Уровни безопасности

```cpp
typedef enum _SAFETY_LEVEL {
    SAFETY_DISABLED = 0,  // ⚠️ ОПАСНО! Без защиты
    SAFETY_MINIMAL = 1,   // Базовые проверки
    SAFETY_NORMAL = 2,    // Стандартная защита (по умолчанию)
    SAFETY_PARANOID = 3,  // Максимальная защита + DRY RUN
} SAFETY_LEVEL;
```

### SAFETY_DISABLED (0)
```
⚠️ КРАЙНЕ ОПАСНО!
- Нет проверок указателей
- Нет бэкапов
- Нет отката
- Работает на реальном железе

Использовать ТОЛЬКО если 100% уверен в коде!
```

### SAFETY_MINIMAL (1)
```
- Базовая проверка указателей
- Автоматический откат при ошибках
- Логирование
- Работает на реальном железе
```

### SAFETY_NORMAL (2) — По умолчанию
```
✅ РЕКОМЕНДУЕТСЯ
- Полная проверка указателей
- Автоматический откат
- Работает ТОЛЬКО в VM
- Полное логирование
```

### SAFETY_PARANOID (3)
```
🔒 МАКСИМАЛЬНАЯ ЗАЩИТА
- Всё от NORMAL
- DRY RUN: код НЕ выполняется, только логируется
- Для отладки и проверки логики
```

---

## Защитные механизмы

### 1. VM Detection

```cpp
// Проверяем CPUID на наличие гипервизора
u32 SafetyDetectVM(void) {
    u32 eax, ebx, ecx, edx;
    CpuidEx(1, 0, &eax, &ebx, &ecx, &edx);
    
    // Bit 31 = Hypervisor Present
    if (ecx & (1 << 31)) {
        return 1;  // Мы в VM
    }
    return 0;  // Реальное железо
}
```

Поддерживаемые VM:
- VMware
- VirtualBox  
- KVM/QEMU
- Hyper-V
- Xen

**Если не VM и включен `VmOnlyMode` → Hypervisor НЕ запустится!**

### 2. Pointer Validation

```cpp
u32 SafetyValidatePointer(void* ptr, u64 size) {
    if (ptr == NULL) return 0;
    
    u64 addr = (u64)ptr;
    
    // Null page protection
    if (addr < 0x1000) return 0;
    
    // Non-canonical address check (x64)
    if (addr > 0x00007FFFFFFFFFFF && 
        addr < 0xFFFF800000000000) return 0;
    
    // Overflow check
    if (addr + size < addr) return 0;
    
    return 1;
}
```

### 3. Backup System

Перед КАЖДОЙ модификацией памяти создаётся бэкап:

```cpp
typedef struct _SAFETY_BACKUP {
    void* Address;           // Адрес модификации
    u8 OriginalData[256];    // Оригинальные байты
    u8 NewData[256];         // Новые байты
    u64 Size;                // Размер
    u32 Applied;             // Применено ли
} SAFETY_BACKUP;

// Создание бэкапа
HV_STATUS SafetyCreateBackup(void* address, u64 size);

// Откат одного изменения
HV_STATUS SafetyRollbackOne(u32 index);

// Откат ВСЕХ изменений
HV_STATUS SafetyRollbackAll(void);
```

### 4. Automatic Rollback

При ЛЮБОЙ ошибке:

```cpp
void SafetyReportError(HV_STATUS error, const char* message) {
    gSafetyState.ErrorCount++;
    
    if (gSafetyState.ErrorCount >= gSafetyState.Config.MaxRetries) {
        // Слишком много ошибок - откатываем ВСЁ!
        SafetyRollbackAll();
        gSafetyState.SafetyTriggered = 1;
    }
}
```

### 5. Safe Operations

Все операции с памятью через безопасные обёртки:

```cpp
// Вместо memcpy
HV_STATUS SafeMemoryCopy(void* dest, const void* src, u64 size);

// Вместо memset
HV_STATUS SafeMemorySet(void* dest, u8 value, u64 size);

// С валидацией
HV_STATUS SafePatch(void* address, void* data, u64 size) {
    // 1. Валидация указателей
    if (!SafetyValidatePointer(address, size)) {
        return HV_ERROR_INVALID_ADDRESS;
    }
    
    // 2. Создание бэкапа
    SafetyCreateBackup(address, size);
    
    // 3. Dry run check
    if (gSafetyState.Config.DryRunMode) {
        LOG_INFO("DRY RUN: Would patch %p", address);
        return HV_SUCCESS;
    }
    
    // 4. Применение патча
    SafeMemoryCopy(address, data, size);
    
    // 5. Верификация
    if (SafeMemoryCompare(address, data, size) != 0) {
        SafetyRollbackAll();
        return HV_ERROR_PATCH_FAILED;
    }
    
    return HV_SUCCESS;
}
```

---

## Error Codes

| Код | Название | Описание |
|-----|----------|----------|
| 0 | HV_SUCCESS | Успех |
| 1 | HV_ERROR_NOT_SUPPORTED | CPU не поддерживает VT-x |
| 2 | HV_ERROR_ALREADY_RUNNING | Hypervisor уже запущен |
| 3 | HV_ERROR_VMX_DISABLED | VT-x отключен в BIOS |
| 4 | HV_ERROR_VMXON_FAILED | VMXON не удался |
| 5 | HV_ERROR_VMCS_FAILED | Ошибка настройки VMCS |
| 6 | HV_ERROR_EPT_FAILED | Ошибка EPT |
| 10 | HV_ERROR_INVALID_VMCALL | Неизвестный VMCALL |
| 11 | HV_ERROR_INVALID_PARAM | Неверный параметр |
| 12 | HV_ERROR_INVALID_ADDRESS | Неверный адрес |
| 20 | HV_ERROR_NOT_VM | Не виртуальная машина |
| 21 | HV_ERROR_SAFETY_TRIGGERED | Сработала защита |

---

## Что происходит при ошибке

```
┌─────────────────────────────────────────────────────────────────────┐
│  Ошибка в коде hypervisor                                           │
└─────────────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────────────┐
│  SafetyReportError() вызывается                                     │
│  └─ Увеличиваем счётчик ошибок                                      │
└─────────────────────────────────────────────────────────────────────┘
                                ↓
                    ┌───────────────────────┐
                    │  ErrorCount >= Max?   │
                    └───────────────────────┘
                           │
              ┌────────────┴────────────┐
              ↓ НЕТ                     ↓ ДА
┌──────────────────────┐    ┌───────────────────────────────────────┐
│  Продолжаем работу   │    │  SafetyRollbackAll()                  │
│  (retry возможен)    │    │  └─ Восстанавливаем все бэкапы        │
└──────────────────────┘    │  └─ SafetyTriggered = 1               │
                            │  └─ Hypervisor выключается безопасно  │
                            └───────────────────────────────────────┘
                                            ↓
                            ┌───────────────────────────────────────┐
                            │  Windows продолжает работать НОРМАЛЬНО│
                            │  Никакого BSOD, никакого брика        │
                            └───────────────────────────────────────┘
```

---

## Тестирование

### Шаг 1: PARANOID mode в VM

```cpp
SafetyInit(SAFETY_PARANOID);

// DRY RUN: ничего не модифицируется
// Только логи - проверяем логику
```

### Шаг 2: NORMAL mode в VM

```cpp
SafetyInit(SAFETY_NORMAL);

// Реальные модификации, но:
// - Только в VM
// - С бэкапами
// - С автооткатом
```

### Шаг 3: Реальное железо (если ОЧЕНЬ нужно)

```cpp
SafetyInit(SAFETY_MINIMAL);

// ⚠️ ОСТОРОЖНО!
// - Работает на реальном железе
// - Бэкапы и откат всё ещё работают
// - Но нет защиты VM-only
```

---

## Рекомендации

1. **ВСЕГДА** тестируй в VM сначала
2. **НИКОГДА** не используй SAFETY_DISABLED на реальном железе
3. Сделай снапшот VM перед тестированием
4. Имей SPI programmer под рукой (для bootkit)
5. Логи - твой лучший друг

---

## FAQ

**Q: Что если BSOD всё равно произошёл?**
A: Система откатит изменения при следующей загрузке (если bootkit не сломан).

**Q: Как отключить VM-only режим?**
A: `SafetyInit(SAFETY_MINIMAL)` или `SAFETY_DISABLED`. НО ЭТО ОПАСНО!

**Q: Максимум бэкапов?**
A: 32 бэкапа по 256 байт каждый. Достаточно для любых патчей.

**Q: Что если hypervisor зависнет?**
A: Triple fault → система перезагрузится. Данные могут потеряться, но железо будет цело.

