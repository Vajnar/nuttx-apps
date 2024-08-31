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
#include <nuttx/sensors/qencoder.h>

int main(int argc, FAR char *argv[])
{
    struct pwm_info_s info;
    int fd;

    // GPIO 0
    bool outvalue = true;
    bool invalue;
    fd = open("/dev/gpio0", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    puts("Writing 1 to /dev/gpio0");
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);

    // GPIO 1
    outvalue = false;
    fd = open("/dev/gpio1", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    puts("Writing 0 to /dev/gpio1");
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    // Quadratic Encoder 0
    int32_t pos;
    int fd_qe0;
    fd_qe0 = open("/dev/qe0", O_RDWR);
    ioctl(fd_qe0, QEIOC_POSITION, &pos);
    printf("QE0 Postition=%ld\n", pos);

    // PWM 0
    memset(&info, 0, sizeof(struct pwm_info_s));
    info.frequency = 1000;
    info.duty = b16divi(uitoub16((uint8_t)20) - 1, 100);

    fd = open("/dev/pwm0", O_RDONLY);
    ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
    ioctl(fd, PWMIOC_START, 0);
    puts("Starting PWM pulse train");
    sleep(10);
    ioctl(fd, PWMIOC_STOP, 0);
    puts("Stopping PWM pulse train");
    close(fd);
    fflush(stdout);

    // GPIO 0 - Low
    outvalue = false;
    fd = open("/dev/gpio0", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    puts("Writing 0 to /dev/gpio0");
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    // Quadratic Encoder 0
    ioctl(fd_qe0, QEIOC_POSITION, &pos);
    printf("QE0 Postition=%ld\n", pos);
    close(fd_qe0);

    return OK;
}
