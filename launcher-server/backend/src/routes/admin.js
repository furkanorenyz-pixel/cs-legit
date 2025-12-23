/**
 * Admin Routes
 */

const express = require('express');
const crypto = require('crypto');
const path = require('path');
const fs = require('fs');
const multer = require('multer');
const db = require('../database/db');
const { authenticate, requireAdmin } = require('../middleware/auth');

const router = express.Router();
const STORAGE_PATH = process.env.STORAGE_PATH || '../storage';
const ENCRYPTION_KEY = process.env.ENCRYPTION_KEY || 'default-32-byte-key-change-me!!';
const CI_API_KEY = process.env.CI_API_KEY || 'change-this-ci-api-key';

/**
 * Middleware: Verify CI API Key (for GitHub Actions)
 */
function verifyCiApiKey(req, res, next) {
    const apiKey = req.headers['x-ci-api-key'];
    
    if (!apiKey || apiKey !== CI_API_KEY) {
        return res.status(401).json({ error: 'Invalid CI API key' });
    }
    
    next();
}

// File upload config
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        const gameDir = path.join(STORAGE_PATH, 'games', req.body.game_id);
        fs.mkdirSync(gameDir, { recursive: true });
        cb(null, gameDir);
    },
    filename: (req, file, cb) => {
        cb(null, `${req.body.version}_${Date.now()}_${file.originalname}`);
    }
});
const upload = multer({ storage });

/**
 * Encrypt file
 */
function encryptFile(inputPath, outputPath) {
    const iv = crypto.randomBytes(16);
    const cipher = crypto.createCipheriv('aes-256-cbc',
        Buffer.from(ENCRYPTION_KEY.padEnd(32).slice(0, 32)),
        iv
    );
    
    const input = fs.readFileSync(inputPath);
    const encrypted = Buffer.concat([iv, cipher.update(input), cipher.final()]);
    fs.writeFileSync(outputPath, encrypted);
    
    return outputPath;
}

// ============================================
// CI/CD Endpoints (GitHub Actions) - NO JWT AUTH
// These use API key instead of JWT, must be BEFORE router.use(authenticate)
// ============================================

/**
 * POST /api/admin/ci/upload
 * Upload build from GitHub Actions
 * Headers: X-CI-API-KEY
 */
router.post('/ci/upload', verifyCiApiKey, upload.single('file'), (req, res) => {
    const { game_id, version, changelog, commit_sha } = req.body;
    
    if (!game_id || !version || !req.file) {
        return res.status(400).json({ error: 'game_id, version, and file required' });
    }
    
    try {
        // Encrypt file
        const encryptedPath = req.file.path + '.enc';
        encryptFile(req.file.path, encryptedPath);
        
        // Remove unencrypted file
        fs.unlinkSync(req.file.path);
        
        // Compute hash
        const hash = crypto.createHash('sha256')
                           .update(fs.readFileSync(encryptedPath))
                           .digest('hex');
        
        // Store relative path
        const relativePath = path.relative(STORAGE_PATH, encryptedPath);
        
        // Check if version exists
        const existing = db.prepare('SELECT id FROM versions WHERE game_id = ? AND version = ?')
                           .get(game_id, version);
        
        if (existing) {
            // Update existing version
            db.prepare(`
                UPDATE versions SET file_path = ?, file_hash = ?, changelog = ?
                WHERE game_id = ? AND version = ?
            `).run(relativePath, hash, changelog || '', game_id, version);
        } else {
            // Insert new version
            db.prepare(`
                INSERT INTO versions (game_id, version, changelog, file_path, file_hash)
                VALUES (?, ?, ?, ?, ?)
            `).run(game_id, version, changelog || '', relativePath, hash);
        }
        
        // Update latest version
        db.prepare('UPDATE games SET latest_version = ? WHERE id = ?')
          .run(version, game_id);
        
        console.log(`[CI] Uploaded ${game_id} v${version} (${commit_sha || 'no-sha'})`);
        
        res.status(201).json({
            success: true,
            message: 'Build uploaded',
            game_id,
            version,
            hash,
            commit_sha
        });
    } catch (err) {
        console.error('[CI] Upload error:', err);
        res.status(500).json({ error: 'Upload failed: ' + err.message });
    }
});

/**
 * POST /api/admin/ci/offsets
 * Update offsets from GitHub Actions
 */
router.post('/ci/offsets/:game', verifyCiApiKey, (req, res) => {
    const { game } = req.params;
    const offsets = req.body;
    
    try {
        const offsetsDir = path.join(STORAGE_PATH, 'offsets');
        fs.mkdirSync(offsetsDir, { recursive: true });
        
        const offsetsPath = path.join(offsetsDir, `${game}.json`);
        fs.writeFileSync(offsetsPath, JSON.stringify(offsets, null, 2));
        
        console.log(`[CI] Updated offsets for ${game}`);
        res.json({ success: true, message: 'Offsets updated', game });
    } catch (err) {
        res.status(500).json({ error: 'Failed to update offsets' });
    }
});

