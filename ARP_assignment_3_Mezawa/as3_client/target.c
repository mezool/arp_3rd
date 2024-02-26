#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include "include/constant.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int sock;
struct sockaddr_in addr;
char send_buf[BUF_SIZE], recv_buf[BUF_SIZE];
int send_size, recv_size;

// variables represent the size of window
int window_value1 = 0;
int window_value2 = 0;

int send_data[MAX_DATA];

int transfer(int);

int wait_for_data(int sockfd, int timeout_seconds)
{
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;

    int result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

    if (result == -1)
    {
        perror("select");
        return -1;
    }
    else if (result == 0)
    {
        //printf("Timeout occurred. No data received within %d seconds.\n", timeout_seconds);
        return 0; // タイムアウト発生
    }

    return 1; // データが受信された
}

int main(int argc, char *argv[])
//Declarations about Variables and Pipes
{
    int send_pipe_read;
    int send_pipe;
    int receive_pipe;
    int receive_pipe_write;

    int writer_num = -1;
    // variables represent the size of window
    //Get descriptors from command line arguments
    if (argc == 3)
    {
        sscanf(argv[1], "%i", &writer_num);
        sscanf(argv[2], "%d %d %d %d", &send_pipe_read, &send_pipe, &receive_pipe, &receive_pipe_write);
    }
    else
    {
        printf("wrong number of arguments\n");
        return -1;
    }

    int in_progress = 0;
    //Close unnecessary pipes
    close(send_pipe_read);
    close(receive_pipe_write);

    printf("Writer number %i \n", writer_num);
    printf("Sending on pipe %i \n", send_pipe);
    printf("Receiving on pipe %i \n", receive_pipe);

    srandom(time(NULL) + writer_num * SEED_MULTIPLIER);

    //WD
    //Publish your pid
    pid_t watchdog_pid;
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[writer_num], "w");
    if (pid_fp == NULL)
    {
        perror("fopen");
        return -1;
    }
    fprintf(pid_fp, "%d", my_pid);
    fclose(pid_fp);

    //printf("Published pid %d \n", my_pid);
    // Read watchdog pid
    FILE *watchdog_fp = NULL;
    struct stat sbuf;

    /* call stat, fill stat buffer, validate success */
    if (stat(PID_FILE_PW, &sbuf) == -1)
    {
        perror("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0)
    {
        if (stat(PID_FILE_PW, &sbuf) == -1)
        {
            perror("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    //printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    clock_t start_time, end_time;
    double elapsed_time;
    start_time = clock();

    int sock;
    struct sockaddr_in addr;

    /* ソケットを作成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("socket error\n");
        return -1;
    }

    /* 構造体を全て0にセット */
    memset(&addr, 0, sizeof(struct sockaddr_in));

    /* サーバーのIPアドレスとポートの情報を設定 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    /* サーバーに接続要求送信 */
    printf("Start connect...\n");
    if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        printf("connect error\n");
        close(sock);
        return -1;
    }
    printf("Finish connect!\n");

    sprintf(send_buf, "TI");
    send_size = send(sock, send_buf, strlen(send_buf) + 1, 0);
    if (send_size == -1)
    {
        printf("send error\n");
    }
    sleep(1);
    while (window_value2 == 0)
    {
        printf("Inside the loop, window_value2: %d\n", window_value2);

        // wait for data with a timeout
        if (wait_for_data(sock, TIMEOUT_SECONDS) == 0)
        {
            //printf("No data received within the specified timeout. Moving on...\n");
            break; // タイムアウトしたらループを抜ける
        }

        // receive the msg from server
        recv_size = recv(sock, recv_buf, BUF_SIZE, 0);
        printf("Received : %s\n", recv_buf);
        fflush(stdout);

        if (recv_size == -1)
        {
            printf("recv error\n");
        }
        else if (recv_size == 0)
        {
            // if 0, connection is closed
            printf("connection ended\n");
        }
        else if (recv_buf[0] == 'E')
        { // show the echo msg
            printf("Received Echo: %s\n", recv_buf);
        }
        else
        {
            sscanf(recv_buf, "%d , %d", &window_value1, &window_value2);
            printf("Received initial value: %d, %d\n", window_value1, window_value2);
        }
        sleep(1);
    }
    // send the obstacle position decided at random
    for (int i = 0; i < 5; i++)
    {
        send_data[2 * i] = random() % window_value1;
    }
    for (int i = 0; i < 5; i++)
    {
        send_data[2 * i + 1] = random() % window_value2;
    }

    // generate the position
    sprintf(send_buf, "T[10]%d,%d|%d,%d|%d,%d|%d,%d|%d,%d|",
            send_data[0], send_data[1], send_data[2],
            send_data[3], send_data[4], send_data[5],
            send_data[6], send_data[7], send_data[8],
            send_data[9]);
    /* send string */
    send_size = send(sock, send_buf, strlen(send_buf) + 1, 0);
    printf("sent target position\n");
    if (send_size == -1)
    {
        printf("send error\n");
        return -1;
    }

    while (1)
    {
        transfer(sock);
        sleep(1);
        if (kill(watchdog_pid, SIGUSR1) < 0)
        {
            perror("kill");
        }
        //printf("target sent signal to watchdog\n"); // added to debug
    }
    close(sock);
    return 0;
}

int transfer(int sock)
{
    sprintf(send_buf, "T");
    /* send string */
    send_size = send(sock, send_buf, strlen(send_buf) + 1, 0);
    if (send_size == -1)
    {
        printf("send error\n");
        return -1;
    }
    // wait for data with a timeout
    if (wait_for_data(sock, TIMEOUT_SECONDS) == 0)
    {
        printf("No data for target received within the specified timeout. Moving on...\n");
        return -1; // タイムアウトしたら終了
    }

    // receive the msg from server
    recv_size = recv(sock, recv_buf, BUF_SIZE, 0);
    if (recv_size == -1)
    {
        printf("recv error\n");
        return -1;
    }
    if (recv_size == 0)
    {
        // if 0, connection is closed
        printf("connection ended\n");
        exit(0);
    }

    // // send the echo msg to client
    // sprintf(send_buf, "%s", recv_buf);
    // send_size = send(sock, send_buf, strlen(send_buf) + 1, 0);
    // if (send_size == -1) {
    //     printf("send error\n");
    // }

    /* if the msg is 'STOP', this is stopped */
    if (strcmp(recv_buf, "STOP") == 0)
    {
        printf("Finish connection\n");
        exit(0);
    }
    else if (strcmp(recv_buf, "GE") == 0)
    {
        // send the obstacle position decided at random
        printf("Received GE");
        for (int i = 0; i < 5; i++)
        {
            send_data[2 * i] = random() % window_value1;
        }
        for (int i = 0; i < 5; i++)
        {
            send_data[2 * i + 1] = random() % window_value2;
        }

        // generate the position
        sprintf(send_buf, "T[10]%d,%d|%d,%d|%d,%d|%d,%d|%d,%d|",
                send_data[0], send_data[1], send_data[2],
                send_data[3], send_data[4], send_data[5],
                send_data[6], send_data[7], send_data[8],
                send_data[9]);
        /* send string */
        send_size = send(sock, send_buf, strlen(send_buf) + 1, 0);
        if (send_size == -1)
        {
            printf("send error\n");
            return -1;
        }
    }
    else
    {
        // show the echo msg
        printf("Received Echo: %s\n", recv_buf);
    }
    return 0;
}
