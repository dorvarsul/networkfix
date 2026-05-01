#!/bin/bash
# lps_hang.sh - Recovery for "failed to leave lps" errors
# This script logs the error and performs a modprobe reload of the 8821ce module

LOG_FILE="wifi_fix.log"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

log_action() {
    echo "[$TIMESTAMP] [SCRIPT] [lps_hang] $1" | tee -a "$LOG_FILE"
}

log_action "Detected: failed to leave lps"
log_action "Starting module reload for 8821ce..."

# Remove and reload module
log_action "Removing 8821ce module..."
modprobe -r 8821ce

sleep 2

log_action "Reloading 8821ce module..."
modprobe 8821ce

log_action "Module reload completed"
