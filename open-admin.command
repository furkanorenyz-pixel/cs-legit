#!/bin/bash
# CS-Legit Admin Panel - Quick Access
# Double-click this file to open admin panel

clear
echo "╔════════════════════════════════════════╗"
echo "║     CS-LEGIT ADMIN PANEL               ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Server details
SERVER="138.124.0.8"
PASSWORD="mmE28jaX99"

# Kill existing tunnels
pkill -f "ssh -L 8080" 2>/dev/null

echo "🔐 Connecting to server..."

# Start SSH tunnel in background
sshpass -p "$PASSWORD" ssh -o StrictHostKeyChecking=no -L 8080:127.0.0.1:80 -N root@$SERVER &
SSH_PID=$!

sleep 2

# Check if tunnel is working
if kill -0 $SSH_PID 2>/dev/null; then
    echo "✅ Connected!"
    echo ""
    echo "🌐 Opening admin panel..."
    sleep 1
    open "http://localhost:8080/panel/"
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Admin Panel: http://localhost:8080/panel/"
    echo ""
    echo "Login: admin"
    echo "Password: admin123"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "Press Ctrl+C to disconnect..."
    wait $SSH_PID
else
    echo "❌ Connection failed!"
    echo ""
    echo "Make sure sshpass is installed:"
    echo "  brew install hudochenkov/sshpass/sshpass"
fi

