# 🚀 Cheat Launcher Server

HTTP API сервер для хранения и обновления читов через лаунчер.

> 🎯 **Быстрый старт:** См. [QUICK_START.md](./QUICK_START.md) для бесплатного деплоя за 5 минут!

## 📋 Возможности

- ✅ Хранение файлов по играм и версиям
- ✅ Автоматическая проверка обновлений
- ✅ Манифесты с хешами файлов
- ✅ REST API для лаунчера
- ✅ Поддержка нескольких игр

## 🏗️ Структура файлов

```
files/
├── cs2/
│   ├── external/
│   │   └── v1.0.0/
│   │       ├── externa.exe
│   │       └── manifest.json
│   └── internal/
│       └── v1.0.0/
│           ├── hv_internal.dll
│           ├── hv_injector.exe
│           └── manifest.json
└── dayz/
    └── external/
        └── v1.0.0/
            └── ...
```

## 🚀 Быстрый старт

### Локально (для тестирования)

```bash
cd server
npm install
npm start
```

Сервер запустится на `http://localhost:3000`

### На продакшене (бесплатно!)

**Рекомендуется:** Railway.app - см. [RAILWAY_DEPLOY.md](./RAILWAY_DEPLOY.md)

**Альтернативы:**
- Render.com - см. [DEPLOY.md](./DEPLOY.md#rendercom)
- Fly.io - см. [DEPLOY.md](./DEPLOY.md#flyio)
- VPS ($5/месяц) - см. [DEPLOY.md](./DEPLOY.md#платные-варианты-vps)

## 📡 API Endpoints

### GET `/api/games`
Получить список всех игр

**Response:**
```json
{
  "games": ["cs2", "dayz"]
}
```

### GET `/api/games/:game/latest?type=external`
Получить последнюю версию для игры

**Response:**
```json
{
  "version": "v1.0.0",
  "manifest": {
    "game": "cs2",
    "type": "external",
    "version": "v1.0.0",
    "description": "CS2 External ESP",
    "uploadDate": "2025-01-21T10:00:00.000Z",
    "files": [
      {
        "filename": "externa.exe",
        "size": 1234567,
        "hash": "abc123..."
      }
    ]
  },
  "downloadUrl": "/files/cs2/external/v1.0.0/"
}
```

### GET `/api/check-update?game=cs2&type=external&currentVersion=v0.9.0`
Проверить наличие обновлений

**Response:**
```json
{
  "updateAvailable": true,
  "currentVersion": "v0.9.0",
  "latestVersion": "v1.0.0",
  "manifest": { ... },
  "downloadUrl": "/files/cs2/external/v1.0.0/"
}
```

### POST `/api/upload`
Загрузить новую версию

**Form Data:**
- `game`: название игры (например, "cs2")
- `type`: тип чита ("external" или "internal")
- `version`: версия (например, "v1.0.0")
- `description`: описание (опционально)
- `files`: файлы для загрузки

**Example (curl):**
```bash
curl -X POST http://localhost:3000/api/upload \
  -F "game=cs2" \
  -F "type=external" \
  -F "version=v1.0.0" \
  -F "description=CS2 External ESP v1.0.0" \
  -F "files=@externa.exe"
```

## 🔧 Настройка

### Изменить порт
```bash
PORT=8080 npm start
```

### Использовать другой хост
```bash
HOST=0.0.0.0 PORT=3000 npm start
```

## 📦 Деплой

### Вариант 1: VPS (Ubuntu/Debian)

```bash
# Установить Node.js
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs

# Клонировать проект
git clone <repo>
cd server

# Установить зависимости
npm install --production

# Запустить через PM2
npm install -g pm2
pm2 start server.js --name cheat-server
pm2 save
pm2 startup
```

### Вариант 2: Docker

```dockerfile
FROM node:20-alpine
WORKDIR /app
COPY package*.json ./
RUN npm install --production
COPY . .
EXPOSE 3000
CMD ["node", "server.js"]
```

```bash
docker build -t cheat-server .
docker run -d -p 3000:3000 -v $(pwd)/files:/app/files cheat-server
```

### Вариант 3: Nginx Reverse Proxy

```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    location / {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
    
    location /files {
        alias /path/to/server/files;
        expires 1d;
        add_header Cache-Control "public, immutable";
    }
}
```

## 🔒 Безопасность

Для продакшена добавьте:

1. **Аутентификация** (JWT токены)
2. **Rate limiting** (express-rate-limit)
3. **HTTPS** (Let's Encrypt)
4. **Валидация файлов** (размер, тип)
5. **Логирование** (winston)

## 📝 Пример манифеста

```json
{
  "game": "cs2",
  "type": "external",
  "version": "v1.0.0",
  "description": "CS2 External ESP with menu",
  "uploadDate": "2025-01-21T10:00:00.000Z",
  "files": [
    {
      "filename": "externa.exe",
      "size": 1234567,
      "hash": "a1b2c3d4e5f6..."
    }
  ]
}
```

