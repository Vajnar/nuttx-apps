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
    bool outvalue = false;
    bool invalue;
    fd = open("/dev/gpio0", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio0", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);

    // GPIO 1
    outvalue = true;
    fd = open("/dev/gpio1", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio1", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    // GPIO 2
    outvalue = true;
    fd = open("/dev/gpio2", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio2", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);

    // GPIO 3
    outvalue = false;
    fd = open("/dev/gpio3", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio3", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    // Quadratic Encoder 0
    int32_t pos;
    int fd_qe0;
    fd_qe0 = open("/dev/qe0", O_RDWR);
    ioctl(fd_qe0, QEIOC_POSITION, &pos);
    printf("QE0 Position=%ld\n", pos);

    // Quadratic Encoder 1
    int fd_qe1;
    fd_qe1 = open("/dev/qe1", O_RDWR);
    ioctl(fd_qe1, QEIOC_POSITION, &pos);
    printf("QE1 Position=%ld\n", pos);

    // PWM 1
    memset(&info, 0, sizeof(struct pwm_info_s));
    info.frequency = 8000;
    info.duty = b16divi(uitoub16((uint8_t)20) - 1, 100);

    printf("duty = %lu\n", info.duty);
    // int fd_pwm0 = open("/dev/pwm0", O_RDONLY);
    // ioctl(fd_pwm0, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
    // ioctl(fd_pwm0, PWMIOC_START, 0);

    int fd_pwm1 = open("/dev/pwm1", O_RDONLY);
    ioctl(fd_pwm1, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
    ioctl(fd_pwm1, PWMIOC_START, 0);
    puts("Starting PWM pulse train");
    int i = 100;
    while (i--) {
        // ioctl(fd_qe0, QEIOC_POSITION, &pos);
        // printf("QE0 Position=%ld, pass=%d\n", pos, i);
        ioctl(fd_qe1, QEIOC_POSITION, &pos);
        ioctl(fd_qe1, QEIOC_RESET, 0);
        double p = 90 - 1 * pos;
        info.duty = b16divi(uitoub16((uint8_t)abs((int32_t)p)) - 1, 100);
        if (info.duty > 32767) {
            info.duty = 32767;
        } else if (info.duty < 13107) {
            info.duty = 13107;
        }
        ioctl(fd_pwm1, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
        printf("QE1 Position=%ld, info.duty=%lu, p=%lf\n", pos, info.duty, p);
        usleep(100000);
    }
    // ioctl(fd_pwm0, PWMIOC_STOP, 0);
    ioctl(fd_pwm1, PWMIOC_STOP, 0);
    puts("Stopping PWM pulse train");
    // close(fd_pwm0);
    close(fd_pwm1);
    fflush(stdout);
    sleep(1);

    // GPIO 0
    outvalue = false;
    fd = open("/dev/gpio0", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio0", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);

    // GPIO 1
    outvalue = false;
    fd = open("/dev/gpio1", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio1", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    // GPIO 2
    outvalue = false;
    fd = open("/dev/gpio2", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio2", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);

    // GPIO 3
    outvalue = false;
    fd = open("/dev/gpio3", O_RDWR);
    ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);

    ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
    printf("Writing %u to /dev/gpio3", (unsigned int)outvalue);
    ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
    printf("Verify: Value=%u\n", (unsigned int)invalue);
    close(fd);
    fflush(stdout);

    close(fd_qe0);
    close(fd_qe1);

    return OK;
}
