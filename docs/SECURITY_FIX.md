# 🔐 СРОЧНО: Исправление утечки Telegram токена

## ⚠️ Проблема
GitHub обнаружил Telegram Bot Token в коде (строка 13 в `telegramMonitor.js`). Токен был в публичном репозитории!

## ✅ Что я исправил

### 1. Убрал токен из кода
**Файл:** `launcher-server/backend/src/services/telegramMonitor.js`

**Было:**
```javascript
const BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN || '8285876782:AAHfsx8nm3MTxR_Fh5hlrMUZIwpY9gtzK60';
const ADMIN_CHAT_ID = '6793512237';
```

**Стало:**
```javascript
const BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN;
const ADMIN_CHAT_ID = process.env.TELEGRAM_ADMIN_ID;
```

Теперь токены читаются **только из переменных окружения** (.env файл).

---

## 🚨 ЧТО НУЖНО СДЕЛАТЬ СРОЧНО

### 1️⃣ Получить новый токен (старый скомпрометирован!)

```bash
# Зайти в Telegram → @BotFather
# Отправить команду:
/mybots

# Выбрать вашего бота
# Нажать: Bot Settings → Revoke Token → Confirm

# Получить новый токен:
# API Token → Скопировать новый токен
```

### 2️⃣ Обновить .env файл на сервере

```bash
# Подключиться к серверу
ssh root@138.124.0.8

# Открыть .env файл
cd ~/cs-legit/launcher-server/backend
nano .env
```

**Добавить/обновить строки:**
```env
# Telegram Bot (для мониторинга CS2 обновлений)
TELEGRAM_BOT_TOKEN=НОВЫЙ_ТОКЕН_ОТ_BOTFATHER
TELEGRAM_ADMIN_ID=6793512237
```

Сохранить: `Ctrl+O`, `Enter`, `Ctrl+X`

### 3️⃣ Перезапустить сервер

```bash
systemctl restart launcher

# Проверить логи:
journalctl -u launcher -f
# Должно быть: "[TelegramMonitor] Started monitoring @cstwoupdate"
```

---

## 📝 Создан .env.example

Файл `launcher-server/backend/.env.example` с шаблоном всех переменных:

```env
# Server Configuration
PORT=3000
HOST=0.0.0.0
NODE_ENV=production

# Security
JWT_SECRET=your_super_secret_jwt_key_min_32_characters
ENCRYPTION_KEY=32_character_encryption_key!!
CI_API_KEY=your_ci_api_key_for_github_actions

# Storage
STORAGE_PATH=../storage

# Telegram Bot (Optional)
TELEGRAM_BOT_TOKEN=your_bot_token_from_@BotFather
TELEGRAM_ADMIN_ID=your_telegram_user_id
```

---

## 🔒 Почему это важно?

1. **Скомпрометированный токен** → кто угодно может отправлять сообщения от имени вашего бота
2. **Токен был публичным** → GitHub Secret Scanning его нашёл (хорошо, что не злоумышленники первыми)
3. **Best Practice** → секреты всегда в `.env`, никогда в коде

---

## ✅ Проверка что всё работает

После обновления токена и перезапуска:

```bash
# 1. Проверить, что сервер запустился
systemctl status launcher

# 2. Проверить логи на ошибки
journalctl -u launcher -n 50 | grep -i error

# 3. Проверить Telegram мониторинг
journalctl -u launcher -n 50 | grep TelegramMonitor

# Должно быть:
# [TelegramMonitor] Started monitoring @cstwoupdate
# [TelegramMonitor] Last known update: ...
```

---

## 🛡️ .gitignore уже настроен

`.env` файл уже в `.gitignore`, поэтому новые токены не попадут в Git:

```gitignore
.env
.env.local
.env.*.local
```

---

## 📚 Дополнительно

Добавлено в документацию:
- **docs/SSH_SETUP.md** - настройка SSH для автодеплоя
- **launcher-server/backend/.env.example** - шаблон переменных окружения

---

**Итог:** После получения нового токена и обновления `.env` всё будет работать безопасно! 🔒

