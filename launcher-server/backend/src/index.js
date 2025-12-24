/**
 * Cheat Launcher Server - Main Entry Point
 */

require('dotenv').config();

const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const path = require('path');

const authRoutes = require('./routes/auth');
const gamesRoutes = require('./routes/games');
const downloadRoutes = require('./routes/download');
const offsetsRoutes = require('./routes/offsets');
const adminRoutes = require('./routes/admin');

const app = express();

// Serve admin panel static files
app.use('/panel', express.static(path.join(__dirname, '../../admin-panel')));

// Serve public website (main page)
app.use(express.static(path.join(__dirname, '../../public')));

// ============================================
// Middleware
// ============================================

app.use(helmet());
app.use(cors());
app.use(express.json());

// General rate limiting (generous for normal use)
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 500, // 500 requests per 15 min (enough for auto-refresh)
    message: { error: 'Too many requests, please try again later.' }
});
app.use(limiter);

// Auth rate limiting (stricter to prevent brute force)
const authLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes  
    max: 30, // 30 auth attempts per 15 min
    message: { error: 'Too many login attempts, please wait.' }
});
app.use('/api/auth/login', authLimiter);

// Stricter limit for downloads
const downloadLimiter = rateLimit({
    windowMs: 60 * 60 * 1000, // 1 hour
    max: 1000, // Increased limit for updates/testing
    message: { error: 'Download limit exceeded.' },
    skip: (req) => {
        // Skip rate limit for launcher updates to allow auto-updates
        return req.path === '/launcher';
    }
});
app.use('/api/download', downloadLimiter);

// ============================================
// Routes
// ============================================

app.use('/api/auth', authRoutes);
app.use('/api/games', gamesRoutes);
app.use('/api/download', downloadRoutes);
app.use('/api/offsets', offsetsRoutes);
app.use('/api/admin', adminRoutes);

// Health check endpoint
app.get('/health', (req, res) => {
    const db = require('./database/db');
    
    try {
        // Test database connectivity
        db.prepare('SELECT 1').get();
        
        // Get basic stats
        const userCount = db.prepare('SELECT COUNT(*) as count FROM users').get();
        const gameCount = db.prepare('SELECT COUNT(*) as count FROM games WHERE is_active = 1').get();
        
        res.json({ 
            status: 'ok', 
            version: '1.0.0',
            uptime: process.uptime(),
            timestamp: new Date().toISOString(),
            database: 'connected',
            stats: {
                users: userCount.count,
                games: gameCount.count
            }
        });
    } catch (error) {
        console.error('[Health Check] Error:', error.message);
        res.status(503).json({ 
            status: 'error', 
            message: 'Service unavailable',
            database: 'disconnected'
        });
    }
});

// Detailed metrics endpoint (for monitoring)
app.get('/api/metrics', (req, res) => {
    const db = require('./database/db');
    
    try {
        const stats = {
            users: {
                total: db.prepare('SELECT COUNT(*) as count FROM users').get().count,
                admins: db.prepare('SELECT COUNT(*) as count FROM users WHERE is_admin = 1').get().count
            },
            licenses: {
                total: db.prepare('SELECT COUNT(*) as count FROM licenses').get().count,
                active: db.prepare('SELECT COUNT(*) as count FROM licenses WHERE user_id IS NOT NULL').get().count,
                unused: db.prepare('SELECT COUNT(*) as count FROM licenses WHERE user_id IS NULL').get().count
            },
            games: db.prepare('SELECT COUNT(*) as count FROM games WHERE is_active = 1').get().count,
            uptime: process.uptime(),
            memory: process.memoryUsage(),
            timestamp: new Date().toISOString()
        };
        
        res.json(stats);
    } catch (error) {
        console.error('[Metrics] Error:', error.message);
        res.status(500).json({ error: 'Failed to fetch metrics' });
    }
});

// ============================================
// Error Handler
// ============================================

app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({ error: 'Internal server error' });
});

// ============================================
// Start Server
// ============================================

const PORT = process.env.PORT || 3000;
const HOST = process.env.HOST || '0.0.0.0';

// Start Telegram monitoring
const { startMonitoring } = require('./services/telegramMonitor');

app.listen(PORT, HOST, () => {
    console.log(`🚀 Launcher Server running on http://${HOST}:${PORT}`);
    console.log(`📁 Storage: ${path.resolve(process.env.STORAGE_PATH || '../storage')}`);
    console.log(`🌐 Public website: http://single-project.duckdns.org`);
    
    // Start monitoring @cstwoupdate for CS2 updates
    startMonitoring();
    console.log(`📱 Telegram monitor started - watching @cstwoupdate`);
});

