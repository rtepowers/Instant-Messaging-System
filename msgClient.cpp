// AUTHOR: Ray Powers
// DATE: March 3, 2013
// FILE: msgClient.cpp

// Standard Library
#include<iostream>
#include<sstream>
#include<string>
#include<cstring>

// Network Function
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>

// User Interface
#include<curses.h>

using namespace std;

// GLOBALS
const int INPUT_LINES = 2;
const char ENTER_SYM = '\n';
const char BACKSPACE_SYM = '\b';
const int DELETE_SYM = 127;
WINDOW *INPUT_SCREEN;
WINDOW *MSG_SCREEN;
fd_set hostfd;
struct timeval tv;
const short MSG_COLOR_BLACK = 0;
const short MSG_COLOR_RED = 1;
const short MSG_COLOR_GREEN = 2;
const short MSG_COLOR_YELLOW = 3;
const short MSG_COLOR_BLUE = 4;
const short MSG_COLOR_MAGENTA = 5;
const short MSG_COLOR_CYAN = 6;
const short MSG_COLOR_WHITE = 7;


// Data Structures
struct sockAddr {
  unsigned short sa_family;   // Address family (AF_INET)
  char sa_data[14];           // Protocol-specific addressinfo
};

struct in_address {
  unsigned long s_addr;       // Internet Address (32bits)
};

struct sockAddress_in {
  unsigned short sin_family;  // Address family (AF_INET)
  unsigned short sin_port;    // Port (16bits)
  struct in_addr sin_addr;    // Internet address structure
  char sin_zero[8];           // Not Used.
};

// Function Prototypes
void clearInputScreen();
// Function resets InputScreen to default state.
// pre: InputScreen should exist.
// post: none

bool getUserInput(string& inputStr);
// Function tests whether the user has typed a message.
// pre: InputScreen should exist.
// post: none

void displayMsg(string& msg);
// Function displays message
// pre: ChatScreen should exist.
// post: none

void prepareWindows();
// Function prepares user interface
// pre: none
// post: none

int openSocket (string hostName, unsigned short serverPort);
// Function sets up a working socket to use in sending data.
// pre: none
// post: none

bool SendMessage(int HostSock, string msg);
// Function sends message to Host socket.
// pre: HostSock should exist.
// post: none

string GetMessage(int HostSock, int messageLength);
// Function retrieves message from Host socket.
// pre: HostSock should exist.
// post: none

bool SendInteger(int HostSock, int hostInt);
// Function sends a network long variable over the network.
// pre: HostSock must exist
// post: none

long GetInteger(int HostSocks);
// Function listens to socket for a network Long variable.
// pre: HostSock must exist.
// post: none

