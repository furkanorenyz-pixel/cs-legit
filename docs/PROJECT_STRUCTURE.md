# 📚 Single-Project - Полная Документация

## 🗂️ Структура Проекта

```
externa-cheat/
├── launcher/                    # GUI Launcher (C++ + ImGui)
├── launcher-server/             # Backend + Website (Node.js + SQLite)
├── externa/                     # External Cheat для CS2
├── hypervisor-cheat/           # Гипервизорный чит (Ring -1)
├── kernel/                      # Kernel driver чит (Ring 0)
├── output/                      # CS2 offsets (автогенерация)
├── .github/workflows/          # CI/CD автоматизация
└── docs/                        # Документация
```

---

## 🚀 Launcher (GUI приложение)

### Расположение
- **Код:** `launcher/src/main.cpp`
- **Конфиг:** `launcher/CMakeLists.txt`
- **Билд:** GitHub Actions (`.github/workflows/launcher.yml`)

### Технологии
- **C++17** + **ImGui** (DirectX 11)
- **WinInet** для HTTP запросов
- **Статическая линковка** (без зависимостей)

### Функции
- ✅ Логин/регистрация через API
- ✅ Активация лицензий
- ✅ Авто-обновление себя и читов
- ✅ Проверка статуса игр
- ✅ Защита (VM/debugger detection)
- ✅ HWID привязка

### Сборка
```bash
cd launcher
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLAUNCHER_VERSION=2.0.X
cmake --build build
```

### Версионирование
- **В CI:** версия передаётся через `-DLAUNCHER_VERSION=${{ env.VERSION }}`
- **Локально:** используется `2.0.50-local`
- **Формат:** `2.0.X` где X = номер билда (GitHub run_number)

---

## 🌐 Launcher Server (Backend + Website)

### Расположение
- **Backend:** `launcher-server/backend/src/`
- **Website:** `launcher-server/public/index.html`
- **Admin Panel:** `launcher-server/admin-panel/index.html`
- **Storage:** `launcher-server/storage/` (бинарники + offsets)

### Технологии
- **Node.js 18+** + **Express**
- **SQLite** (база данных)
- **Nginx** (reverse proxy)
- **Systemd** (автозапуск)

### API Endpoints

#### Публичные
- `GET /` - главная страница (скачать лаунчер)
- `GET /api/games/status` - статус игр и лаунчера
- `GET /api/download/launcher` - скачать лаунчер (без rate limit)
- `POST /api/auth/login` - логин
- `POST /api/auth/register` - регистрация

#### Защищённые (JWT token)
- `GET /api/auth/me` - инфо о пользователе
- `POST /api/auth/activate` - активировать лицензию
- `GET /api/download/:game/external` - скачать чит

#### Админские (JWT + admin role)
- `POST /api/admin/licenses` - генерация ключей
- `POST /api/admin/games/:id/status` - установить статус игры
- `GET /api/admin/users` - список пользователей

#### CI/CD (API key)
- `POST /api/admin/ci/upload` - загрузка билдов с GitHub Actions
- `POST /api/admin/reload` - hot reload сервера

### База Данных (SQLite)

**Таблицы:**
- `users` - пользователи (username, password hash, hwid)
- `licenses` - лицензии (ключ, game_id, expires_at)
- `games` - игры (id, name, latest_version)
- `game_status` - статус игр (operational/updating/maintenance)
- `versions` - история версий читов
- `download_logs` - логи скачиваний

### Запуск на сервере
```bash
cd ~/cs-legit/launcher-server/backend
npm install --production
node src/index.js
```

**Через systemd:**
```bash
systemctl start launcher
systemctl status launcher
journalctl -u launcher -f
```

### Rate Limits
- **Общий API:** 100 req/15min на IP
- **Downloads:** 1000 req/hour
- **Launcher downloads:** без лимита (skip в middleware)

---

## 🎮 Externa (External Cheat для CS2)

### Расположение
- **Код:** `externa/src/main.cpp`
- **Syscalls:** `externa/src/syscall.asm`
- **Билд:** `.github/workflows/build.yml`

### Особенности
- External (читает память извне)
- Syscall для обхода античита
- ESP (визуализация игроков)
- Загружается через лаунчер

