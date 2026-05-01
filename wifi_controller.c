#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#define KMSG_PATH "/dev/kmsg"
#define LOG_FILE "wifi_fix.log"
#define KMSG_BUF_SIZE 2048
#define CMD_BUF_SIZE 512
#define OPERSTATE_PATH "/sys/class/net/wlo0/operstate"

/* Pattern to script mappings */
typedef struct {
    const char *pattern;
    const char *script;
} ErrorMapping;

static const ErrorMapping error_map[] = {
    {"failed to send h2c command", "./fixes/h2c_timeout.sh"},
    {"failed to leave lps", "./fixes/lps_hang.sh"},
    {"failed to poll register", "./fixes/pcie_error.sh"},
    {"download firmware failed", "./fixes/fw_fail.sh"}
};

#define ERROR_MAP_COUNT (sizeof(error_map) / sizeof(error_map[0]))

/* Global log file handle */
FILE *log_fp = NULL;

/**
 * Get current timestamp as string for logging
 */
static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", local);
}

/**
 * Log message to file and stdout
 */
static void log_message(const char *level, const char *message) {
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    char log_line[512];
    snprintf(log_line, sizeof(log_line), "[%s] [%s] %s\n", timestamp, level, message);
    
    printf("%s", log_line);
    
    if (log_fp) {
        fprintf(log_fp, "%s", log_line);
        fflush(log_fp);
    }
}

/**
 * Initialize log file
 */
static int init_logging(void) {
    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        perror("Failed to open log file");
        return -1;
    }
    
    log_message("INFO", "=== WiFi Recovery Tool Started ===");
    return 0;
}

/**
 * Cleanup logging
 */
static void cleanup_logging(void) {
    if (log_fp) {
        log_message("INFO", "=== WiFi Recovery Tool Stopped ===");
        fclose(log_fp);
    }
}

/**
 * Execute a bash script and log the result
 */
static int execute_script(const char *script_path) {
    char cmd_buf[CMD_BUF_SIZE];
    char msg[256];
    
    snprintf(cmd_buf, sizeof(cmd_buf), "/bin/bash %s", script_path);
    snprintf(msg, sizeof(msg), "Executing script: %s", script_path);
    log_message("ACTION", msg);
    
    int result = system(cmd_buf);
    
    if (result == 0) {
        snprintf(msg, sizeof(msg), "Script executed successfully: %s", script_path);
        log_message("SUCCESS", msg);
    } else {
        snprintf(msg, sizeof(msg), "Script failed with code %d: %s", result, script_path);
        log_message("ERROR", msg);
    }
    
    return result;
}

/**
 * Check if network interface is down
 */
static int is_interface_down(void) {
    FILE *fp = fopen(OPERSTATE_PATH, "r");
    if (!fp) {
        log_message("WARN", "Could not check interface state");
        return 0;
    }
    
    char state[32];
    if (fgets(state, sizeof(state), fp) == NULL) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    
    /* Remove trailing newline */
    state[strcspn(state, "\n")] = '\0';
    
    return strcmp(state, "down") == 0;
}

/**
 * Scan a single log line for error patterns and execute matching script
 */
static int scan_and_execute(const char *line) {
    for (unsigned int i = 0; i < ERROR_MAP_COUNT; i++) {
        if (strstr(line, error_map[i].pattern)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Pattern detected: %s", error_map[i].pattern);
            log_message("DETECT", msg);
            
            return execute_script(error_map[i].script);
        }
    }
    
    return -1; /* No pattern matched */
}

/**
 * Read from /dev/kmsg in non-blocking mode
 * Seeks to end to read only recent 2KB of logs
 */
static int monitor_and_recover(void) {
    int fd = open(KMSG_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        log_message("ERROR", "Failed to open /dev/kmsg");
        return -1;
    }
    
    /* Seek to end to analyze only recent logs */
    if (lseek(fd, 0, SEEK_END) < 0) {
        log_message("ERROR", "Failed to seek to end of /dev/kmsg");
        close(fd);
        return -1;
    }
    
    log_message("INFO", "Monitoring kernel logs for error patterns...");
    
    char buf[KMSG_BUF_SIZE];
    int pattern_found = 0;
    
    /* Read available data (non-blocking) */
    ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
    
    if (bytes > 0) {
        buf[bytes] = '\0';
        
        /* Split buffer by lines and scan each */
        char *line = strtok(buf, "\n");
        while (line != NULL) {
            if (strlen(line) > 0) {
                if (scan_and_execute(line) >= 0) {
                    pattern_found = 1;
                    break;
                }
            }
            line = strtok(NULL, "\n");
        }
    } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        log_message("ERROR", "Failed to read from /dev/kmsg");
    }
    
    close(fd);
    
    /* Fallback: If no pattern found, check if interface is down */
    if (!pattern_found) {
        if (is_interface_down()) {
            log_message("DETECT", "Interface wlo0 is down - executing generic reset");
            execute_script("./fixes/generic_reset.sh");
        } else {
            log_message("INFO", "No error patterns detected and interface is up");
        }
    }
    
    return 0;
}

/**
 * Main entry point
 */
int main(void) {
    /* Check for root privileges */
    if (getuid() != 0) {
        fprintf(stderr, "Error: This tool must be run as root (sudo).\n");
        return 1;
    }
    
    /* Initialize logging */
    if (init_logging() < 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }
    
    /* Run recovery logic */
    int result = monitor_and_recover();
    
    /* Cleanup */
    cleanup_logging();
    
    return result;
}
