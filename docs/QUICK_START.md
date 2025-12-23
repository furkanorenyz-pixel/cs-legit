# 🚀 Quick Start Guide

## Для Разработчика

### 1️⃣ Первый запуск

```bash
# Клонировать репозиторий
git clone https://github.com/gavrikov2044-bot/cs-legit.git
cd cs-legit

# Настроить сервер (см. launcher-server/docs/DEPLOYMENT.md)
cd launcher-server/backend
npm install
cp .env.example .env
nano .env  # настроить переменные
npm run migrate
node src/index.js
```

### 2️⃣ Локальная разработка лаунчера

```bash
cd launcher
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/launcher.exe
```

### 3️⃣ Деплой изменений

**Любые изменения:**
```bash
git add .
git commit -m "feat: описание изменений"
git push
```

**GitHub Actions автоматически:**
- Соберёт launcher → загрузит на сервер
- Соберёт externa → загрузит на сервер
- Обновит бэкенд (если изменения в launcher-server/)

---

## Для Администратора

### 🔧 Управление сервером

**Перезапуск:**
```bash
ssh root@138.124.0.8
systemctl restart launcher
```

**Логи:**
```bash
journalctl -u launcher -f
```

**Обновление кода:**
```bash
cd ~/cs-legit/launcher-server/backend
git pull
systemctl restart launcher
```

### 🔑 Генерация лицензий

**Через админку:**
1. Открыть `http://single-project.duckdns.org/panel`
2. Логин: admin / твой пароль
3. Licenses → Generate

**Через API:**
```bash
curl -X POST http://single-project.duckdns.org/api/admin/licenses \
  -H "Authorization: Bearer <admin-token>" \
  -H "Content-Type: application/json" \
  -d '{"game_id":"cs2","days":null,"count":10}'
```

### 🎮 Управление статусом игр

**Установить "Updating":**
```bash
curl -X POST http://single-project.duckdns.org/api/admin/games/cs2/status \
  -H "Authorization: Bearer <token>" \
  -d '{"status":"updating","message":"Game update detected"}'
```

**Установить "Operational":**
```bash
curl -X POST http://single-project.duckdns.org/api/admin/games/cs2/status \
  -H "Authorization: Bearer <token>" \
  -d '{"status":"operational","message":"Cheat updated"}'
```

---

## Для Пользователя

### 📥 Установка

1. Зайти на `http://single-project.duckdns.org`
2. Нажать **DOWNLOAD LAUNCHER**
3. Запустить `launcher.exe`

### 🔐 Первый вход

1. Получить лицензионный ключ
2. Нажать "Register"
3. Ввести ключ, логин, пароль
4. Войти

### 🎯 Запуск чита

1. Открыть игру (CS2)
2. В лаунчере выбрать игру
3. Нажать **LAUNCH**
4. Чит загрузится автоматически

---

## 🐛 Частые Проблемы

| Проблема | Решение |
|----------|---------|
| "Download limit exceeded" | Подождать 1 час или увеличить лимит в `index.js` |
| "Connection failed" | Проверить, что сервер запущен: `systemctl status launcher` |
| "Invalid license" | Проверить, что ключ не использован: `/panel` → Licenses |
| "Update Required" (бесконечно) | Скачать свежий лаунчер с сайта заново |
| SSH connection closed | Проверить fail2ban: `fail2ban-client status sshd` |

---

## 📚 Подробная Документация

- **Структура проекта:** `docs/PROJECT_STRUCTURE.md`
- **Деплой сервера:** `launcher-server/docs/DEPLOYMENT.md`
- **Админ панель:** `docs/ADMIN_PANEL.md`
- **GitHub настройка:** `docs/GITHUB_SETUP.md`

---

**Вопросы?** Смотри полную документацию в `/docs/`

