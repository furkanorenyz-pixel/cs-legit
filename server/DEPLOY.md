# 🚀 Деплой сервера для лаунчера

## 📋 Варианты сервера

### 1. 🆓 Бесплатные варианты

#### **Railway.app** (Рекомендуется)
- ✅ Бесплатный tier (500 часов/месяц)
- ✅ Автоматический деплой из GitHub
- ✅ HTTPS из коробки
- ✅ Простая настройка

**Шаги:**
1. Зарегистрируйся на [railway.app](https://railway.app)
2. New Project → Deploy from GitHub
3. Выбери репозиторий
4. Railway автоматически определит Node.js
5. Добавь переменную `PORT` (Railway сам назначит)

#### **Render.com**
- ✅ Бесплатный tier
- ✅ Автоматический деплой
- ✅ HTTPS

**Шаги:**
1. Зарегистрируйся на [render.com](https://render.com)
2. New → Web Service
3. Подключи GitHub репозиторий
4. Build Command: `cd server && npm install`
5. Start Command: `cd server && npm start`

#### **Fly.io**
- ✅ Бесплатный tier
- ✅ Глобальный CDN
- ✅ Простой деплой

**Шаги:**
```bash
# Установить flyctl
curl -L https://fly.io/install.sh | sh

# Логин
fly auth login

# Деплой
cd server
fly launch
```

### 2. 💰 Платные варианты (VPS)

#### **DigitalOcean Droplet** ($5/месяц)
```bash
# На Ubuntu 22.04
sudo apt update
sudo apt install -y nodejs npm nginx

# Клонировать проект
git clone <repo>
cd server
npm install --production

# PM2 для автозапуска
sudo npm install -g pm2
pm2 start server.js --name cheat-server
pm2 save
pm2 startup
```

#### **Hetzner Cloud** (€4/месяц)
Аналогично DigitalOcean

#### **Vultr** ($5/месяц)
Аналогично DigitalOcean

### 3. ☁️ Облачные хранилища

#### **AWS S3 + CloudFront**
- ✅ Очень дешево (первые 5GB бесплатно)
- ✅ Глобальный CDN
- ✅ Нужен отдельный API для управления

#### **Cloudflare R2**
- ✅ S3-совместимое API
- ✅ Бесплатный egress (нет платы за трафик)
- ✅ Первые 10GB бесплатно

## 🔧 Настройка Nginx (для VPS)

```nginx
# /etc/nginx/sites-available/cheat-server
server {
    listen 80;
    server_name your-domain.com;

    # Редирект на HTTPS
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name your-domain.com;

    ssl_certificate /etc/letsencrypt/live/your-domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your-domain.com/privkey.pem;

    # API прокси
    location /api {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_cache_bypass $http_upgrade;
    }

    # Статические файлы
    location /files {
        alias /path/to/server/files;
        expires 1d;
        add_header Cache-Control "public, immutable";
        
        # CORS для загрузки
        add_header Access-Control-Allow-Origin *;
    }
}
```

## 🔒 SSL сертификат (Let's Encrypt)

```bash
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d your-domain.com
```

## 📤 Загрузка файлов

### Через curl
```bash
curl -X POST https://your-server.com/api/upload \
  -F "game=cs2" \
  -F "type=external" \
  -F "version=v1.0.0" \
  -F "description=CS2 External ESP" \
  -F "files=@externa.exe"
```

### Через скрипт
```bash
cd server
./upload-example.sh
```

### Через GitHub Actions
Создай workflow для автоматической загрузки после билда:

```yaml
# .github/workflows/upload.yml
name: Upload to Server
on:
  workflow_run:
    workflows: ["Build"]
    types: [completed]

jobs:
  upload:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Upload files
        run: |
          curl -X POST ${{ secrets.SERVER_URL }}/api/upload \
            -F "game=cs2" \
            -F "type=external" \
            -F "version=v${{ github.run_number }}" \
            -F "files=@build/externa.exe"
```

## 🔐 Безопасность (опционально)

### Добавить аутентификацию

```javascript
// server.js
const jwt = require('jsonwebtoken');

const SECRET = process.env.JWT_SECRET || 'your-secret-key';

// Middleware для проверки токена
function authenticate(req, res, next) {
    const token = req.headers.authorization?.split(' ')[1];
    
    if (!token) {
        return res.status(401).json({ error: 'No token' });
    }
    
    try {
        const decoded = jwt.verify(token, SECRET);
        req.user = decoded;
        next();
    } catch (err) {
        return res.status(401).json({ error: 'Invalid token' });
    }
}

// Защитить upload endpoint
app.post('/api/upload', authenticate, upload.array('files'), ...);
```

### Rate Limiting

```javascript
const rateLimit = require('express-rate-limit');

const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 минут
    max: 100 // максимум 100 запросов
});

app.use('/api/', limiter);
```

## 📊 Мониторинг

### PM2 Monitoring
```bash
pm2 monit
pm2 logs cheat-server
```

### Health Check Endpoint
```javascript
app.get('/health', (req, res) => {
    res.json({ status: 'ok', uptime: process.uptime() });
});
```

## 🎯 Итоговая структура

```
server/
├── server.js          # Основной сервер
├── package.json       # Зависимости
├── files/             # Хранилище файлов (не в git)
│   ├── cs2/
│   │   ├── external/
│   │   └── internal/
│   └── dayz/
├── .env              # Секреты (не в git)
└── README.md
```

## ✅ Чеклист деплоя

- [ ] Сервер запущен и доступен
- [ ] HTTPS настроен (для продакшена)
- [ ] Файлы загружаются через `/api/upload`
- [ ] API отвечает на `/api/games`
- [ ] Лаунчер может проверить обновления
- [ ] Файлы скачиваются через `/files/`
- [ ] Логирование работает
- [ ] Backup настроен (опционально)

