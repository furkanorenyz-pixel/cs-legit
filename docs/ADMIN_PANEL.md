# 🔐 Admin Panel Guide

## Архитектура безопасности

```
┌─────────────────────────────────────────────────────────────────┐
│                         СЕРВЕР                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Nginx (порт 80)                                           │   │
│  │                                                           │   │
│  │  PUBLIC (доступно всем):                                  │   │
│  │    /api/auth/*     - логин/регистрация                   │   │
│  │    /api/games/*    - список игр                          │   │
│  │    /api/download/* - скачивание читов                    │   │
│  │    /api/offsets/*  - оффсеты игр                         │   │
│  │    /health         - проверка статуса                    │   │
│  │                                                           │   │
│  │  PRIVATE (только localhost):                              │   │
│  │    /panel/*        - веб админ-панель                    │   │
│  │    /api/admin/*    - API для управления                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                     Node.js (127.0.0.1:3000)                    │
└─────────────────────────────────────────────────────────────────┘
                               ↑
                    SSH Tunnel (только ты)
                               ↑
┌─────────────────────────────────────────────────────────────────┐
│                     ТВОЙ КОМПЬЮТЕР                              │
│                                                                  │
│   ssh -L 8080:127.0.0.1:80 root@single-project.duckdns.org                     │
│                                                                  │
│   http://localhost:8080/panel/ → Админ-панель                   │
└─────────────────────────────────────────────────────────────────┘
```

## Подключение к админ-панели

### Шаг 1: Открой SSH туннель

```bash
# На Windows (PowerShell/cmd):
ssh -L 8080:127.0.0.1:80 root@single-project.duckdns.org

# Пароль: mmE28jaX99
```

### Шаг 2: Открой браузер

Перейди на: **http://localhost:8080/panel/**

### Шаг 3: Авторизация

1. **HTTP Basic Auth** (браузер спросит):
   - Username: `admin`
   - Password: `SuperAdmin123`

2. **Логин в панели**:
   - Username: `admin`
   - Password: `admin123`

---

## Функции админ-панели

### 🔑 Генерация лицензий

- Выбери игру (CS2, DayZ, Rust)
- Выбери срок подписки (1 день / 1 неделя / 1 месяц / Lifetime)
- Укажи количество ключей
- Опционально добавь префикс (TRIAL, VIP, и т.д.)

**Пример ключей:**
```
CS2-A1B2-C3D4-E5F6     (Lifetime)
TRIAL-1234-5678-ABCD   (7 дней)
VIP-XXXX-YYYY-ZZZZ     (1 месяц)
```

### 👥 Управление пользователями

- Просмотр всех зарегистрированных пользователей
- Сброс HWID (если пользователь сменил ПК)
- Просмотр последней активности

### 📋 Все лицензии

- Список всех выданных ключей
- Фильтр неиспользованных
- Отзыв лицензий
- Копирование ключей

---

## Как выдать подписку пользователю

### Вариант 1: Через генерацию ключа

1. Сгенерируй ключ в панели
2. Отправь ключ пользователю
3. Пользователь регистрируется с этим ключом в лаунчере

### Вариант 2: Через API (для автоматизации)

```bash
# Через SSH туннель
TOKEN="твой_jwt_токен"

# Создать ключ на 30 дней
curl -u admin:SuperAdmin123 -X POST http://localhost:8080/api/admin/licenses \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"game_id":"cs2","days":30,"count":1}'
```

---

## Безопасность

| Угроза | Защита |
|--------|--------|
| Доступ к админке из интернета | Nginx блокирует `/panel` и `/api/admin` для внешних IP |
| Брутфорс пароля | Двойная авторизация: HTTP Basic + JWT |
| Перехват трафика | SSH туннель шифрует всё |
| Подмена токена | JWT подписан секретным ключом |

---

## Смена паролей

### Сменить пароль HTTP Basic Auth:

```bash
ssh root@single-project.duckdns.org
htpasswd -b /etc/nginx/.htpasswd admin НОВЫЙ_ПАРОЛЬ
```

### Сменить пароль админа в приложении:

```bash
ssh root@single-project.duckdns.org
nano /root/cs-legit/launcher-server/backend/.env
# Измени ADMIN_PASSWORD=...
systemctl restart launcher
```

---

## Скрипт быстрого подключения (macOS/Linux)

Сохрани как `admin.sh`:

```bash
#!/bin/bash
echo "🔐 Подключение к админ-панели..."
echo "После подключения открой: http://localhost:8080/panel/"
echo ""
ssh -L 8080:127.0.0.1:80 root@single-project.duckdns.org
```

---

## Windows: Создай ярлык

1. Создай файл `admin.bat`:

```batch
@echo off
echo Connecting to admin panel...
echo Open http://localhost:8080/panel/ after connection
ssh -L 8080:127.0.0.1:80 root@single-project.duckdns.org
pause
```

2. Запусти и открой браузер

---

## Проверка доступности

```bash
# Из интернета (должно быть 403):
curl http://single-project.duckdns.org/panel/
# Ответ: 403 Forbidden

# Через туннель (должно работать):
curl -u admin:SuperAdmin123 http://localhost:8080/panel/
# Ответ: HTML страница
```