### Сборка
```bash
cd externa
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## 🔐 Hypervisor Cheat (Ring -1)

### Расположение
- **Гипервизор:** `hypervisor-cheat/hypervisor/`
- **Internal:** `hypervisor-cheat/internal/`
- **Bootkit:** `hypervisor-cheat/bootkit/`

### Архитектура
- **Ring -1:** Гипервизор (EPT hooking)
- **Ring 0:** Драйвер (коммуникация)
- **Ring 3:** Usermode (ESP, aimbot)

### Безопасность
- VM detection bypass
- Anti-debugging
- Integrity checks

---

## 🛠️ CI/CD (GitHub Actions)

### Workflows

#### `.github/workflows/launcher.yml`
- **Триггер:** изменения в `launcher/`
- **Действия:**
  1. Скачать ImGui
  2. Собрать через CMake (MSVC + Ninja)
  3. Загрузить на сервер через `/api/admin/ci/upload`
- **Версия:** `2.0.${{ github.run_number }}`

#### `.github/workflows/build.yml`
- **Триггер:** изменения в `externa/`
- **Действия:** сборка + загрузка external cheat
- **Версия:** `1.0.${{ github.run_number }}`

#### `.github/workflows/deploy-server.yml`
- **Триггер:** изменения в `launcher-server/`
- **Действия:** вызов `/api/admin/reload` для hot reload

---

## 🔄 Workflow: Обновление Лаунчера

```
1. Разработчик → git push (изменения launcher/)
2. GitHub Actions → компиляция launcher.exe
3. GitHub Actions → POST /api/admin/ci/upload
4. Сервер → шифрует и сохраняет в storage/games/launcher/
5. Сервер → обновляет games.latest_version = 2.0.X
6. Лаунчер → GET /api/games/status → видит новую версию
7. Лаунчер → показывает "Update Required"
8. Лаунчер → скачивает с /api/download/launcher
9. Лаунчер → применяет update.bat → перезапуск
```

---

## 🔄 Workflow: Обновление Чита

```
1. Разработчик → git push (изменения externa/)
2. GitHub Actions → компиляция externa.exe
3. GitHub Actions → POST /api/admin/ci/upload (game_id=cs2)
4. Сервер → шифрует и сохраняет в storage/games/cs2/
5. Сервер → обновляет games.latest_version = 1.0.X
6. Лаунчер → GET /api/auth/me → видит новую версию чита
7. Пользователь → нажимает "LAUNCH"
8. Лаунчер → скачивает с /api/download/cs2/external
9. Лаунчер → запускает cs2_external.exe
```

---

## 🗝️ Система Лицензий

### Типы
- **Lifetime** - бессрочно (`expires_at = NULL`)
- **1 Day** - 1 день
- **1 Week** - 7 дней
- **1 Month** - 30 дней
- **3 Months** - 90 дней
- **1 Year** - 365 дней

### Генерация (Admin Panel)
```javascript
POST /api/admin/licenses
{
  "game_id": "cs2",
  "days": null,  // null = lifetime
  "count": 1
}
```

### Активация (Launcher)
```javascript
POST /api/auth/activate
Authorization: Bearer <token>
{
  "license_key": "CS2-XXXX-XXXX-XXXX"
}
```

### Привязка HWID
- Лицензия привязывается к HWID при первом использовании
- Смена HWID через админку: `DELETE /api/admin/users/:id/hwid`

---

## 📡 Мониторинг Обновлений CS2

### Telegram Monitor
- **Файл:** `launcher-server/backend/src/services/telegramMonitor.js`
- **Канал:** @cstwoupdate
- **Частота:** каждые 5 минут
- **Действие:** при обновлении CS2 → статус игры → "updating"

---

## 🔒 Безопасность

### Launcher
- **HWID:** генерируется из CPU/MB серийников
- **VM Detection:** проверка гипервизоров (VMware, VBox, Hyper-V)
- **Debugger Detection:** IsDebuggerPresent, CheckRemoteDebuggerPresent
- **Integrity:** проверка подписи exe (опционально)

### Server
- **JWT:** токены с истечением (7 дней)
- **Rate Limiting:** защита от DDoS
- **HWID Lock:** один аккаунт = одно устройство
- **Encryption:** AES-256-CBC для бинарников

---

## 🚨 Troubleshooting

### Launcher показывает "v2" вместо "v2.0.X"
**Причина:** CMake не передал `-DLAUNCHER_VERSION`  
**Решение:** проверить `.github/workflows/launcher.yml` строка 66

### Бесконечное обновление
**Причина:** несинхронные версии (launcher != server)  
**Решение:** проверить `games.latest_version` в базе

### Rate limit exceeded
**Причина:** превышен лимит скачиваний (1000/час)  
**Решение:** увеличить в `launcher-server/backend/src/index.js`

### SSH connection closed
**Причина:** fail2ban заблокировал IP  
**Решение:** `fail2ban-client unban <IP>` или `ufw disable`

---

## 📞 Поддержка

- **Telegram:** @single_project
- **GitHub Issues:** https://github.com/gavrikov2044-bot/cs-legit/issues
- **Документация:** `/docs/`

---

**Версия документации:** 1.0  
**Последнее обновление:** 23.12.2025

