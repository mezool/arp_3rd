//from main.c from homework_5
#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "include/constant.h"

//Interprocess communication by creating several child processes, each executing a different program
//Execute each process
int main(int argc, char *argv[])
{
    // Pids for all children
    pid_t child_obstacle;
    pid_t child_target;
    pid_t child_watchdog;

    //Pipes for inter-process communication
    int obstacle_server[2];
    int server_obstacle[2];
    int target_server[2];
    int server_target[2];
// make pipes
    pipe(obstacle_server);
    pipe(server_obstacle);
    pipe(target_server);
    pipe(server_target);

    // Make a log file with the start time/date
    time_t now = time(NULL);
    struct tm *timenow;
    timenow = gmtime(&now);

    char logfile_name[80];

    //There should be a check that the log folder exists but I haven't done that
    sprintf(logfile_name, "./log/watchdog%i-%i-%i_%i:%i:%i.txt", timenow->tm_year + 1900, timenow->tm_mon +1, timenow->tm_mday, timenow->tm_hour +1, timenow->tm_min, timenow->tm_sec);

    fopen(PID_FILE_PW, "w");
    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    for(int i = 0; i < NUM_PROCESSES; i++)
    {
        fopen(fnames[i], "w");
    }

    int res;
    int num_children;

    char obstacle_args[80];
    char target_args[80];
// get pipe discriptors
    sprintf(obstacle_args, "%d %d %d %d", obstacle_server[0], obstacle_server[1], server_obstacle[0], server_obstacle[1]);
    sprintf(target_args, "%d %d %d %d", target_server[0], target_server[1], server_target[0], server_target[1]);
//Use fork to create child processes, each executing a different program.

//object
    child_obstacle = fork();
    if (child_obstacle < 0) {
        perror("Fork");
        return -1;
    }

    if (child_obstacle == 0) {
        char * arg_list[] = {"./obstacle", "0", obstacle_args, NULL };
        execvp("./obstacle", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

//target
    child_target = fork();
    if (child_target < 0) {
        perror("Fork");
        return -1;
    }

    if (child_target == 0) {
        char * arg_list[] = {"./target", "1", target_args, NULL };
        execvp("./target", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;


    // Create watchdog
    child_watchdog = fork();
    if (child_watchdog < 0) {
        perror("Fork");
        return -1;
    }

    if (child_watchdog == 0) {
        char * arg_list[] = {"./watchdog", NULL};
        execvp("./watchdog", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;
   
    
    //wait for all children to terminate
    for(int i = 0; i < num_children; i ++){
        wait(&res);
    }

    return 0;
}
