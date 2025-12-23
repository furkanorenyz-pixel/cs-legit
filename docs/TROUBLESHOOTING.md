# 🔍 Диагностика и Решение Проблем

## 🚨 Launcher: "v2" вместо "v2.0.X"

### Причина
CMake не передаёт `LAUNCHER_VERSION` правильно или в коде есть `#define` после `#ifndef`.

### Проверка
1. Посмотреть лог сборки GitHub Actions
2. Найти строку: `-- Launcher version: X.X.X`
3. Если версия не `2.0.X`, проблема в workflow

### Решение
Проверить `.github/workflows/launcher.yml` строка 66:
```yaml
cmake ... -DLAUNCHER_VERSION=${{ env.VERSION }}
```

Проверить `launcher/CMakeLists.txt`:
```cmake
message(STATUS "Launcher version: ${LAUNCHER_VERSION}")
target_compile_definitions(launcher PRIVATE LAUNCHER_VERSION=\"${LAUNCHER_VERSION}\")
```

Проверить `launcher/src/main.cpp`:
```cpp
#ifndef LAUNCHER_VERSION
    #define LAUNCHER_VERSION "2.0.50-local"
#endif
```

**Порядок важен:** `#ifndef` **должен быть ДО** любых `#define`.

---

## 🔄 Бесконечное "Update Required"

### Причина
Версия в exe != версии на сервере, или сервер отдаёт старый файл.

### Диагностика
```bash
# 1. Проверить версию на сервере
curl http://single-project.duckdns.org/api/games/status | jq .games.launcher.version

# 2. Проверить, что лежит в storage
ssh root@<server> "ls -lht ~/cs-legit/launcher-server/storage/games/launcher/ | head -3"

# 3. Запустить лаунчер, посмотреть версию внизу
# Если launcher показывает v2.0.46, а сервер v2.0.47 → он правильно просит обновление
# Если версии равны, но попап всё равно есть → баг в CompareVersions()
```

### Решение

**A. Если сервер отдаёт старый файл:**
```bash
# Проверить, что CI успешно загрузил файл
# https://github.com/gavrikov2044-bot/cs-legit/actions

# Проверить логи сервера
journalctl -u launcher -n 100 | grep "CI] Uploaded"
```

**B. Если версия в exe старая:**
```bash
# Пересобрать и перезалить через CI
git commit --allow-empty -m "rebuild: trigger launcher build"
git push
```

**C. Если логика сравнения кривая:**
Проверить `launcher/src/main.cpp` функцию `CompareVersions()`:
```cpp
// Должна правильно парсить "2.0.47"
int aMajor = 0, aMinor = 0, aPatch = 0;
sscanf(a.c_str(), "%d.%d.%d", &aMajor, &aMinor, &aPatch);
```

---

## 🔴 Статус на сайте красный

### Причина
В базе `game_status.status != 'operational'` или игра вообще не в базе.

### Диагностика
```bash
# Подключиться к базе
ssh root@<server>
cd ~/cs-legit/launcher-server/backend
node -e "const db = require('./src/database/db'); console.log(db.prepare('SELECT * FROM game_status').all());"
```

### Решение
```bash
# Установить operational для launcher
node -e "
const db = require('./src/database/db');
db.prepare(\`
  INSERT OR REPLACE INTO game_status (game_id, status, message, updated_by)
  VALUES ('launcher', 'operational', 'Latest version available', 'admin')
\`).run();
console.log('✅ Status fixed');
"

# Или через API
curl -X POST http://single-project.duckdns.org/api/admin/games/launcher/status \
  -H "Authorization: Bearer <admin-token>" \
  -d '{"status":"operational","message":"Ready"}'
```

---

## 📥 "Download limit exceeded"

### Причина
Rate limiter заблокировал IP (больше 1000 скачиваний/час).

### Проверка
```bash
curl http://single-project.duckdns.org/api/download/launcher
# Если вернёт {"error":"Download limit exceeded."} → лимит сработал
```

### Решение