int main (int argNum, char* argValues[]) {

  // Locals
  string inputStr;
  string hostname;
  unsigned short serverPort;
  int hostSock;
  int numberOfSocks;

  // Clear FD_Set and set timeout.
  FD_ZERO(&hostfd);
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  // Need to grab Command-line arguments and convert them to useful types
  // Initialize arguments with proper variables.
  if (argNum < 3 || argNum > 3){
    // Incorrect number of arguments
    cerr << "Incorrect number of arguments. Please try again." << endl;
    return -1;
  }

  // Need to store arguments
  hostname = argValues[1];
  serverPort = atoi(argValues[2]);

  // Begin User Interface
  prepareWindows();

  // Establish a socket Connection
  hostSock = openSocket(hostname, serverPort);
  FD_SET(hostSock, &hostfd);
  numberOfSocks = hostSock + 1;
  
  
  if (hostSock > 0 ) {
    // Enter a loop to begin
    while (true) {

      // If the user finished typing a message, get it and process it.
      if (getUserInput(inputStr)) {

	// If it's a command, handle it.
	if (inputStr == "/quit" || inputStr == "/close" || inputStr == "/exit") {
	  SendInteger(hostSock, inputStr.length()+1);
	  SendMessage(hostSock, inputStr);
	  break;
	}
          
	// Display the message in the chat window.
	string tmp = "You said: ";
	tmp.append(inputStr);
	displayMsg(tmp);
	//inputStr.clear();

	// Send to Server
	if (!SendInteger(hostSock, inputStr.length()+1)) {
	  cerr << "Unable to send Int. " << endl;
	  break;
	}

	if (!SendMessage(hostSock, inputStr)) {
	  cerr << "Unable to send Message. " << endl;
	  break;
	}
	inputStr.clear();
          
	// Make sure to reset for the next user input.
	clearInputScreen();
      }

      // Check for Messages
      int pollSock = select(numberOfSocks, &hostfd, NULL, NULL, &tv);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      FD_SET(hostSock, &hostfd);
      if (pollSock != 0 && pollSock != -1) {
	string tmp;
	long msgLength = GetInteger(hostSock);
	if (msgLength <= 0) {
	  cerr << "Couldn't get integer from Host." << endl;
	  break;
	}
	string HostMsg = GetMessage(hostSock, msgLength);
	if (HostMsg == "") {
	  cerr << "Couldn't get message from Host." << endl;
	  break;
	}
	tmp = "";
	tmp.append(HostMsg);
	displayMsg(tmp);
	tmp.clear();
	wrefresh(INPUT_SCREEN);
      }
    }
  }
  // Clean up and Close things down.
  delwin(INPUT_SCREEN);
  delwin(MSG_SCREEN);
  endwin();
  close(hostSock);

  return 0;
}


bool SendMessage(int HostSock, string msg) {

  // Local Variables
  int msgLength = msg.length()+1;
  char msgBuff[msgLength];
  strcpy(msgBuff, msg.c_str());
  msgBuff[msgLength-1] = '\0';

  // Since they now know how many bytes to receive, we'll send the message
  int msgSent = send(HostSock, msgBuff, msgLength, 0);
  if (msgSent != msgLength){
    // Failed to send
    cerr << "Unable to send data. Closing clientSocket: " << HostSock << "." << endl;
    return false;
  }

  return true;
}

string GetMessage(int HostSock, int messageLength) {

  // Retrieve msg
  int bytesLeft = messageLength;
  char buffer[messageLength];
  char* buffPTR = buffer;
  while (bytesLeft > 0){
    int bytesRecv = recv(HostSock, buffPTR, messageLength, 0);
    if (bytesRecv <= 0) {
      // Failed to Read for some reason.
      cerr << "Could not recv bytes. Closing clientSocket: " << HostSock << "." << endl;
      return "";
    }
    bytesLeft = bytesLeft - bytesRecv;
    buffPTR = buffPTR + bytesRecv;
  }

  return buffer;
}

long GetInteger(int HostSock) {

  // Retreive length of msg
  int bytesLeft = sizeof(long);
  long networkInt;
  char* bp = (char *) &networkInt;
  
  while (bytesLeft) {
    int bytesRecv = recv(HostSock, bp, bytesLeft, 0);
    if (bytesRecv <= 0){
      // Failed to receive bytes
      cerr << "Failed to receive bytes. Closing clientSocket: " << HostSock << "." << endl;
      return -1;
    }
    bytesLeft = bytesLeft - bytesRecv;
    bp = bp + bytesRecv;
  }
  return ntohl(networkInt);
}

bool SendInteger(int HostSock, int hostInt) {

  // Local Variables
  long networkInt = htonl(hostInt);

  // Send Integer (as a long)
  int didSend = send(HostSock, &networkInt, sizeof(long), 0);
  if (didSend != sizeof(long)){
    // Failed to Send
    cerr << "Unable to send data. Closing clientSocket: " << HostSock << "."  << endl;
    return false;
  }

  return true;
}

