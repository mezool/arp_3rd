// server
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include "include/constant.h"
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <ncurses.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

int F_x = 0;
int F_y = 0;
double F_ox[5] = {0, 0, 0, 0, 0};
double F_oy[5] = {0, 0, 0, 0, 0};
double F_tx[5] = {0, 0, 0, 0, 0};
double F_ty[5] = {0, 0, 0, 0, 0};
int tar_pos[10];
int ob_pos[10];
int M = 1;
int K = 1;
double T = 0.05;
double dist;
double distx = 6;
double disty = 6;
int goal[6] = {0, 1, 1, 1, 1, 1};
int end = 0;
int quit = 0;
int timescore[5] = {30, 60, 90, 120, 150};
double score = 0;

struct client1_data
{
    int socket;
};

void *handle_target(void *arg);

int main(int argc, char *argv[])
{
    //placeholders for pipes to close
    int send_pipe_read[2];
    int receive_pipe_write[2];

    //pipes
    int send_pipe_writer_0;
    int receive_pipe_writer_0;
    int send_pipe_interface;
    int receive_pipe_interface;

    fd_set reading;
    char read_str[MAX_MSG_LEN];

    char ack_char[MAX_MSG_LEN];
    ack_char[0] = ACK_CHAR;

    char send_str[MAX_MSG_LEN];
    double send_data[5];
    double send_pos[MAX_DATA];

    int cell[2] = {0, 0};
    int tmp_cell[2] = {0, 0};
    int has_changed = 0;

    char format_string[MAX_MSG_LEN] = "%d";
    char output_string[MAX_MSG_LEN];

    char logfile_name[80];

    //placeholders for pipes to close
    int send_pipe_writer_0_fds[2];
    int receive_pipe_writer_0_fds[2];
    int send_pipe_interface_fds[2];
    int receive_pipe_interface_fds[2];

    // Pipe creation and error checking
    if (pipe(send_pipe_writer_0_fds) == -1 || pipe(receive_pipe_writer_0_fds) == -1 ||
        pipe(send_pipe_interface_fds) == -1 || pipe(receive_pipe_interface_fds) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Assignment of file descriptors
    send_pipe_writer_0 = send_pipe_writer_0_fds[1];
    receive_pipe_writer_0 = receive_pipe_writer_0_fds[0];
    send_pipe_interface = send_pipe_interface_fds[1];
    receive_pipe_interface = receive_pipe_interface_fds[0];

    //The program receives four command line arguments and sets the descriptors for each pipe.
    if (argc == 5)
    {
        sscanf(argv[3], "%d %d %d %d", &receive_pipe_writer_0, &receive_pipe_write[0], &send_pipe_read[0], &send_pipe_writer_0);
        sscanf(argv[4], "%d %d %d %d", &receive_pipe_interface, &receive_pipe_write[1], &send_pipe_read[1], &send_pipe_interface);
    }
    else
    {
        printf("wrong args %i \n", argc);
        sleep(15);
        return -1;
    }

    // Extract log file name
    if (argc == 5)
    {
        snprintf(logfile_name, 80, "%s", argv[2]);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }
    int process_num;
    if (argc == 5)
    {
        sscanf(argv[1], "%d", &process_num);
    }
    else
    {
        printf("wrong args\n");
        return -1;
    }
    //close innecessary pipes
    for (int i = 0; i < 2; i++)
    {
        close(send_pipe_read[i]);
        close(receive_pipe_write[i]);
    }

    //Compose fd from received information
    int fds[2];
    fds[0] = receive_pipe_writer_0;
    fds[1] = receive_pipe_interface;

    printf("Receiving on %d, %d \n", receive_pipe_writer_0, receive_pipe_interface);
    printf("Sending on %d, %d \n", send_pipe_writer_0, send_pipe_interface);

    int max_fd = -1;
    for (int i = 0; i < 2; i++)
    {
        if (fds[i] > max_fd)
        {
            max_fd = fds[i];
        }
    }
    //Identify maximum
    printf("Max fd %i\n", max_fd);

    pid_t watchdog_pid;
    // Publish your pid
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    if (pid_fp == NULL)
    {
        perror("fopen");
        return -1;
    }
    fprintf(pid_fp, "%d", my_pid);
    fclose(pid_fp);
    //printf("Published pid %d \n", my_pid);

    //WD
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
    printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    //Read how long to sleep process for
    // int sleep_durations[3] = PROCESS_SLEEPS_US;
    // int sleep_duration = sleep_durations[process_num];

    int shared_seg_size = (1 * sizeof(cell));
    // initializes/clears contents of logfile
    FILE *lf_fp = fopen(logfile_name, "w");
    if (pid_fp == NULL)
    {
        perror("fopen");
        return -1;
    }
    fprintf(lf_fp, "Fx, Fy, x, y, score \n");
    fclose(lf_fp);

    //int row, col;
    //getmaxyx(stdscr, row, col); // Get the number of rows and columns on the screen
    double x_i = MAX_NUMBER1 / 2;
    double x_i1 = MAX_NUMBER1 / 2;
    double x_i2 = MAX_NUMBER1 / 2;
    double y_i = MAX_NUMBER2 / 2;
    double y_i1 = MAX_NUMBER2 / 2;
    double y_i2 = MAX_NUMBER2 / 2;

    //socket
    int w_addr, c_sock1, c_sock2;
    struct sockaddr_in a_addr, c1_addr, c2_addr;
    socklen_t c1_len = sizeof(c1_addr);
    socklen_t c2_len = sizeof(c2_addr);

    /* make socket */
    w_addr = socket(AF_INET, SOCK_STREAM, 0);
    if (w_addr == -1)
    {
        printf("socket error\n");
        return -1;
    }

    /* set all the structure to 0 */
    memset(&a_addr, 0, sizeof(struct sockaddr_in));

    /* set the IP address and imformatin about the port */
    a_addr.sin_family = AF_INET;
    a_addr.sin_port = htons((unsigned short)SERVER_PORT);
    a_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    int reuse = 1;

    if (setsockopt(w_addr, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        printf("setsockopt error\n");
        close(w_addr);
        return -1;
    }

    if (bind(w_addr, (const struct sockaddr *)&a_addr, sizeof(a_addr)) == -1)
    {
        printf("bind error\n");
        close(w_addr);
        return -1;
    }

    /* listen */
    if (listen(w_addr, 2) == -1)
    {
        printf("listen error\n");
        close(w_addr);
        return -1;
    }

    /* accept and wait the connect of socket */
    printf("Waiting connect...\n");
    c_sock1 = accept(w_addr, NULL, NULL);
    if (c_sock1 == -1)
    {
        printf("accept error\n");
        close(w_addr);
        return -1;
    }
    printf("One of the clients is connected\n");

    c_sock2 = accept(w_addr, NULL, NULL);
    if (c_sock2 == -1)
    {
        printf("accept error\n");
        close(w_addr);
        return -1;
    }
    printf("Connected!!\n");

    clock_t start_time, end_time;
    double elapsed_time;
    start_time = clock();

    while (1)
    {
        //reset reading set
        // Select to see if there is readable data
        //select can monitor multiple file descriptors simultaneously
        FD_ZERO(&reading);
        FD_SET(receive_pipe_writer_0, &reading);
        FD_SET(receive_pipe_interface, &reading);
        int ret_val = select(max_fd + 1, &reading, NULL, NULL, NULL);

        /* communicate with connected socket */
        pthread_t thread_id1;
        if (pthread_create(&thread_id1, NULL, handle_target, (void *)&c_sock1) != 0)
        {
            printf("pthread_create error\n");
            close(c_sock1);
        }
        pthread_t thread_id2;
        if (pthread_create(&thread_id2, NULL, handle_target, (void *)&c_sock2) != 0)
        {
            printf("pthread_create error\n");
            close(c_sock2);
        }

        for (int i = 0; i < max_fd + 1; i++)
        {
            if (FD_ISSET(i, &reading))
            {
                //deal with data from drone
                if (i == receive_pipe_writer_0)
                {
                    read(i, read_str, MAX_MSG_LEN);
                    //printf("drone\n");
                    // deal with ASK_CHAR msg
                    if (read_str[0] == ASK_CHAR)
                    {
                        //printf("writer 0 wants to write\n");
                        // printf("Writer 0 can write\n");
                        write(send_pipe_writer_0, ack_char, strlen(ack_char) + 1);
                        // deal with the others msg
                    }
                    else
                    {
                        sscanf(read_str, "%i, %i", &tmp_cell[0], &tmp_cell[1]);
                        //printf("Writer 0 set cell 0 to %i, %i\n", tmp_cell[0],tmp_cell[1]);
                    }
                }
                if (i == receive_pipe_interface)
                {
                    read(i, read_str, MAX_MSG_LEN);
                    //printf("interface\n");
                    // deal with ASK_CHAR msg
                    if (read_str[0] == ASK_CHAR)
                    {
                        sprintf(send_str, "P %lf, %lf, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
                                send_pos[0], send_pos[1], tar_pos[0], tar_pos[1], tar_pos[2], tar_pos[3], tar_pos[4], tar_pos[5],
                                tar_pos[6], tar_pos[7], tar_pos[8], tar_pos[9], ob_pos[0], ob_pos[1], ob_pos[2], ob_pos[3],
                                ob_pos[4], ob_pos[5], ob_pos[6], ob_pos[7], ob_pos[8], ob_pos[9]);
                        //printf("positions are sent to interface\n");
                        write(send_pipe_interface, send_str, strlen(send_str) + 1);
                    }
                    else if (read_str[0] == INFO_CHAR)
                    {
                        sprintf(send_str, "D %f, %f, %f, %f, %f",
                                send_data[0], send_data[1], send_data[2], send_data[3], send_data[4]);
                        //printf("datas are sent to interface\n");
                        write(send_pipe_interface, send_str, strlen(send_str) + 1);
                    }
                }
            }
        }
        // renew the data of position
        for (int i = 0; i < 2; ++i)
        {
            cell[i] = tmp_cell[i];
        }

        // Reads data from shared memory and copies it to the array cells.
        F_x = cell[0];
        F_y = cell[1];
        double F_oX = 0;
        double F_oY = 0;
        double F_tX = 0;
        double F_tY = 0;
        if (fabs(F_x) > 10000 || fabs(F_y) > 10000)
        {
            quit = 1;
        }

        // calculate repulsive forces
        for (int i = 0; i < 5; i++)
        {
            distx = x_i - ob_pos[2 * i];
            disty = y_i - ob_pos[2 * i + 1];
            dist = distx * distx + disty * disty;
            if (dist <= 25)
            {
                if (distx < 0.5 && distx > 0)
                {
                    distx = 0.5;
                }
                if (distx > -0.5 && distx < 0)
                {
                    distx = -0.5;
                }
                if (distx == 0)
                {
                    distx = 100;
                }
                if (disty < 0.5 && disty > 0)
                {
                    disty = 0.5;
                }
                if (disty > -0.5 && disty < 0)
                {
                    disty = -0.5;
                }
                if (disty == 0)
                {
                    disty = 100;
                }
                F_ox[i] = 10 * (fabs(1 / distx) - (1 / 5)) * (1 / distx) * (1 / fabs(distx));
                F_oy[i] = 10 * (fabs(1 / disty) - (1 / 5)) * (1 / disty) * (1 / fabs(disty));
            }
            else
            {
                F_ox[i] = 0;
                F_oy[i] = 0;
            }
        }

        // calculate the atractive force
        for (int i = 0; i < 5; i++)
        {
            distx = x_i - tar_pos[2 * i];
            disty = y_i - tar_pos[2 * i + 1];
            dist = distx * distx + disty * disty;
            if (dist <= 25)
            {
                if (dist > 0.25)
                {
                    double force_magnitude = -5 * (fabs(1 / dist) - (1 / 5)) * (1 / dist);
                    // calculate the direction
                    double angle = atan2(disty, distx);
                    // calculate the magnitude of x and y
                    F_tx[i] = force_magnitude * cos(angle);
                    F_ty[i] = force_magnitude * sin(angle);
                }
            }
            else
            {
                F_tx[i] = 0;
                F_ty[i] = 0;
            }
        }

        // sum up the forces
        int lengthx = sizeof(F_ox) / sizeof(F_ox[0]);
        for (int i = 0; i < lengthx; i++)
        {
            F_oX += F_ox[i];
        }

        int lengthy = sizeof(F_oy) / sizeof(F_oy[0]);
        for (int i = 0; i < lengthy; i++)
        {
            F_oY += F_oy[i];
        }

        int lentarx = sizeof(F_tx) / sizeof(F_tx[0]);
        for (int i = 0; i < lentarx; i++)
        {
            F_tX += F_tx[i];
        }

        int lentary = sizeof(F_ty) / sizeof(F_ty[0]);
        for (int i = 0; i < lentary; i++)
        {
            F_tY += F_ty[i];
        }

        // calculate the drone's position
        x_i2 = x_i1;
        x_i1 = x_i;
        x_i = ((F_x + F_oX + F_tX) * T * T + M * (2 * x_i1 - x_i2) + K * T * x_i1) / (M + K * T);
        //x_i = ( F_x*T*T +M*(2*x_i1-x_i2)+K*T*x_i1) / (M+K*T);
        if (x_i > MAX_NUMBER1 - 1)
        {
            x_i = MAX_NUMBER1 - 1;
        }
        else if (x_i < 0)
        {
            x_i = 0;
        }
        y_i2 = y_i1;
        y_i1 = y_i;
        y_i = ((F_y + F_oY + F_tY) * T * T + M * (2 * y_i1 - y_i2) + K * T * y_i1) / (M + K * T);
        //y_i = ( F_y*T*T +M*(2*y_i1-y_i2)+K*T*y_i1) / (M+K*T);
        if (y_i > MAX_NUMBER2 - 4)
        {
            y_i = MAX_NUMBER2 - 4;
        }
        else if (y_i < 0)
        {
            y_i = 0;
        }

        double F_xall = F_x + F_oX + F_tX;
        double F_yall = F_y + F_oY + F_tY;
        send_pos[0] = x_i;
        send_pos[1] = y_i;
        send_data[0] = F_x;
        send_data[1] = F_y;
        send_data[2] = F_xall - F_x;
        send_data[3] = F_yall - F_y;
        send_data[4] = score;

        for (int i = 0; i < 5; i++)
        {
            if (fabs(x_i - tar_pos[2 * i]) < 1 && fabs(y_i - tar_pos[2 * i + 1]) < 1 && goal[i] == 0)
            {
                goal[i + 1] = 0;
                tar_pos[2 * i] = 500;
                tar_pos[2 * i + 1] = 500;
                end_time = clock();
                elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
                printf("Elapsed time: %f seconds\n", elapsed_time);
                score = score + 1000 * (timescore[i] - elapsed_time);
            }
        }
        //printf("end: %d\n", end);
        if (goal[5] == 0)
        {
            end = 1;
        }

        // send signal to watchdog every process_signal_interval
        // Send the SIGUSR1 signal to the watchdog process using the kill function.
        if (kill(watchdog_pid, SIGUSR1) < 0)
        {
            perror("kill");
        }
        //printf("Sent signal to watchdog\n");  //add to debug
        FILE *lf_fp = fopen(logfile_name, "a");
        fprintf(lf_fp, "%f , %f , %f , %f , %f \n", F_xall, F_yall, x_i, y_i, score);
        //printf("%d", goal[1]);
        fclose(lf_fp);
        usleep(10000);
    }

    /* close the socket */
    close(w_addr);
    close(c_sock1);
    close(c_sock2);
    return 0;
}

void *handle_target(void *arg)
{
    int c_sock = *(int *)arg;
    int recv_size, send_size;
    char recv_buf[BUF_SIZE], send_buf[BUF_SIZE];
    // receive msg from client1
    recv_size = recv(c_sock, recv_buf, BUF_SIZE, 0);
    if (recv_size == -1)
    {
        printf("recv error\n");
    }
    else if (recv_size == 0)
    {
        // if 0, connection is closed
        printf("connection ended\n");
    }
    else
    { // show the echo msg
        printf("Received: %s\n", recv_buf);

        // send the echo msg to client1
        sprintf(send_buf, "Echo %s", recv_buf);
        //send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
        if (send_size == -1)
        {
            printf("echo send error\n");
        }
    }

    // if game started or completed
    if (strcmp(recv_buf, "TI") == 0 || strcmp(recv_buf, "OI") == 0)
    {
        // send the window size
        sprintf(send_buf, "%d , %d", MAX_NUMBER1, MAX_NUMBER2 - 3);
        printf("send maximum %d, %d \n", MAX_NUMBER1, MAX_NUMBER2 - 3);
        send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
        if (send_size == -1)
        {
            printf("size send error\n");
        }
    }
    else if (recv_buf[0] == 'T')
    {
        sscanf(recv_buf,
               "T[10]%d,%d|%d,%d|%d,%d|%d,%d|%d,%d|",
               &tar_pos[0], &tar_pos[1], &tar_pos[2],
               &tar_pos[3], &tar_pos[4], &tar_pos[5],
               &tar_pos[6], &tar_pos[7], &tar_pos[8],
               &tar_pos[9]);
    }
    else if (recv_buf[0] == 'O')
    {
        sscanf(recv_buf,
               "O[10]%d,%d|%d,%d|%d,%d|%d,%d|%d,%d",
               &ob_pos[0], &ob_pos[1], &ob_pos[2],
               &ob_pos[3], &ob_pos[4], &ob_pos[5],
               &ob_pos[6], &ob_pos[7], &ob_pos[8],
               &ob_pos[9]);
    }

    if (end == 1 && recv_buf[0] == 'T')
    {
        // send the window size
        sprintf(send_buf, "GE");
        printf("sent GE");
        send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
        if (send_size == -1)
        {
            printf("GE send error\n");
        }
        send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
        if (send_size == -1)
        {
            printf("GE send error\n");
        }
        end = 0;
        for (int i = 1; i < 6; i++)
        {
            goal[i] = 1;
        }
    }
    if (quit == 1)
    {
        sprintf(send_buf, "STOP");
        if (recv_buf[0] == 'T')
        {
            send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
            if (send_size == -1)
            {
                printf("STOP send error\n");
            }
        }
        else
        {
            send_size = send(c_sock, send_buf, strlen(send_buf) + 1, 0);
            if (send_size == -1)
            {
                printf("STOP send error\n");
            }
        }
    }
    pthread_exit(NULL);
}