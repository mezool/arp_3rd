#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE 1024

#define MAX_MSG_LEN 1024
#define MAX_TARG_ARR_SIZE 20
#define MAX_OBST_ARR_SIZE 20

#define ID_T 'TI'
#define ID_O 'OI'
#define MSG_T 'T'
#define MSG_O 'O'
#define GAME_END 'GE'
#define STOP 'STOP'


#define ACK_CHAR 'K'
#define ASK_CHAR 'A'
#define ASKR_CHAR 'W' /* Ask for reading. Used in target.c */
#define REJ_CHAR 'R'
#define EXCEPTION_CHAR 'E'
#define INFO_CHAR 'I'

#define SHMOBJ_PATH "/shm_server"
#define SEM_PATH "/sem_server"

#define SEED_MULTIPLIER 120
#define MAX_NUMBER1 230
#define MAX_NUMBER2 60

#define OBJECT_NUMBER 10

#define MAX_DATA 22

#define NUM_PROCESSES 3 /* number of processes watchdog is tracking */
#define PROCESS_SLEEPS_US {100000, 250000, 100000} /* time each child process sleeps */
#define PROCESS_SIGNAL_INTERVAL 5 /* number of process cycles before signaling watchdog */
#define WATCHDOG_SLEEP_US 200000 /* watchdog sleep interval */
#define PROCESS_TIMEOUT_S 15 /* time in seconds of process inactivity before watchdog kills all processes */


#define PID_FILE_SP {"/tmp/pid_file0", "/tmp/pid_file1", "/tmp/pid_file2"} /* filenames for simple process pids */
#define PID_FILE_PW "/tmp/pid_filew" /* filename for watchdog process pid */
#define PROCESS_NAMES {"Process 0", "Process 1", "Process 2"} /* process names used for ncurses windows and in log file */


#endif // !CONSTANTS_H