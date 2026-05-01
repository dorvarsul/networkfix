#!/bin/bash
# generic_reset.sh - Generic network interface reset for Realtek 8821CE
# This script performs a complete interface reset with rfkill and NetworkManager restart

set -e

LOG_FILE="wifi_fix.log"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

log_action() {
    echo "[$TIMESTAMP] [SCRIPT] [generic_reset] $1" | tee -a "$LOG_FILE"
}

log_action "Starting generic interface reset..."

# Bring interface down
log_action "Bringing down wlo0..."
ip link set wlo0 down

# Use rfkill to block and unblock wifi
log_action "Using rfkill to block wifi..."
rfkill block wifi

sleep 1

log_action "Using rfkill to unblock wifi..."
rfkill unblock wifi

# Flush IP route cache
log_action "Flushing IP route cache..."
ip route flush cache

# Bring interface back up
log_action "Bringing up wlo0..."
ip link set wlo0 up

# Restart NetworkManager
log_action "Restarting NetworkManager..."
systemctl restart NetworkManager

log_action "Generic reset completed successfully"
