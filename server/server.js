/**
 * CHEAT LAUNCHER SERVER
 * HTTP API для хранения и обновления читов
 * 
 * Структура:
 * /files/
 *   /cs2/
 *     /external/
 *       v1.0.0/
 *         externa.exe
 *         manifest.json
 *     /internal/
 *       v1.0.0/
 *         hv_internal.dll
 *         hv_injector.exe
 *         manifest.json
 *   /dayz/
 *     /external/
 *       ...
 */

const express = require('express');
const multer = require('multer');
const fs = require('fs-extra');
const path = require('path');
const crypto = require('crypto');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use('/files', express.static('files'));

// Конфигурация загрузки файлов
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        const { game, type, version } = req.body;
        const uploadPath = path.join('files', game, type, version);
        fs.ensureDirSync(uploadPath);
        cb(null, uploadPath);
    },
    filename: (req, file, cb) => {
        cb(null, file.originalname);
    }
});

const upload = multer({ storage });

// ============================================
// API Routes
// ============================================

/**
 * GET /api/games
 * Получить список всех игр
 */
app.get('/api/games', (req, res) => {
    const filesDir = path.join(__dirname, 'files');
    
    if (!fs.existsSync(filesDir)) {
        return res.json({ games: [] });
    }
    
    const games = fs.readdirSync(filesDir)
        .filter(item => {
            const itemPath = path.join(filesDir, item);
            return fs.statSync(itemPath).isDirectory();
        });
    
    res.json({ games });
});

/**
 * GET /api/games/:game/versions
 * Получить список версий для игры
 */
app.get('/api/games/:game/versions', (req, res) => {
    const { game, type } = req.params;
    const gameDir = path.join(__dirname, 'files', game, type || 'external');
    
    if (!fs.existsSync(gameDir)) {
        return res.json({ versions: [] });
    }
    
    const versions = fs.readdirSync(gameDir)
        .filter(item => {
            const itemPath = path.join(gameDir, item);
            return fs.statSync(itemPath).isDirectory();
        })
        .sort((a, b) => {
            // Сортировка версий (v1.0.0 > v0.9.0)
            return b.localeCompare(a, undefined, { numeric: true });
        });
    
    res.json({ versions });
});

/**
 * GET /api/games/:game/latest
 * Получить последнюю версию для игры
 */
app.get('/api/games/:game/latest', (req, res) => {
    const { game } = req.params;
    const { type = 'external' } = req.query;
    const gameDir = path.join(__dirname, 'files', game, type);
    
    if (!fs.existsSync(gameDir)) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    const versions = fs.readdirSync(gameDir)
        .filter(item => {
            const itemPath = path.join(gameDir, item);
            return fs.statSync(itemPath).isDirectory();
        })
        .sort((a, b) => b.localeCompare(a, undefined, { numeric: true }));
    
    if (versions.length === 0) {
        return res.status(404).json({ error: 'No versions found' });
    }
    
    const latestVersion = versions[0];
    const manifestPath = path.join(gameDir, latestVersion, 'manifest.json');
    
    if (!fs.existsSync(manifestPath)) {
        return res.status(404).json({ error: 'Manifest not found' });
    }
    
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    
    res.json({
        version: latestVersion,
        manifest,
        downloadUrl: `/files/${game}/${type}/${latestVersion}/`
    });
});

/**
 * GET /api/games/:game/:type/:version/manifest
 * Получить манифест версии
 */
app.get('/api/games/:game/:type/:version/manifest', (req, res) => {
    const { game, type, version } = req.params;
    const manifestPath = path.join(__dirname, 'files', game, type, version, 'manifest.json');
    
    if (!fs.existsSync(manifestPath)) {
        return res.status(404).json({ error: 'Manifest not found' });
    }
    
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    res.json(manifest);
});

/**
 * GET /api/games/:game/:type/:version/files
 * Получить список файлов версии
 */
