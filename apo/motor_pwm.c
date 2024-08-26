#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <inttypes.h>

#include <nuttx/timers/pwm.h>
#include <nuttx/ioexpander/gpio.h>

int main(int argc, FAR char *argv[])
{
    struct pwm_info_s info;
    int fd;

    // PWM
    memset(&info, 0, sizeof(struct pwm_info_s));
    info.frequency = 1000;
    info.duty = b16divi(uitoub16((uint8_t)10) - 1, 100);

    fd = open("/dev/pwm1", O_RDONLY);
    ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
    ioctl(fd, PWMIOC_START, 0);
    puts("Starting PWM pulse train");
    sleep(10);
    ioctl(fd, PWMIOC_STOP, 0);
    puts("Stopping PWM pulse train");
    close(fd);
    fflush(stdout);

    // GPIO
    bool outvalue = true;
    bool invalue;
    fd = open("/dev/gpio0", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    puts("Writing 1 to /dev/gpio0");
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);

    sleep(10);

    outvalue = false;
    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    puts("Writing 0 to /dev/gpio0");
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);
    return OK;
}
