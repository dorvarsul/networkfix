# Quick Start Guide

## Build the Tool
```bash
cd /home/dor/dev/personal/networkfix
make
```

## Run the Tool
```bash
sudo ./wifi_controller
```

## Check Logs
```bash
cat wifi_fix.log          # View all logs
tail -f wifi_fix.log      # Watch logs in real-time
grep ERROR wifi_fix.log   # Find errors only
```

## Project Files

### Main Controller
- `wifi_controller.c` - C source code with kernel log monitoring
- `wifi_controller` - Compiled binary (executable)
- `Makefile` - Build configuration

### Recovery Scripts (in fixes/ directory)
- `generic_reset.sh` - Full interface reset with rfkill and NetworkManager restart
- `h2c_timeout.sh` - Recovery for "failed to send h2c command" errors
- `lps_hang.sh` - Recovery for "failed to leave lps" errors  
- `pcie_error.sh` - Recovery for "failed to poll register" errors
- `fw_fail.sh` - Recovery for "download firmware failed" errors

## Key Features Implemented

✓ Root privilege check at startup
✓ Non-blocking kernel log reading from /dev/kmsg
✓ Seek to end of buffer for recent 2KB analysis
✓ EAGAIN error handling
✓ snprintf() for safe string building
✓ Pattern to script mapping system
✓ Timestamped logging (file + terminal)
✓ Interface state fallback check
✓ 4 error patterns + generic reset logic

## File Structure
```
networkfix/
├── wifi_controller.c       (5.8K) - Main controller
├── wifi_controller         (17K)  - Compiled binary
├── Makefile                (824B) - Build system
├── README.md               (6.4K) - Full documentation
├── QUICKSTART.md           (this file)
├── wifi_fix.log            - Runtime log (created on first run)
└── fixes/
    ├── generic_reset.sh    (960B)
    ├── h2c_timeout.sh      (605B)
    ├── lps_hang.sh         (585B)
    ├── pcie_error.sh       (597B)
    └── fw_fail.sh          (593B)
```

## Compilation Summary
- **Compiler**: GCC with -Wall -Wextra -O2 -std=c99
- **Output**: Single executable, no external dependencies
- **Status**: Builds cleanly with no warnings

## Next Steps
1. Review `wifi_controller.c` for implementation details
2. Customize `fixes/` scripts if needed
3. Test with `sudo ./wifi_controller`
4. Check `wifi_fix.log` for results
