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

double drone_cell[2];
int ob_cell[10];
int tar_cell[10];
double info_cell[5];
double F_x;
double F_y;
double F_extX;
double F_extY;
double score;

int main(int argc, char *argv[])
{
    int send_pipe_read;
    int send_pipe;
    int receive_pipe;
    int receive_pipe_write;

    int chch;

    char ask_char[MAX_MSG_LEN];
    ask_char[0] = ASK_CHAR;
    char read_str[MAX_MSG_LEN];
    char ack_char = ACK_CHAR;
    char info_char[MAX_MSG_LEN];
    info_char[0] = INFO_CHAR;
    int writer_num = -1;

    int in_progress1 = 0;
    int in_progress2 = 0;
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

    //ncurses
    initscr();                             //initialize ncurses
    start_color();                         // activate the usage of colors
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // red text and black background
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    noecho();    //Do not display when a key is entered
    curs_set(0); //Do not display the cursor
    timeout(0);  // Unblocked in 0 milisecond

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

    while (1)
    {
        // send ask
        if (in_progress1 == 0)
        {
            //printf("interface want to read\n");
            write(send_pipe, ask_char, strlen(ask_char) + 1);
            in_progress1 = 1;
        }
        if (in_progress1 == 2)
        {
            //printf("interface want to read\n");
            write(send_pipe, info_char, strlen(info_char) + 1);
            in_progress1 = 3;
        }
        read(receive_pipe, read_str, MAX_MSG_LEN);
        if (read_str[0] == REJ_CHAR)
        {
            printf("Rejected \n");
        }
        else if (read_str[0] == 'P')
        {
            sscanf(read_str, "P %lf, %lf, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
                   &drone_cell[0], &drone_cell[1], &tar_cell[0], &tar_cell[1], &tar_cell[2], &tar_cell[3], &tar_cell[4], &tar_cell[5],
                   &tar_cell[6], &tar_cell[7], &tar_cell[8], &tar_cell[9], &ob_cell[0], &ob_cell[1], &ob_cell[2], &ob_cell[3],
                   &ob_cell[4], &ob_cell[5], &ob_cell[6], &ob_cell[7], &ob_cell[8], &ob_cell[9]);
            in_progress1 = 2;
        }
        else if (read_str[0] == 'D')
        {
            sscanf(read_str, "D %lf, %lf, %lf, %lf, %lf",
                   &info_cell[0], &info_cell[1], &info_cell[2], &info_cell[3], &info_cell[4]);
            in_progress1 = 0;
        }

        F_x = info_cell[0];
        F_y = info_cell[1];
        F_extX = info_cell[2];
        F_extY = info_cell[3];
        score = info_cell[4];

        // ncurses
        erase();          // clear the window
        int ch = getch(); //wait for key input
        if (ch == 'q')
            break;                               // terminate if "q" is input
        attron(COLOR_PAIR(1));                   // activate the pair of colors
        mvprintw(drone_cell[1], drone_cell[0], "+"); // show drone
        attroff(COLOR_PAIR(1));                  // void the pair
        attron(COLOR_PAIR(2));                   // activate the pair of colors
        for (int i = 0; i < 5; i++)
        {
            mvprintw(ob_cell[2 * i + 1], ob_cell[2 * i], "o"); // show object
        }
        attroff(COLOR_PAIR(2)); // void the pair
        attron(COLOR_PAIR(3));  // activate the pair of colors
        for (int i = 0; i < 5; i++)
        {
            mvprintw(tar_cell[2 * i + 1], tar_cell[2 * i], "%d", i+1); // show object
        }
        mvprintw(MAX_NUMBER2 - 2, 2, "Fx = %f , Fy = %f , x = %f , y = %f, F_extX = %f, F_extY = %f, score = %f", F_x, F_y, drone_cell[0], drone_cell[1], F_extX, F_extY, score);
        WINDOW *win = newwin(MAX_NUMBER2 - 3, MAX_NUMBER1, 0, 0); // make main window
        box(win, 0, 0);                                           // drow the lines
        wrefresh(win);
        WINDOW *iwin = newwin(3, MAX_NUMBER1, MAX_NUMBER2 - 3, 0); // make inspection window
        box(iwin, 0, 0);                                           // drow the lines
        wrefresh(iwin);
        refresh(); // renew the screen
        //printf("x_i is: %f, y_i is: %f", x_i, y_i);

        sleep(1);
        if (kill(watchdog_pid, SIGUSR1) < 0)
        {
            perror("kill");
        }
        //printf("Object sent signal to watchdog\n"); // added to debug
    }

    return 0;
}