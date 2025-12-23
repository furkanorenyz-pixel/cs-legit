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
 * 
 * Body:
 * - game_id: string (required) - cs2, dayz, rust, all
 * - days: number (optional) - subscription days (null = lifetime)
 * - count: number (optional) - how many keys to generate (default 1)
 * - prefix: string (optional) - custom prefix for keys
 */
router.post('/licenses', (req, res) => {
    const { game_id, days, count = 1, prefix } = req.body;
    
    if (!game_id) {
        return res.status(400).json({ error: 'game_id required' });
    }
    
    if (count > 100) {
        return res.status(400).json({ error: 'Max 100 keys at once' });
    }
    
    // Calculate expiry
    let expiresAt = null;
    if (days && days > 0) {
        expiresAt = new Date(Date.now() + days * 24 * 60 * 60 * 1000).toISOString();
    }
    
    const keys = [];
    const insertStmt = db.prepare(`
        INSERT INTO licenses (user_id, game_id, license_key, expires_at)
        VALUES (NULL, ?, ?, ?)
    `);
    
    for (let i = 0; i < count; i++) {
        // Generate unique license key
        const keyPrefix = prefix || game_id.toUpperCase();
        const keyBody = crypto.randomBytes(6).toString('hex').toUpperCase();
        const licenseKey = `${keyPrefix}-${keyBody.slice(0, 4)}-${keyBody.slice(4, 8)}-${keyBody.slice(8, 12)}`;
        
        insertStmt.run(game_id, licenseKey, expiresAt);
        keys.push(licenseKey);
    }
    
    // Format subscription type
    let subscriptionType = 'Lifetime';
    if (days === 1) subscriptionType = '1 Day';
    else if (days === 7) subscriptionType = '1 Week';
    else if (days === 30) subscriptionType = '1 Month';
    else if (days === 90) subscriptionType = '3 Months';
    else if (days === 365) subscriptionType = '1 Year';
    else if (days) subscriptionType = `${days} Days`;
    
    res.status(201).json({
        success: true,
        count: keys.length,
        game_id,
        subscription: subscriptionType,
        expires_at: expiresAt,
        keys: keys
    });
});

/**
 * GET /api/admin/licenses
 * List all licenses with filters
 */
router.get('/licenses', (req, res) => {
    const { game_id, unused, expired } = req.query;
    
    let query = 'SELECT l.*, u.username FROM licenses l LEFT JOIN users u ON l.user_id = u.id WHERE 1=1';
    const params = [];
    
    if (game_id) {
        query += ' AND l.game_id = ?';
        params.push(game_id);
    }
    
    if (unused === 'true') {
        query += ' AND l.user_id IS NULL';
    }
    
    if (expired === 'true') {
        query += " AND l.expires_at IS NOT NULL AND l.expires_at < datetime('now')";
    } else if (expired === 'false') {
        query += " AND (l.expires_at IS NULL OR l.expires_at > datetime('now'))";
    }
    
    query += ' ORDER BY l.created_at DESC LIMIT 100';
    
    const licenses = db.prepare(query).all(...params);
    
    res.json({ licenses });
});

/**
 * DELETE /api/admin/licenses/:key
 * Delete/revoke a license key
 */
