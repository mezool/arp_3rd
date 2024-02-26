all: masterp dronep keyboard watchdog serverp interface
	
masterp:
	gcc master_server.c -o build/masterp
dronep:
	gcc drone.c -o build/dronep
keyboard:
	gcc keyboard.c -o build/keyboard
watchdog:
	gcc WD_server.c -o build/watchdog
serverp:
	gcc server.c -o build/serverp -lm
interface:
	gcc interface.c -o build/interface -lncurses