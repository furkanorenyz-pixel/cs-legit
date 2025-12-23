/**
 * Games Routes
 */

const express = require('express');
const db = require('../database/db');
const { authenticate } = require('../middleware/auth');

const router = express.Router();

/**
 * GET /api/games
 * Get list of all available games
 */
router.get('/', authenticate, (req, res) => {
    const games = db.prepare(`
        SELECT 
            g.id, 
            g.name, 
            g.description, 
            g.icon_url,
            g.latest_version,
            CASE WHEN l.id IS NOT NULL THEN 1 ELSE 0 END as has_license,
            l.expires_at as license_expires
        FROM games g
        LEFT JOIN licenses l ON l.game_id = g.id AND l.user_id = ?
            AND (l.expires_at IS NULL OR l.expires_at > datetime('now'))
        WHERE g.is_active = 1
        ORDER BY g.name
    `).all(req.user.id);
    
    res.json({ games });
});

/**
 * GET /api/games/:id
 * Get single game details
 */
router.get('/:id', authenticate, (req, res) => {
    const game = db.prepare(`
        SELECT 
            g.*,
            CASE WHEN l.id IS NOT NULL THEN 1 ELSE 0 END as has_license,
            l.expires_at as license_expires
        FROM games g
        LEFT JOIN licenses l ON l.game_id = g.id AND l.user_id = ?
            AND (l.expires_at IS NULL OR l.expires_at > datetime('now'))
        WHERE g.id = ? AND g.is_active = 1
    `).get(req.user.id, req.params.id);
    
    if (!game) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    res.json({ game });
});

/**
 * GET /api/games/:id/versions
 * Get all versions of a game cheat
 */
router.get('/:id/versions', authenticate, (req, res) => {
    // Check license
    const license = db.prepare(`
        SELECT * FROM licenses 
        WHERE user_id = ? AND game_id = ? 
        AND (expires_at IS NULL OR expires_at > datetime('now'))
    `).get(req.user.id, req.params.id);
    
    if (!license) {
        return res.status(403).json({ error: 'No license for this game' });
    }
    
    const versions = db.prepare(`
        SELECT version, changelog, created_at
        FROM versions
        WHERE game_id = ?
        ORDER BY created_at DESC
    `).all(req.params.id);
    
    res.json({ versions });
});

/**
 * GET /api/games/:id/status
 * Check if game cheat is up to date
 */
router.get('/:id/status', authenticate, (req, res) => {
    const { current_version } = req.query;
    
    const game = db.prepare('SELECT latest_version FROM games WHERE id = ?')
                   .get(req.params.id);
    
    if (!game) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    const needsUpdate = current_version !== game.latest_version;
    
    res.json({
        current_version,
        latest_version: game.latest_version,
        needs_update: needsUpdate
    });
});

module.exports = router;

