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
    pid_t child_drone;
    pid_t child_keyboard;
    pid_t child_interface;
    pid_t child_server;
    pid_t child_watchdog;

    //Pipes for inter-process communication
    int drone_server[2];
    int server_drone[2];
    int keyboard_server[2];
    int server_keyboard[2];
    int interface_server[2];
    int server_interface[2];

// make pipes
    pipe(drone_server);
    pipe(server_drone);
    pipe(keyboard_server);
    pipe(server_keyboard);
    pipe(interface_server);
    pipe(server_interface);

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

    char drone_args[80];
    char keyboard_args[80];
    char interface_args[80];
// get pipe discriptors
    sprintf(drone_args, "%d %d %d %d", drone_server[0], drone_server[1], server_drone[0], server_drone[1]);
    sprintf(keyboard_args, "%d %d %d %d", keyboard_server[0], keyboard_server[1], server_keyboard[0], server_keyboard[1]);
    sprintf(interface_args, "%d %d %d %d", interface_server[0], interface_server[1], server_interface[0], server_interface[1]);
//Use fork to create child processes, each executing a different program.
//execvp is used to execute each child process.
    child_server = fork(); //-1 error　0 fork as child　+ fork and return to parent
    if (child_server < 0) {
        perror("Fork");
        return -1;
    }

    if (child_server == 0) {
        char * arg_list[] = { "./serverp","0", logfile_name, drone_args, interface_args, NULL };
        execvp("./serverp", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

//drone
    child_drone = fork();
    if (child_drone < 0) {
        perror("Fork");
        return -1;
    }

    if (child_drone == 0) {
        char * arg_list[] = {"./dronep", "1", drone_args, NULL };
        execvp("./dronep", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

    // create keyboard manager
    child_keyboard = fork();
    if (child_keyboard < 0) {
        perror("Fork");
        return -1;
    }

if (child_keyboard == 0) {
        char * arg_list[] = { "konsole", "-e", "./keyboard", NULL };
        execvp("konsole", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

// create interface manager
    child_interface = fork();
    if (child_interface < 0) {
        perror("Fork");
        return -1;
    }

    if (child_interface == 0) {
        char * arg_list[] = { "konsole", "-e", "./interface", "2", interface_args, NULL };
        execvp("konsole", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

    sleep(3);

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