app.get('/api/games/:game/:type/:version/files', (req, res) => {
    const { game, type, version } = req.params;
    const versionDir = path.join(__dirname, 'files', game, type, version);
    
    if (!fs.existsSync(versionDir)) {
        return res.status(404).json({ error: 'Version not found' });
    }
    
    const files = fs.readdirSync(versionDir)
        .filter(item => {
            const itemPath = path.join(versionDir, item);
            return fs.statSync(itemPath).isFile();
        })
        .map(filename => {
            const filePath = path.join(versionDir, filename);
            const stats = fs.statSync(filePath);
            const hash = calculateFileHash(filePath);
            
            return {
                filename,
                size: stats.size,
                hash,
                url: `/files/${game}/${type}/${version}/${filename}`
            };
        });
    
    res.json({ files });
});

/**
 * POST /api/upload
 * Загрузить новую версию
 */
app.post('/api/upload', upload.array('files'), (req, res) => {
    const { game, type, version, description } = req.body;
    
    if (!game || !type || !version) {
        return res.status(400).json({ error: 'Missing required fields' });
    }
    
    const versionDir = path.join(__dirname, 'files', game, type, version);
    
    // Создать манифест
    const manifest = {
        game,
        type,
        version,
        description: description || '',
        uploadDate: new Date().toISOString(),
        files: req.files.map(file => ({
            filename: file.filename,
            size: file.size,
            hash: calculateFileHash(file.path)
        }))
    };
    
    const manifestPath = path.join(versionDir, 'manifest.json');
    fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2));
    
    res.json({
        success: true,
        manifest,
        message: `Version ${version} uploaded successfully`
    });
});

/**
 * GET /api/check-update
 * Проверить обновления (для лаунчера)
 */
app.get('/api/check-update', (req, res) => {
    const { game, type, currentVersion } = req.query;
    
    if (!game || !type) {
        return res.status(400).json({ error: 'Missing game or type' });
    }
    
    const gameDir = path.join(__dirname, 'files', game, type);
    
    if (!fs.existsSync(gameDir)) {
        return res.json({ updateAvailable: false });
    }
    
    const versions = fs.readdirSync(gameDir)
        .filter(item => {
            const itemPath = path.join(gameDir, item);
            return fs.statSync(itemPath).isDirectory();
        })
        .sort((a, b) => b.localeCompare(a, undefined, { numeric: true }));
    
    if (versions.length === 0) {
        return res.json({ updateAvailable: false });
    }
    
    const latestVersion = versions[0];
    const hasUpdate = !currentVersion || latestVersion !== currentVersion;
    
    if (!hasUpdate) {
        return res.json({ updateAvailable: false, version: currentVersion });
    }
    
    const manifestPath = path.join(gameDir, latestVersion, 'manifest.json');
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    
    res.json({
        updateAvailable: true,
        currentVersion: currentVersion || 'none',
        latestVersion,
        manifest,
        downloadUrl: `/files/${game}/${type}/${latestVersion}/`
    });
});

// ============================================
// Helpers
// ============================================

function calculateFileHash(filePath) {
    const fileBuffer = fs.readFileSync(filePath);
    return crypto.createHash('sha256').update(fileBuffer).digest('hex');
}

// ============================================
// Start Server
// ============================================

// Создать структуру директорий
const filesDir = path.join(__dirname, 'files');
fs.ensureDirSync(filesDir);

app.listen(PORT, () => {
    console.log(`🚀 Cheat Launcher Server running on port ${PORT}`);
    console.log(`📁 Files directory: ${filesDir}`);
    console.log(`\n📡 API Endpoints:`);
    console.log(`   GET  /api/games - List all games`);
    console.log(`   GET  /api/games/:game/latest - Get latest version`);
    console.log(`   GET  /api/check-update - Check for updates`);
    console.log(`   POST /api/upload - Upload new version`);
});