int openSocket (string hostName, unsigned short serverPort) {

  // Local variables.
  struct hostent* host;
  int status;
  int bytesRecv;

  // Create a socket and start server communications.
  int hostSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (hostSock <= 0) {
    // Socket was unsuccessful.
    cerr << "Socket was unable to be opened." << endl;
    return -1;
  }

  // Get host IP and Set proper fields
  host = gethostbyname(hostName.c_str());
  if (!host) {
    cerr << "Unable to resolve hostname's ip address. Exiting..." << endl;
    return -1;
  }
  char* tmpIP = inet_ntoa( *(struct in_addr *)host->h_addr_list[0]);
  unsigned long serverIP;
  status = inet_pton(AF_INET, tmpIP,(void*) &serverIP);
  if (status <= 0) return -1;

  struct sockAddress_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = serverIP ;
  serverAddress.sin_port = htons(serverPort);

  // Now that we have the proper information, we can open a connection.
  status = connect(hostSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if (status < 0) {
    cerr << "Error with the connection." << endl;
    return -1;
  }

  return hostSock;
}

void prepareWindows() {

  // Initialize Screen
  initscr();
  start_color();

  // Set timeout of input to 2 miliseconds.
  halfdelay(1);
  //nodelay(INPUT_SCREEN, true);

  // Create new MSG_SCREEN window and enable scrolling of text.
  noecho();
  cbreak();
  MSG_SCREEN = newwin(LINES - INPUT_LINES, COLS, 0, 0);
  scrollok(MSG_SCREEN, TRUE);
  wsetscrreg(MSG_SCREEN, 0, LINES - INPUT_LINES - 1);
  wrefresh(MSG_SCREEN);

  // SET Colors for window.
  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  wbkgd(MSG_SCREEN, COLOR_PAIR(1));

  // Create new INPUT_SCREEN window. No scrolling.
  INPUT_SCREEN = newwin(INPUT_LINES, COLS, LINES - INPUT_LINES, 0);
  
  // Prepare Input Screen.
  clearInputScreen();
  
}

void clearInputScreen() {

  // Clear screen and write a division line.
  werase(INPUT_SCREEN);
  mvwhline(INPUT_SCREEN, 0, 0, '-', COLS);

  // Move cursor to the start of the next line.
  wmove(INPUT_SCREEN, 1, 0);
  waddstr(INPUT_SCREEN, "Input:  ");

  wrefresh(INPUT_SCREEN);

}

bool getUserInput(string& inputStr) {

  // Locals
  bool success = false;
  int userText;

  // Get input from from the input screen. Timeout will occur if no characters are available.
  userText = wgetch(INPUT_SCREEN);

  // Did we return a proper value?
  if (userText == ERR)
    return false;

  // Can we display the text? Add to inputStr if yes.
  if (isprint(userText)) {
    inputStr += (char)userText;
    waddch(INPUT_SCREEN, userText);
  }
  // Pressing <enter> signals that the user has 'sent' something.
  else {
    // Allow for Backspacing.
    if (userText == BACKSPACE_SYM || userText == DELETE_SYM) {
      inputStr.replace(inputStr.length()-1, 1, "");
      wmove(INPUT_SCREEN, 1, 8 + inputStr.length());
      userText = 32;
      waddch(INPUT_SCREEN, userText);
      wmove(INPUT_SCREEN, 1, 8 + inputStr.length());

      // Show new screen.
      wrefresh(INPUT_SCREEN);
    }
    
    if (userText == ENTER_SYM) {
    // If inputStr isn't empty, it should be submitted.
      if (inputStr.size() > 0) {
	success = true;
      }
    }
  }
  return success;
}

void displayMsg(string &msg) {
  
  // Add Msg to screen.
  if (msg.c_str()[0] == '/') {
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    wbkgd(MSG_SCREEN, COLOR_PAIR(2));
    
  } else {
    // SET Colors for window.
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    wbkgd(MSG_SCREEN, COLOR_PAIR(1));
  }
  waddstr(MSG_SCREEN, msg.c_str());
  waddch(MSG_SCREEN, '\n');
  
  // Show new screen.
  wrefresh(MSG_SCREEN);

}
