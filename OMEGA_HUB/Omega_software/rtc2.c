#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Function to convert BCD to decimal
int bcd_to_dec(int bcd) {
    return ((bcd / 16 * 10) + (bcd % 16));
}

// Function to convert decimal to BCD
int dec_to_bcd(int dec) {
    return ((dec / 10 * 16) + (dec % 10));
}

// Function to set the time on the RTC
void set_time(int bus, int hour, int min, int sec) {
    char buffer[100];
    // Write the time to the RTC using bcd format
    sprintf(buffer, "i2cset -y %d 0x68 0x00 0x%02x", bus, dec_to_bcd(sec));
    system(buffer);
    sprintf(buffer, "i2cset -y %d 0x68 0x01 0x%02x", bus, dec_to_bcd(min));
    system(buffer);
    sprintf(buffer, "i2cset -y %d 0x68 0x02 0x%02x", bus, dec_to_bcd(hour));
    system(buffer);
}

// Function to fetch and set the system time from an NTP server
void set_system_time_from_ntp() {
    // Using ntpdate to set system time; ensure ntpdate is installed
    system("ntpd -q -p pool.ntp.org");
}

// Function to read the current time from the RTC
void read_time(int bus) {
    FILE *fp;
    char buffer[100];
    int bcd_sec, bcd_min, bcd_hour;

    sprintf(buffer, "i2cget -y %d 0x68 0x00 b", bus);
    fp = popen(buffer, "r");
    fscanf(fp, "%x", &bcd_sec);
    pclose(fp);

    sprintf(buffer, "i2cget -y %d 0x68 0x01 b", bus);
    fp = popen(buffer, "r");
    fscanf(fp, "%x", &bcd_min);
    pclose(fp);

    sprintf(buffer, "i2cget -y %d 0x68 0x02 b", bus);
    fp = popen(buffer, "r");
    fscanf(fp, "%x", &bcd_hour);
    pclose(fp);

    printf("Current time: %02d:%02d:%02d\n", bcd_to_dec(bcd_hour), bcd_to_dec(bcd_min), bcd_to_dec(bcd_sec));
}

int main() {
    int bus = 0; // Set this to your I2C bus number

    // Fetch and set the system time from an NTP server
    set_system_time_from_ntp();

    // Get the system time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    // Set the RTC time
    set_time(bus, tm->tm_hour, tm->tm_min, tm->tm_sec);

    while (1) {
        read_time(bus);
        sleep(10); // Delay for 10 seconds
    }

    return 0;
}


