# WiFi Recovery Tool for Realtek 8821CE

A modular network recovery system for Linux systems using the Realtek 8821CE WiFi chipset. This tool automatically detects driver errors from kernel logs and executes targeted recovery scripts.

## Features

- **Kernel Log Monitoring**: Reads `/dev/kmsg` in non-blocking mode to detect recent errors
- **Pattern Matching**: Maps specific error messages to targeted recovery scripts
- **Automatic Recovery**: Executes appropriate recovery actions without user intervention
- **Comprehensive Logging**: All actions are timestamped and logged to `wifi_fix.log` and terminal output
- **Root Privilege Verification**: Ensures tool runs with required permissions
- **Safe Command Building**: Uses `snprintf()` for secure string handling

## Architecture

```
networkfix/
├── wifi_controller.c       # Main C controller (entry point)
├── Makefile                # Build configuration
├── wifi_fix.log            # Log file (created at runtime)
└── fixes/
    ├── generic_reset.sh    # Generic interface reset
    ├── h2c_timeout.sh      # Recovery for h2c command timeouts
    ├── lps_hang.sh         # Recovery for LPS (Low Power State) hangs
    ├── pcie_error.sh       # Recovery for PCIe register polling errors
    └── fw_fail.sh          # Recovery for firmware download failures
```

## Error Pattern Mapping

The tool automatically detects and responds to these kernel errors:

| Error Pattern | Script | Recovery Action |
|---------------|--------|-----------------|
| `failed to send h2c command` | `h2c_timeout.sh` | Reload module |
| `failed to leave lps` | `lps_hang.sh` | Reload module |
| `failed to poll register` | `pcie_error.sh` | Reload module |
| `download firmware failed` | `fw_fail.sh` | Reload module |
| Interface down (fallback) | `generic_reset.sh` | Full interface reset + NetworkManager restart |

## Building

### Prerequisites
- GCC compiler
- Make utility
- Linux kernel headers (for `/dev/kmsg` access)
- Root access (required for execution)

### Compilation

```bash
make           # Build the tool
make clean     # Remove build artifacts
make help      # Show available targets
```

## Usage

### Basic Execution

```bash
sudo ./wifi_controller
```

This will:
1. Read the most recent 2KB of kernel logs from `/dev/kmsg`
2. Scan for known error patterns
3. Execute the matching recovery script if a pattern is found
4. If no pattern is found, check if interface `wlo0` is down
5. Log all actions with timestamps to `wifi_fix.log`

### Log File

All activities are logged to `wifi_fix.log`:

```bash
cat wifi_fix.log                    # View all logs
tail -f wifi_fix.log                # Follow logs in real-time
grep ERROR wifi_fix.log             # Filter error messages
grep "Pattern detected" wifi_fix.log # See detected errors
```

## Implementation Details

### C Controller Features

1. **Non-blocking Kernel Log Reading**
   - Opens `/dev/kmsg` with `O_NONBLOCK` flag
   - Seeks to end of buffer using `SEEK_END` to analyze only recent logs
   - Handles `EAGAIN` error gracefully

2. **Root Privilege Check**
   - Verifies `getuid() == 0` at startup
   - Exits with error if not running as root

3. **Safe String Handling**
   - Uses `snprintf()` for all command building (prevents buffer overflows)
   - Bounded buffer operations throughout

4. **Timestamped Logging**
   - Logs every action with ISO 8601 timestamp
   - Dual output: file and terminal
   - Color-coded log levels (INFO, ACTION, DETECT, SUCCESS, ERROR, WARN)

5. **Fallback Logic**
   - Checks `/sys/class/net/wlo0/operstate` for interface state
   - Triggers `generic_reset.sh` if interface is down

### Bash Scripts

All recovery scripts in `fixes/`:
- Log their specific error type
- Execute module reload: `modprobe -r 8821ce && modprobe 8821ce`
- Include 2-second delay between remove and reload
- Append timestamps to `wifi_fix.log`

**generic_reset.sh** performs:
1. Bring interface down
2. Block WiFi via rfkill
3. Unblock WiFi via rfkill
4. Flush IP route cache
5. Bring interface up
6. Restart NetworkManager

## Example Output

```
[2026-05-01 14:23:45] [INFO] === WiFi Recovery Tool Started ===
[2026-05-01 14:23:45] [INFO] Monitoring kernel logs for error patterns...
[2026-05-01 14:23:46] [DETECT] Pattern detected: failed to send h2c command
[2026-05-01 14:23:46] [ACTION] Executing script: ./fixes/h2c_timeout.sh
[2026-05-01 14:23:46] [SCRIPT] [h2c_timeout] Detected: failed to send h2c command
[2026-05-01 14:23:46] [SCRIPT] [h2c_timeout] Starting module reload for 8821ce...
[2026-05-01 14:23:46] [SCRIPT] [h2c_timeout] Removing 8821ce module...
[2026-05-01 14:23:48] [SCRIPT] [h2c_timeout] Reloading 8821ce module...
[2026-05-01 14:23:49] [SCRIPT] [h2c_timeout] Module reload completed
[2026-05-01 14:23:49] [SUCCESS] Script executed successfully: ./fixes/h2c_timeout.sh
[2026-05-01 14:23:49] [INFO] === WiFi Recovery Tool Stopped ===
```

## Troubleshooting

### "Permission denied" Error
- Ensure you run with `sudo`: `sudo ./wifi_controller`

### "Failed to open /dev/kmsg"
- Check that `/dev/kmsg` exists (standard on all modern Linux systems)
- Some systems may restrict kernel log access; try running as root

### Scripts Not Found
- Ensure `fixes/` directory exists in the same directory as `wifi_controller`
- Verify scripts are executable: `chmod +x fixes/*.sh`

### No Output from Logs
- Check if there are actual kernel messages: `dmesg | grep 8821`
- Verify the pattern matches exactly (case-sensitive)

## Security Considerations

- **Root Execution**: Tool requires root privileges to access kernel logs and control network interfaces
- **Buffer Safety**: All string operations use bounded functions (`snprintf()`)
- **Input Validation**: Pattern matching is string-based only; no command injection vectors
- **No Credential Storage**: No passwords or sensitive data are stored

## Development

### Adding New Error Patterns

1. Add pattern and script mapping to `error_map[]` in `wifi_controller.c`
2. Create new script in `fixes/` directory
3. Make script executable: `chmod +x fixes/new_script.sh`
4. Recompile: `make clean && make`

### Example Addition

```c
// In error_map[] array:
{"your error pattern", "./fixes/your_recovery.sh"}
```

## Support

For issues with the Realtek 8821CE driver, check:
- Kernel logs: `dmesg | grep 8821`
- Module status: `modprobe -c | grep 8821`
- Network interface: `ip link show wlo0`
