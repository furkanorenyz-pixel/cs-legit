/**
 * Download Routes
 */

const express = require('express');
const path = require('path');
const fs = require('fs');
const crypto = require('crypto');
const db = require('../database/db');
const { authenticate, verifyHwid, checkLicense } = require('../middleware/auth');

const router = express.Router();
const STORAGE_PATH = process.env.STORAGE_PATH || '../storage';
const ENCRYPTION_KEY = process.env.ENCRYPTION_KEY || 'default-32-byte-key-change-me!!';

/**
 * Decrypt file for streaming
 */
function decryptFile(filePath) {
    const encrypted = fs.readFileSync(filePath);
    
    // First 16 bytes are IV
    const iv = encrypted.slice(0, 16);
    const data = encrypted.slice(16);
    
    const decipher = crypto.createDecipheriv('aes-256-cbc', 
        Buffer.from(ENCRYPTION_KEY.padEnd(32).slice(0, 32)), 
        iv
    );
    
    return Buffer.concat([decipher.update(data), decipher.final()]);
}

/**
 * GET /api/download/:game/:version
 * Download cheat file
 */
router.get('/:game/:version', authenticate, verifyHwid, (req, res) => {
    const { game, version } = req.params;
    const hwid = req.headers['x-hwid'];
    
    // Check license
    const license = db.prepare(`
        SELECT * FROM licenses 
        WHERE user_id = ? AND game_id = ? 
        AND (expires_at IS NULL OR expires_at > datetime('now'))
    `).get(req.user.id, game);
    
    if (!license) {
        return res.status(403).json({ error: 'No valid license for this game' });
    }
    
    // Get version info
    const versionInfo = db.prepare(`
        SELECT * FROM versions 
        WHERE game_id = ? AND version = ?
    `).get(game, version);
    
    if (!versionInfo) {
        return res.status(404).json({ error: 'Version not found' });
    }
    
    // Construct file path
    const filePath = path.join(STORAGE_PATH, versionInfo.file_path);
    
    if (!fs.existsSync(filePath)) {
        return res.status(404).json({ error: 'File not found' });
    }
    
    // Log download
    db.prepare(`
        INSERT INTO download_logs (user_id, game_id, version, ip_address, hwid)
        VALUES (?, ?, ?, ?, ?)
    `).run(req.user.id, game, version, req.ip, hwid);
    
    // Check if file is encrypted (.enc extension)
    if (filePath.endsWith('.enc')) {
        try {
            const decrypted = decryptFile(filePath);
            
            // Send with original filename (without .enc)
            const originalName = path.basename(filePath, '.enc');
            res.setHeader('Content-Disposition', `attachment; filename="${originalName}"`);
            res.setHeader('Content-Type', 'application/octet-stream');
            res.send(decrypted);
        } catch (err) {
            console.error('Decryption error:', err);
            return res.status(500).json({ error: 'Failed to decrypt file' });
        }
    } else {
        // Send unencrypted file directly
        res.download(filePath);
    }
});

/**
 * GET /api/download/:game/latest
 * Download latest version
 */
router.get('/:game/latest', authenticate, verifyHwid, (req, res) => {
    const { game } = req.params;
    
    const gameInfo = db.prepare('SELECT latest_version FROM games WHERE id = ?').get(game);
    
    if (!gameInfo || !gameInfo.latest_version) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    // Redirect to specific version
    res.redirect(`/api/download/${game}/${gameInfo.latest_version}`);
});

module.exports = router;

