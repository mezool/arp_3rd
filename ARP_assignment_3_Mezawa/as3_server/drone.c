//drone
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "include/constant.h"
#include <termios.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>

//variables
int F_x = 0;
int F_y = 0;

void send_periodic_data(int send_pipe, int F_x, int F_y) {
    char send_str[MAX_MSG_LEN];
    // int send_intx = F_x % MAX_NUMBER;
    // int send_inty = F_y % MAX_NUMBER;
    sprintf(send_str, "%i, %i", F_x, F_y);
    //printf("Acknowledged, drone sent %i, %i \n", F_x, F_y);
    write(send_pipe, send_str, strlen(send_str) + 1);
}

int main(int argc, char *argv[]) 
//Declarations about Variables and Pipes
{
    int send_pipe_read;
    int send_pipe;
    int receive_pipe;
    int receive_pipe_write;
    int process_num;
    process_num = 0;

    char ask_char[MAX_MSG_LEN]; 
    ask_char[0] = ASK_CHAR;
    char read_str[MAX_MSG_LEN];
    char ack_char = ACK_CHAR;
    char send_str[MAX_MSG_LEN];
    int writer_num = -1;

    int fifo_fd;
    char *fifo_path = "/tmp/myfifotest";  // pass of FIFO

    // open the FIFO to read
    fifo_fd = open(fifo_path, O_RDONLY);

    if (fifo_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    ssize_t read_bytes;
//Get descriptors from command line arguments
    if(argc == 3){
        sscanf(argv[1],"%i", &writer_num);
        sscanf(argv[2], "%d %d %d %d", &send_pipe_read, &send_pipe, &receive_pipe, &receive_pipe_write);
    } else {
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

    int cells[2] = {0,0};

    srandom(process_num * SEED_MULTIPLIER);

    //WD
    //Publish your pid
    pid_t watchdog_pid;
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    if (pid_fp == NULL) {
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
    if (stat (PID_FILE_PW, &sbuf) == -1) {
        perror ("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0) {
        if (stat (PID_FILE_PW, &sbuf) == -1) {
            perror ("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    //printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    // int timer_counter = 0;
    int timer_threshold = 15;  

    while (1) 
    {   
        // send msg if any key aren't typed in 1 second
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fifo_fd, &read_fds);

        int ready = select(fifo_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready == -1) {
            perror("select");
            break;
        } else if (ready == 0) {
            //printf("Timeout: Do something... %d, %d\n",F_x, F_y);
            // timeout process
            if(!in_progress){
            //printf("Drone sent write request\n");
            write(send_pipe, ask_char, strlen(ask_char) + 1);
            in_progress = 1;
        }
        else //wait for reply
        {
            read(receive_pipe, read_str, MAX_MSG_LEN);
            // send force
            if(read_str[0] == ACK_CHAR)
            {
               send_periodic_data(send_pipe, F_x, F_y);
            }
            // show rejected if this code can't send the msg
            else if (read_str[0] == REJ_CHAR)
            {
                printf("Rejected \n");
            }
            in_progress = 0;
        }
            // send_periodic_data(send_pipe, F_x, F_y);
        } else {
        // read from FIFO
        read_bytes = read(fifo_fd, buffer, sizeof(buffer));

        if (read_bytes > 0) {
            write(STDOUT_FILENO, buffer, read_bytes);
            write(STDOUT_FILENO, "\n", 1); 
        } else if (read_bytes == 0) {
            // finish the loop when FIFO is closed
            break;
        } else {
            perror("read");
            break;
        }

// calculate forces
        if (strcmp(buffer, "up_left") == 0){
            F_x = F_x - 1;
            F_y = F_y - 1;
        } else if (strcmp(buffer, "up") == 0 ){
            F_y = F_y - 1;
        } else if (strcmp(buffer, "up_right") == 0 ) {
            F_x = F_x + 1;
            F_y = F_y - 1;
        } else if (strcmp(buffer, "left") == 0 ) {
            F_x = F_x - 1;
        } else if (strcmp(buffer, "right") == 0 ) {
            F_x = F_x + 1;
        } else if (strcmp(buffer, "down_left") == 0 ) {
            F_x = F_x - 1;
            F_y = F_y + 1;
        } else if (strcmp(buffer, "down") == 0 ) {
            F_y = F_y + 1;
        } else if (strcmp(buffer, "down_right") == 0 ) {
            F_x = F_x + 1;
            F_y = F_y + 1;
        } else if (strcmp(buffer, "stop") == 0 ) {
            F_x = 0;
            F_y = 0;
        } else if (strcmp(buffer, "quit") == 0 ) {
            F_x = 50000;
            F_y = 50000;
        } 
        //printf("F_x is: %d, F_y is: %d\n", F_x, F_y); // added to debug
        }
        if(kill(watchdog_pid, SIGUSR1) < 0)
            {
                perror("kill");
            }
        //printf("Sent signal to watchdog\n"); // added to debug
        usleep(50000);
        // printf("%d", timer_counter);
    } 
    // close FIFO
    close(fifo_fd);
    return 0; 
} 