router.delete('/licenses/:key', (req, res) => {
    const { key } = req.params;
    
    const result = db.prepare('DELETE FROM licenses WHERE license_key = ?').run(key);
    
    if (result.changes === 0) {
        return res.status(404).json({ error: 'License not found' });
    }
    
    res.json({ success: true, message: 'License revoked' });
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
 * GET /api/admin/users/:id
 * Get full user details with subscriptions
 */
router.get('/users/:id', (req, res) => {
    const user = db.prepare(`
        SELECT id, username, hwid, is_admin, is_banned, created_at, last_login
        FROM users WHERE id = ?
    `).get(req.params.id);
    
    if (!user) {
        return res.status(404).json({ error: 'User not found' });
    }
    
    // Get user's licenses
    const licenses = db.prepare(`
        SELECT id, game_id, license_key, expires_at, created_at,
               CASE 
                   WHEN expires_at IS NULL THEN 'lifetime'
                   WHEN expires_at > datetime('now') THEN 'active'
                   ELSE 'expired'
               END as status
        FROM licenses WHERE user_id = ?
        ORDER BY created_at DESC
    `).all(req.params.id);
    
    // Get recent downloads
    const downloads = db.prepare(`
        SELECT game_id, version, ip_address, hwid, downloaded_at
        FROM download_logs WHERE user_id = ?
        ORDER BY downloaded_at DESC LIMIT 20
    `).all(req.params.id);
    
    res.json({ user, licenses, downloads });
});

/**
 * GET /api/admin/users/full
 * Get all users with subscription status
 */
router.get('/users-full', (req, res) => {
    const users = db.prepare(`
        SELECT 
            u.id, 
            u.username, 
            u.hwid,
            u.is_admin,
            u.is_banned,
            u.created_at, 
            u.last_login,
            (SELECT COUNT(*) FROM licenses l 
             WHERE l.user_id = u.id 
             AND (l.expires_at IS NULL OR l.expires_at > datetime('now'))) as active_licenses,
            (SELECT GROUP_CONCAT(DISTINCT l2.game_id) FROM licenses l2 
             WHERE l2.user_id = u.id 
             AND (l2.expires_at IS NULL OR l2.expires_at > datetime('now'))) as active_games,
            (SELECT MIN(l3.expires_at) FROM licenses l3 
             WHERE l3.user_id = u.id 
             AND l3.expires_at IS NOT NULL 
             AND l3.expires_at > datetime('now')) as next_expiry
        FROM users u
        ORDER BY u.created_at DESC
    `).all();
    
    res.json({ users });
});

/**
 * DELETE /api/admin/users/:id/hwid
 * Reset user HWID (allow rebind)
 */
router.delete('/users/:id/hwid', (req, res) => {
    const result = db.prepare('UPDATE users SET hwid = NULL WHERE id = ?').run(req.params.id);
    
    if (result.changes === 0) {
        return res.status(404).json({ error: 'User not found' });
    }
    
    console.log(`[Admin] Reset HWID for user ${req.params.id}`);
    res.json({ success: true, message: 'HWID reset - user can now login from new device' });
});

/**
 * POST /api/admin/users/:id/ban
 * Ban user
 */
router.post('/users/:id/ban', (req, res) => {
    const { reason } = req.body;
    
    db.prepare('UPDATE users SET is_banned = 1, ban_reason = ? WHERE id = ?')
      .run(reason || 'No reason specified', req.params.id);
    
    console.log(`[Admin] Banned user ${req.params.id}: ${reason}`);
    res.json({ success: true, message: 'User banned' });
});

/**
 * POST /api/admin/users/:id/unban
 * Unban user
 */
router.post('/users/:id/unban', (req, res) => {
    db.prepare('UPDATE users SET is_banned = 0, ban_reason = NULL WHERE id = ?')
      .run(req.params.id);
    
    console.log(`[Admin] Unbanned user ${req.params.id}`);
    res.json({ success: true, message: 'User unbanned' });
});

/**
 * DELETE /api/admin/users/:id
 * Delete user account
 */
router.delete('/users/:id', (req, res) => {
    const userId = req.params.id;
    
    // Don't allow deleting yourself
    if (parseInt(userId) === req.user.id) {
        return res.status(400).json({ error: 'Cannot delete yourself' });
    }
    
    // Delete user's licenses first
    db.prepare('DELETE FROM licenses WHERE user_id = ?').run(userId);
    
    // Delete download logs
    db.prepare('DELETE FROM download_logs WHERE user_id = ?').run(userId);
    
    // Delete user
    const result = db.prepare('DELETE FROM users WHERE id = ?').run(userId);
    
    if (result.changes === 0) {
        return res.status(404).json({ error: 'User not found' });
    }
    
    console.log(`[Admin] Deleted user ${userId}`);
    res.json({ success: true, message: 'User deleted' });
});

/**
 * POST /api/admin/users/:id/extend
 * Extend user's subscription
 */
router.post('/users/:id/extend', (req, res) => {
    const { game_id, days } = req.body;
    const userId = req.params.id;
    
    if (!game_id || !days) {
        return res.status(400).json({ error: 'game_id and days required' });
    }
    
    // Find active license for this game
    const license = db.prepare(`
        SELECT * FROM licenses 
        WHERE user_id = ? AND game_id = ?
        ORDER BY expires_at DESC LIMIT 1
    `).get(userId, game_id);
    
    if (!license) {
        // Create new license
        const expiresAt = new Date(Date.now() + days * 24 * 60 * 60 * 1000).toISOString();
        const licenseKey = `ADMIN-${crypto.randomBytes(6).toString('hex').toUpperCase()}`;
        
        db.prepare(`
            INSERT INTO licenses (user_id, game_id, license_key, expires_at)
            VALUES (?, ?, ?, ?)
        `).run(userId, game_id, licenseKey, expiresAt);
        
        return res.json({ success: true, message: `Created new ${days}-day license` });
    }
    
    if (license.expires_at === null) {
        return res.json({ success: true, message: 'User already has lifetime license' });
    }
    
    // Extend existing license
    const currentExpiry = new Date(license.expires_at);
    const baseDate = currentExpiry > new Date() ? currentExpiry : new Date();
    const newExpiry = new Date(baseDate.getTime() + days * 24 * 60 * 60 * 1000).toISOString();
    
    db.prepare('UPDATE licenses SET expires_at = ? WHERE id = ?')
      .run(newExpiry, license.id);
    
    console.log(`[Admin] Extended user ${userId} license for ${game_id} by ${days} days`);
    res.json({ success: true, message: `Extended by ${days} days`, new_expiry: newExpiry });
});

/**
 * POST /api/admin/users/:id/revoke
 * Revoke user's license for a game
 */
router.post('/users/:id/revoke', (req, res) => {
    const { game_id } = req.body;
    const userId = req.params.id;
    
    if (!game_id) {
        return res.status(400).json({ error: 'game_id required' });
    }
    
    const result = db.prepare('DELETE FROM licenses WHERE user_id = ? AND game_id = ?')
                     .run(userId, game_id);
    
    if (result.changes === 0) {
        return res.status(404).json({ error: 'No license found for this game' });
    }
    
    console.log(`[Admin] Revoked ${game_id} license for user ${userId}`);
    res.json({ success: true, message: 'License revoked' });
});

module.exports = router;

