/*
*		Instant-Messenger Application
*		Created by Ray Powers, Nichole Minas, Ken Nicks, Huynh Nguyen
*		© 2013
*/

---
ASSUMPTIONS:

	Max threads capped at 50

---
COMPILE:

	make
		OR
	g++ msgServer.cpp -o msgServer -lpthread
	g++ msgClient.cpp -o msgClient -lcurses -lpthread 

---
USAGE:

	Server:
		./msgServer [port #]
	Client:
		./msgClient [server address] [port #]

---
NOTES:
	Color from the Curses library will only appear on terminals that can support them.