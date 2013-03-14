all: imClient
imClient: msgClient.cpp
	g++ msgClient.cpp -o msgClient -lcurses -lpthread
	g++ msgServer.cpp -o msgServer -lpthread

clean:
	rm -rf msgClient

