// 参考:
// http://www.geekpage.jp/programming/linux-network/nonblocking.php
// http://www.kegel.com/dkftpbench/nonblocking.html

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <wiringPi.h>

#define PIN_FIN (1) // GPIO 18
#define PIN_RIN (4) // GPIO 23
#define PWM_CLOCK_DIVISOR (8)
#define PWM_RANGE (64)
#define RECV_BUF_SIZE (32)
#define RECV_TIMEOUT (30)

void signal_handler(int signum);

void stop_motor()
{
    pinMode(PIN_FIN, OUTPUT);
    pinMode(PIN_RIN, OUTPUT);
    digitalWrite(PIN_FIN, LOW);
    digitalWrite(PIN_RIN, LOW);
}

void init_motor()
{
    // using the wiringPi pin numbering scheme
    if(-1 == wiringPiSetup())
    {
        fprintf(stderr, "wiringPiSetup() failed\n");
        exit(1);
    }

    stop_motor();
}

void set_signal_handler(int signum)
{
    if(SIG_ERR == signal(signum, signal_handler))
    {
        fprintf(stderr, "set_signal_handler failed\n");
        exit(1);
    }
}

void signal_handler(int signum)
{
    fprintf(stderr, "signal_handler: got signal %d\n", signum);
    //set_signal_handler(signum);

    stop_motor();

    exit(1);
}

void run()
{
    int sock;
    struct sockaddr_in addr;

    char buf[RECV_BUF_SIZE];
    int flags;
    int n;
    int recv_timeout = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    if(-1 == (flags = fcntl(sock, F_GETFL, 0)))
    {
        flags = 0;
    }
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        n = recv(sock, buf, sizeof(buf), 0);
        if(n < 1)
        {
            if(EAGAIN == errno || EWOULDBLOCK == errno)
            {
                usleep(100000);
                if(++recv_timeout >= RECV_TIMEOUT)
                {
                    stop_motor();
                    recv_timeout = RECV_TIMEOUT;
                }                
            }
            else
            {
                printf("errno = %d\n", errno);
                break;
            }
        }
        else
        {
            int i;
            int pwm_duty = 0;
            for(i = 0; i < n; i++)
            {
                printf("%d\n", buf[i]);
                pwm_duty = buf[i];
            }

            recv_timeout = 0;

            if(pwm_duty == 0)
            {
                stop_motor();
            }
            else if(pwm_duty == 255)
            {
                // 0xff: keepalive
                // NOP
            }
            else
            {
                if(pwm_duty >= 40)
                {
                    pwm_duty = 40;
                }
    
                pinMode(PIN_FIN, PWM_OUTPUT);
                pwmSetMode(PWM_MODE_MS); // constant freq, variable mark-space time

                // pwm freq = 19200[kHz] / 8[clock] / 64[range] = 37.5[kHz]
                // (target freq of BD6211F = 20kHz-100kHz)
                pwmSetClock(PWM_CLOCK_DIVISOR);
                pwmSetRange(PWM_RANGE);

                pwmWrite(PIN_FIN, pwm_duty);
                digitalWrite(PIN_RIN, LOW);
            }
        }
    }

    close(sock);

    return;
}

int main()
{
    set_signal_handler(SIGINT);
    set_signal_handler(SIGTERM);

    init_motor();
    run();

    return 0;
}

