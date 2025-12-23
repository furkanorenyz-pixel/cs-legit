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

// ============================================
// Middleware
// ============================================

app.use(helmet());
app.use(cors());
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // limit each IP to 100 requests per windowMs
    message: { error: 'Too many requests, please try again later.' }
});
app.use(limiter);

// Stricter limit for downloads
const downloadLimiter = rateLimit({
    windowMs: 60 * 60 * 1000, // 1 hour
    max: 10, // 10 downloads per hour
    message: { error: 'Download limit exceeded.' }
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

// Health check
app.get('/health', (req, res) => {
    res.json({ status: 'ok', version: '1.0.0' });
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

app.listen(PORT, HOST, () => {
    console.log(`🚀 Launcher Server running on http://${HOST}:${PORT}`);
    console.log(`📁 Storage: ${path.resolve(process.env.STORAGE_PATH || '../storage')}`);
});

