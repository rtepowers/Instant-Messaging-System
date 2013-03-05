all: imClient
imClient: msgClient.cpp
	g++ msgClient.cpp -o msgClient -lcurses
	g++ msgServer.cpp -o msgServer -lpthread

clean:
	rm -rf msgClient

