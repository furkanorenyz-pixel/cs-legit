/**
 * Telegram Channel Monitor for CS2 Updates
 * Monitors a Telegram channel and updates game status when CS2 update is detected
 */

const https = require('https');
const db = require('../database/db');

// Configuration - set via environment variables
const BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN || '';
const CHANNEL_ID = process.env.TELEGRAM_CHANNEL_ID || '';

let lastUpdateId = 0;
let isMonitoring = false;

/**
 * Keywords that indicate a CS2 update (from @cstwoupdate channel)
 */
const UPDATE_KEYWORDS = [
    'обновление counter strike 2',
    'обновления cs2',
    '#cs2update',
    'версия клиента',
    'размер ~',
    'counter strike 2 от',
    'cs2update'
];

/**
 * Check if message contains update keywords
 */
function isUpdateMessage(text) {
    if (!text) return false;
    const lowerText = text.toLowerCase();
    return UPDATE_KEYWORDS.some(keyword => lowerText.includes(keyword));
}

/**
 * Set game status in database
 */
function setGameStatus(gameId, status, message = null) {
    // Create status table if not exists
    db.prepare(`
        CREATE TABLE IF NOT EXISTS game_status (
            game_id TEXT PRIMARY KEY,
            status TEXT DEFAULT 'operational',
            message TEXT,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
            updated_by TEXT
        )
    `).run();
    
    db.prepare(`
        INSERT OR REPLACE INTO game_status (game_id, status, message, updated_at, updated_by)
        VALUES (?, ?, ?, datetime('now'), ?)
    `).run(gameId, status, message, 'telegram');
    
    console.log(`[TelegramMonitor] Set ${gameId} status to: ${status}`);
}

/**
 * Get current game status
 */
function getGameStatus(gameId) {
    try {
        db.prepare(`
            CREATE TABLE IF NOT EXISTS game_status (
                game_id TEXT PRIMARY KEY,
                status TEXT DEFAULT 'operational',
                message TEXT,
                updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
                updated_by TEXT
            )
        `).run();
        
        const status = db.prepare('SELECT * FROM game_status WHERE game_id = ?').get(gameId);
        return status || { game_id: gameId, status: 'operational', message: null };
    } catch (e) {
        return { game_id: gameId, status: 'operational', message: null };
    }
}

/**
 * Poll Telegram for new messages (if bot token is configured)
 */
async function pollTelegram() {
    if (!BOT_TOKEN || !CHANNEL_ID) {
        return; // Telegram not configured
    }
    
    return new Promise((resolve) => {
        const url = `https://api.telegram.org/bot${BOT_TOKEN}/getUpdates?offset=${lastUpdateId + 1}&timeout=30`;
        
        https.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    const json = JSON.parse(data);
                    if (json.ok && json.result) {
                        for (const update of json.result) {
                            lastUpdateId = update.update_id;
                            
                            const message = update.message || update.channel_post;
                            if (message && message.text) {
                                if (isUpdateMessage(message.text)) {
                                    console.log('[TelegramMonitor] CS2 update detected!');
                                    setGameStatus('cs2', 'updating', 'Game update detected via Telegram');
                                }
                            }
                        }
                    }
                } catch (e) {
                    console.error('[TelegramMonitor] Parse error:', e.message);
                }
                resolve();
            });
        }).on('error', (e) => {
            console.error('[TelegramMonitor] Request error:', e.message);
            resolve();
        });
    });
}

/**
 * Start monitoring (if configured)
 */
function startMonitoring() {
    if (!BOT_TOKEN || !CHANNEL_ID) {
        console.log('[TelegramMonitor] Not configured - skipping');
        return;
    }
    
    if (isMonitoring) return;
    isMonitoring = true;
    
    console.log('[TelegramMonitor] Started monitoring');
    
    async function poll() {
        if (!isMonitoring) return;
        await pollTelegram();
        setTimeout(poll, 5000); // Poll every 5 seconds
    }
    
    poll();
}

/**
 * Stop monitoring
 */
function stopMonitoring() {
    isMonitoring = false;
    console.log('[TelegramMonitor] Stopped');
}

module.exports = {
    startMonitoring,
    stopMonitoring,
    setGameStatus,
    getGameStatus,
    isUpdateMessage
};

