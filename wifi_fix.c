#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define KMSG_PATH "/dev/kmsg"
#define BUF_SIZE 4096

// Specific red flags for the 8821ce driver
const char *ERRORS[] = {
    "failed to send h2c command",
    "firmware failed to leave lps",
    "failed to get tx report",
    "timed out to flush queue"
};

void execute(const char *cmd) {
    printf("[RUNNING]: %s\n", cmd);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "[ERROR]: Command failed with code %d\n", result);
    }
}

void analyze_line(char *line) {
    if (strstr(line, "8821ce") || strstr(line, "rtw88")) {
        printf("[LOG]: %s\n", line);

        for (int i = 0; i < 4; i++) {
            if (strstr(line, ERRORS[i])) {
                printf("\a\n!!! CRITICAL FAILURE DETECTED: %s !!!\n", ERRORS[i]);
                printf("Suggested Action: Full Driver Reload Required (Option 2).\n\n");
            }
        }
    }
}

void monitor_kernel_logs() {
    int fd = open(KMSG_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open /dev/kmsg");
        return;
    }

    // Seek to the end of the buffer to only listen for NEW events
    lseek(fd, 0, SEEK_END);

    char buf[BUF_SIZE];
    printf("\n--- Live 8821ce Monitor Started ---\n");
    printf("Listening for hardware/driver desync... (Ctrl+C to stop)\n\n");

    while (1) {
        ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
        if (bytes > 0) {
            buf[bytes] = '\0';
            analyze_line(buf);
        } else if (bytes < 0 && errno != EAGAIN) {
            perror("Read error");
            break;
        }
        usleep(100000); // 100ms poll
    }
    close(fd);
}

int main() {
    if (getuid() != 0) {
        printf("Error: This tool must be run as root (sudo).\n");
        return 1;
    }

    int choice;
    while (1) {
        printf("\n--- 8821ce Recovery Tool ---\n");
        printf("1. Quick Interface Reset\n");
        printf("2. Deep Driver Reload (Recovery)\n");
        printf("3. Check Kernel Logs (dmesg)\n");
        printf("4. Exit\n");
        printf("5. Enable Verbose Mode & Start Live Monitor\n");
        printf("Selection: ");
        
        if (scanf("%d", &choice) != 1) break;

        switch (choice) {
            case 1:
                printf("\nResetting interface...\n");
                execute("ip link set wlo0 down");
                sleep(1);
                execute("rfkill unblock wifi");
                execute("ip link set wlo0 up"); // Note: Fixed potential typo from wlo1 to wlo0
                break;

            case 2:
                printf("\nPerforming Deep Driver Reload...\n");
                execute("systemctl stop NetworkManager");
                execute("modprobe -r 8821ce");
                sleep(2);
                execute("modprobe 8821ce");
                sleep(1);
                execute("systemctl start NetworkManager");
                break;

            case 3:
                printf("\nFiltering dmesg for 8821ce errors:\n");
                execute("dmesg | grep -i 8821ce | tail -n 20");
                break;

            case 4:
                return 0;

            case 5:
                printf("\nEnabling verbose driver-kernel communication...\n");
                execute("modprobe -r 8821ce");
                sleep(1);
                execute("modprobe 8821ce rtw_drv_log_level=5");
                monitor_kernel_logs();
                break;

            default:
                printf("Invalid choice.\n");
        }
    }

    return 0;
}