**A. Увеличить лимит**
В `launcher-server/backend/src/index.js`:
```javascript
const downloadLimiter = rateLimit({
    max: 10000,  // было 1000
    skip: (req) => req.path === '/launcher'  // или полностью отключить для launcher
});
```

**B. Сбросить счётчики (временно)**
```bash
systemctl restart launcher  # rate limit хранится в памяти
```

**C. Подождать**
Лимит сбросится через 1 час с момента первой ошибки.

---

## 🔌 SSH: Connection closed

### Причина
1. Fail2ban заблокировал IP
2. Firewall закрыл порт 22
3. `PasswordAuthentication no` в sshd_config

### Диагностика
```bash
# На сервере (через веб-консоль)
# 1. Проверить fail2ban
fail2ban-client status sshd

# 2. Проверить firewall
ufw status

# 3. Проверить SSH конфиг
grep "PasswordAuthentication" /etc/ssh/sshd_config

# 4. Посмотреть логи SSH
tail -50 /var/log/auth.log | grep sshd
```

### Решение

**A. Разблокировать IP**
```bash
fail2ban-client unban <IP>
```

**B. Открыть порт 22**
```bash
ufw allow 22/tcp
ufw reload
```

**C. Включить пароли в SSH**
```bash
nano /etc/ssh/sshd_config
# Найти: PasswordAuthentication no
# Заменить на: PasswordAuthentication yes
systemctl restart ssh
```

---

## 🎮 Чит не запускается

### Причина
1. Игра не запущена
2. Нет лицензии
3. Статус игры "updating" или "offline"

### Диагностика
В лаунчере посмотреть:
- LICENSE STATUS: должно быть "ACTIVE"
- Статус CS2: должно быть "ONLINE" (зелёный)
- Кнопка LAUNCH: должна быть активной (не серой)

### Решение

**A. Активировать лицензию**
```bash
# Создать ключ в админке
curl -X POST http://.../api/admin/licenses \
  -d '{"game_id":"cs2","days":null}'

# Ввести ключ в лаунчере → ACTIVATE LICENSE
```

**B. Изменить статус игры**
```bash
curl -X POST http://.../api/admin/games/cs2/status \
  -d '{"status":"operational","message":"Working"}'
```

**C. Запустить игру**
Сначала открыть CS2, потом нажать LAUNCH в лаунчере.

---

## 🌐 Сайт не открывается

### Причина
1. Nginx не запущен
2. Backend не работает
3. Firewall закрыл порт 80

### Диагностика
```bash
# Проверить nginx
systemctl status nginx

# Проверить backend
systemctl status launcher

# Проверить порты
netstat -tlnp | grep -E "80|3000"

# Проверить firewall
ufw status
```

### Решение
```bash
# Запустить nginx
systemctl start nginx

# Запустить backend
systemctl start launcher

# Открыть порт 80
ufw allow 80/tcp
ufw allow 443/tcp
```

---

## 🔧 Автодеплой не работает (403 Forbidden)

### Причина
Nginx блокирует POST на `/api/admin/*` без проксирования на backend.

### Решение
Проверить `/etc/nginx/sites-enabled/launcher`:
```nginx
location / {
    proxy_pass http://127.0.0.1:3000;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
}
```

Если есть `deny` или `return 403` → удалить.

---

## 📞 Как получить помощь

### 1. Собрать диагностику
```bash
# На сервере
systemctl status launcher > debug.txt
journalctl -u launcher -n 200 >> debug.txt
curl http://localhost:3000/api/games/status >> debug.txt
ls -lh ~/cs-legit/launcher-server/storage/games/launcher/ >> debug.txt
```

### 2. Проверить логи CI
https://github.com/gavrikov2044-bot/cs-legit/actions

### 3. Проверить версии
- Лаунчер: смотреть внизу окна
- Сервер: `curl .../api/games/status | jq .games.launcher.version`
- Файл: `ls -lh storage/games/launcher/`

---

**Не решилась проблема?** Открой Issue с:
- Скриншотом лаунчера
- Выводом `/api/games/status`
- Логами сервера (последние 50 строк)

