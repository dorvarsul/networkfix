#!/bin/bash
# fw_fail.sh - Recovery for "download firmware failed" errors
# This script logs the error and performs a modprobe reload of the 8821ce module

LOG_FILE="wifi_fix.log"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

log_action() {
    echo "[$TIMESTAMP] [SCRIPT] [fw_fail] $1" | tee -a "$LOG_FILE"
}

log_action "Detected: download firmware failed"
log_action "Starting module reload for 8821ce..."

# Remove and reload module
log_action "Removing 8821ce module..."
modprobe -r 8821ce

sleep 2

log_action "Reloading 8821ce module..."
modprobe 8821ce

log_action "Module reload completed"
