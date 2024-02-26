readme_arp_3rd_assignment

Here are my second assignment.
It includes :
readme_arp_3rd_assignment.txt (this txt file)
as3_client(folder) which includes
constant.h
makefile
master_client.c
obstacle.c
target.c
WD_client.c

as3_server(folder) which includes
constant.h
makefile
master_server.c
drone.c
server.c
keyboard.c
interface.c
WD_server.c


arp_3_architecture.jpg

constant.h are in: include


Role of codes:
    The master_client.c, master_server.c forks and executes each file.

    The keyboard.c assigns the role of each key. It also prompts for key input using Konsole and sends the typed key information to the drone.c using FIFO.

    The drone.c calculates the input force by using the information about the typed key from the keyboard.c using FIFO. When this code does this, a signal is sent to the WD_server.c to confirm this code is alive. The drone.c sends the force information to the server.c by using a pipe. 
    
	The server.c calculates the drone's position by using the force information from the drone.c by using a pipe and from the target.c and object.c by using socket. This calculates the repulsive force and attractive force from the position information from information from the drone.c and target.c. The signal is sent to the WD_server.c to confirm this code is alive. When the drone gets to the target, this code moves the target out of the window. The score is calculated based on how long the drone takes to reach each target.

	The interface.c manages a blackboard that shows the drone, obstacles, and targets. Moreover, this also manages an inspection window that shows the drone's position, force, and score. This information is also written in a log file by server.c.

    The WD_server.c and WD_client.c confirm whether codes are alive. This terminates the systems if the watchdog doesn't receive the signal from each code longer than a certain time (The initial is set to 15 seconds).

    The obstacle.c sends the obstacles' positions to the server. The number of the obstacles is set as 5. When this code does this, a signal is sent to the WD_client.c to confirm this code is alive. The object.c sends the position information to the server.c by using a socket. The positions are changed every 10 seconds.

    The target.c sends the targets' initial positions and calculates the score. The signal is sent to the WD_client.c to confirm this code is alive. The target.c sends the position information to the server.c using a socket.


To run the server:
cd ARP_assignment_3_Mezawa
cd as3_server
make
cd build
./masterp

To run the client:
cd ARP_assignment_3_Mezawa
cd as3_client
make
cd build
./masterp

log files would be in:
as3_server/build/log

Role of keys:
Typing 'q', the system is finished.
Typing 'w', the force input to the upper left.
Typing 'e', the force input to the upper.
Typing 'r', the force input to the upper right.
Typing 's', the force input to the left.
Typing 'f', the force input to the right.
Typing 'x', the force input to the lower left.
Typing 'c', the force input to the lower.
Typing 'v', the force input to the lower right.
Typing 'd', the force input to the drone stops.

There are also repulsive forces and attractive forces so they make the drone's manipulation a bit difficult.

These files (including this readme.txt) are shared in the GitHub.
https://github.com/mezool/ARP_3rd

apology
In submitting this assignment, there are a few specifications that have not been completed or reached. They are listed below.
Both obstacle and target are designed with the assumption that there are five, and no other number is accepted.
The message from the server to the client does not work properly, and as a stopgap measure, the target sends a 'T' at each loop.

These will cause connectivity problems with other projects, and I hope to have them fixed by the time of testing.