/**
 * GET /api/admin/ci/status
 * Check CI API status
 */
router.get('/ci/status', verifyCiApiKey, (req, res) => {
    res.json({ 
        status: 'ok',
        message: 'CI API is working',
        timestamp: new Date().toISOString()
    });
});

// ============================================
// Admin Routes (require JWT auth)
// ============================================

// All admin routes below require authentication and admin privileges
router.use(authenticate);
router.use(requireAdmin);

/**
 * GET /api/admin/stats
 * Get dashboard statistics
 */
router.get('/stats', (req, res) => {
    const stats = {
        users: db.prepare('SELECT COUNT(*) as count FROM users').get().count,
        games: db.prepare('SELECT COUNT(*) as count FROM games').get().count,
        licenses: db.prepare('SELECT COUNT(*) as count FROM licenses').get().count,
        downloads_today: db.prepare(`
            SELECT COUNT(*) as count FROM download_logs 
            WHERE downloaded_at > datetime('now', '-1 day')
        `).get().count
    };
    
    res.json(stats);
});

/**
 * GET /api/admin/users
 * List all users
 */
router.get('/users', (req, res) => {
    const users = db.prepare(`
        SELECT id, username, hwid, is_admin, created_at, last_login
        FROM users ORDER BY created_at DESC
    `).all();
    
    res.json({ users });
});

/**
 * POST /api/admin/licenses
 * Generate new license key
 */
router.post('/licenses', (req, res) => {
    const { game_id, user_id, days } = req.body;
    
    if (!game_id) {
        return res.status(400).json({ error: 'game_id required' });
    }
    
    // Generate unique license key
    const licenseKey = `${game_id.toUpperCase()}-${crypto.randomBytes(4).toString('hex').toUpperCase()}-${crypto.randomBytes(4).toString('hex').toUpperCase()}`;
    
    // Calculate expiry
    let expiresAt = null;
    if (days) {
        expiresAt = new Date(Date.now() + days * 24 * 60 * 60 * 1000).toISOString();
    }
    
    db.prepare(`
        INSERT INTO licenses (user_id, game_id, license_key, expires_at)
        VALUES (?, ?, ?, ?)
    `).run(user_id || null, game_id, licenseKey, expiresAt);
    
    res.status(201).json({
        license_key: licenseKey,
        game_id,
        expires_at: expiresAt
    });
});

/**
 * POST /api/admin/games
 * Create new game
 */
router.post('/games', (req, res) => {
    const { id, name, description } = req.body;
    
    if (!id || !name) {
        return res.status(400).json({ error: 'id and name required' });
    }
    
    try {
        db.prepare(`
            INSERT INTO games (id, name, description, latest_version)
            VALUES (?, ?, ?, '1.0.0')
        `).run(id, name, description || '');
        
        res.status(201).json({ message: 'Game created', id });
    } catch (err) {
        return res.status(409).json({ error: 'Game ID already exists' });
    }
});

/**
 * POST /api/admin/versions
 * Upload new version
 */
router.post('/versions', upload.single('file'), (req, res) => {
    const { game_id, version, changelog } = req.body;
    
    if (!game_id || !version || !req.file) {
        return res.status(400).json({ error: 'game_id, version, and file required' });
    }
    
    // Encrypt file
    const encryptedPath = req.file.path + '.enc';
    encryptFile(req.file.path, encryptedPath);
    
    // Remove unencrypted file
    fs.unlinkSync(req.file.path);
    
    // Compute hash
    const hash = crypto.createHash('sha256')
                       .update(fs.readFileSync(encryptedPath))
                       .digest('hex');
    
    // Store relative path
    const relativePath = path.relative(STORAGE_PATH, encryptedPath);
    
    // Insert version
    db.prepare(`
        INSERT INTO versions (game_id, version, changelog, file_path, file_hash)
        VALUES (?, ?, ?, ?, ?)
    `).run(game_id, version, changelog || '', relativePath, hash);
    
    // Update latest version
    db.prepare('UPDATE games SET latest_version = ? WHERE id = ?')
      .run(version, game_id);
    
    res.status(201).json({
        message: 'Version uploaded',
        game_id,
        version,
        hash
    });
});

/**
 * POST /api/admin/offsets/:game
 * Update offsets for a game
 */
router.post('/offsets/:game', (req, res) => {
    const { game } = req.params;
    const offsets = req.body;
    
    const offsetsDir = path.join(STORAGE_PATH, 'offsets');
    fs.mkdirSync(offsetsDir, { recursive: true });
    
    const offsetsPath = path.join(offsetsDir, `${game}.json`);
    fs.writeFileSync(offsetsPath, JSON.stringify(offsets, null, 2));
    
    res.json({ message: 'Offsets updated', game });
});

/**
 * DELETE /api/admin/users/:id/hwid
 * Reset user HWID
 */
router.delete('/users/:id/hwid', (req, res) => {
    db.prepare('UPDATE users SET hwid = NULL WHERE id = ?').run(req.params.id);
    res.json({ message: 'HWID reset' });
});

module.exports = router;

