#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <stdlib.h>
#include <string.h>

int get_connected_devices(char** devices, int max_device_number) {
    int fd;

    int device_count = 0; // Number of connected devices

    int i;

    

    // Allocate memory for each device name (including space for "/dev/ttyUSB" and the null terminator)
    for (i = 0; i < max_device_number; i++) {
        devices[i] = (char *)malloc(sizeof(char) * (strlen("/dev/ttyUSB") + 2));
        if (devices[i] == NULL) {
            perror("Memory allocation failed");
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++) {
                free(devices[j]);
            }
            free(devices);
            return 1;
        }
    }

    // Copy the "/dev/ttyUSB" prefix to each device name and check for connected devices
    for (i = 0; i < max_device_number; i++) {
        sprintf(devices[i], "/dev/ttyUSB%d", i); // Append the port number to the device name
        fd = open(devices[i], O_RDWR | O_NONBLOCK);
        if (fd != -1) {
            close(fd);
            device_count++;
        } else {
            // If device not found, free the memory for this device name
            free(devices[i]);
            devices[i] = NULL;
        }
    }
    return 0;
}

int main() {
    int i;
    int max_device_number = 10;
    char **devices; // Declare a pointer to pointer to char for the list of device names
    // Allocate memory for the device name pointers
    devices = (char **)malloc(sizeof(char *) * max_device_number);
    if (devices == NULL) {
        perror("Memory allocation failed");
        return 1;
    }

    get_connected_devices(devices, max_device_number);
    // Print the connected devices
    printf("Connected devices:\n");
    for (i = 0; i < max_device_number; i++) {
        if (devices[i] != NULL) {
            printf("%s\n", devices[i]);
        }
    }
    for (int j = 0; j < max_device_number; j++) {
        free(devices[j]);
    }
    // Free the allocated memory for the list of device names
    free(devices);

    return 0;
}
