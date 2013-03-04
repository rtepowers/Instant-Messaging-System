all: imClient
imClient: msgClient.cpp
	g++ msgClient.cpp -o msgClient -lcurses

clean:
	rm -rf msgClient

