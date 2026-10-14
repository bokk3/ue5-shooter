#!/bin/bash
# ==============================================================================
# Noble Fermi - Linux Dedicated Server VPS Runner
# Automatically handles auto-restart on crash, logging, and port configuration.
# ==============================================================================

PORT=${PORT:-7777}
QUERY_PORT=${QUERY_PORT:-27015}
MAX_PLAYERS=${MAX_PLAYERS:-16}
TICK_RATE=${TICK_RATE:-128}
SERVER_NAME=${SERVER_NAME:-"Noble Fermi Linux Dedicated Server"}
MAP_NAME=${MAP_NAME:-"/Game/Maps/TDM_Crash"}

if [ -f "./NobleFermiServer" ]; then
    SERVER_BIN="./NobleFermiServer"
    EXTRA_FLAGS=""
else
    SERVER_BIN="./NobleFermi"
    EXTRA_FLAGS="-server -nullrhi"
fi

echo "================================================================="
echo " Starting Noble Fermi Dedicated Server on Linux VPS"
echo " Port: $PORT | Query Port: $QUERY_PORT | Tickrate: ${TICK_RATE}Hz"
echo " Server Name: $SERVER_NAME"
echo "================================================================="

while true; do
    echo "[$(date)] Launching NobleFermiServer binary..."
    
    $SERVER_BIN "$MAP_NAME" \
        $EXTRA_FLAGS \
        -log \
        -unattended \
        -Port=$PORT \
        -QueryPort=$QUERY_PORT \
        -MaxPlayers=$MAX_PLAYERS \
        -TickRate=$TICK_RATE \
        -ServerName="$SERVER_NAME" \
        -nohome
        
    EXIT_CODE=$?
    echo "[$(date)] Server stopped with exit code $EXIT_CODE."
    
    if [ $EXIT_CODE -eq 0 ]; then
        echo "Graceful shutdown requested. Exiting."
        break
    else
        echo "Server crashed or closed unexpectedly! Restarting in 5 seconds..."
        sleep 5
    fi
done
