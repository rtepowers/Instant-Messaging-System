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


---
COMMANDS:
	/msg <username> <message to send>
		This sends a message to the user specified.

	/users
		This displays a list of connected users.

	/poke <username>
		This 'pokes' the user specified.

	/time
	/time <username>
		These commands display the time for a specific user. If no user is specified,
		the user calling the command will be substituted. 

	/joke
		Displays a random joke.

	/picture
		Displays a neat picture.

	/exit
	/close
	/quit
		These commands close the connection to the server and quits the program.