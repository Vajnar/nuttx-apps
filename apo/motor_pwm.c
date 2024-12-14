#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/timers/pwm.h>
#include <nuttx/ioexpander/gpio.h>
#include <nuttx/sensors/qencoder.h>

#include <sys/ioctl.h>

int main(int argc, char *argv[])
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

    const int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    const int bool_true = 1;
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(bool_true));

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(5000);

    bind(tcp_socket, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    listen(tcp_socket, 1);
    int tcp_conn_socket = accept(tcp_socket, NULL, NULL);
    setsockopt(tcp_conn_socket, SOL_SOCKET, SO_KEEPALIVE, &bool_true, sizeof(bool_true));
    setsockopt(tcp_conn_socket, IPPROTO_TCP, TCP_NODELAY, &bool_true, sizeof(bool_true));
    int flags = fcntl(tcp_conn_socket, F_GETFL, 0);
    fcntl(tcp_conn_socket, F_SETFL, flags | O_NONBLOCK);

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

    while(1) {
        ioctl(fd_qe1, QEIOC_POSITION, &pos);
        ioctl(fd_qe1, QEIOC_RESET, 0);
        printf("QE1 Position=%ld\n", pos);

        char incoming[64];
        int num_read = read(tcp_conn_socket, &incoming, sizeof(incoming));
        if (num_read < 0 && errno == EWOULDBLOCK) {
            sleep(1);
            continue;
        } else if (num_read <= 0) {
            break;
        }
        //write(tcp_conn_socket, &incoming, num_read);
        incoming[num_read-1] = '\0';
        int pwm_left, pwm_right;
        sscanf(incoming, "%d,%d", &pwm_left, &pwm_right);

        printf("incoming: %s, pwm_left=%d, pwm_right=%d\n", incoming, pwm_left, pwm_right);
        fflush(stdout);
        if (pwm_left < 0) {
            pwm_left = 0;
        } else if (pwm_left > 50) {
            pwm_left = 50;
        }
        if (pwm_right < 0) {
            pwm_right = 0;
        } else if (pwm_right > 50) {
            pwm_right = 50;
        }
        info.duty = b16divi(uitoub16((uint8_t)pwm_left) - 1, 100);
        ioctl(fd_pwm1, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&info));
        sleep(1);
    }
    close(tcp_conn_socket);
    close(tcp_socket);

    // ioctl(fd_pwm0, PWMIOC_STOP, 0);
    ioctl(fd_pwm1, PWMIOC_STOP, 0);
    puts("Stopping PWM pulse train");
    // close(fd_pwm0);
    close(fd_pwm1);
    fflush(stdout);

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
    return 0;
}
