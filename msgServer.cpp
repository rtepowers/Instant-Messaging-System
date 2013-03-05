// AUTHOR: Raymond Powers
// DATE: November 26th, 2012
// PLATFORM: C++ 

// DESCRIPTION: This program serves as the server portion of the networked number game.

// Standard Library
#include<iostream>
#include<sstream>
#include<string>
#include<ctime>
#include<cstdlib>
#include<tr1/unordered_map>
#include<deque>

// Network Functions
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

// Multithreading
#include<pthread.h>

using namespace std;

// DATA TYPES
struct threadArgs {
  int clientSock;
};

struct User {
  string username;
  string password;
  time_t timeConnected;
  bool isConnected;
};

struct Msg {
  string to;
  string from;
  string msg;
  string cmd;
};


// GLOBALS
const int MAXPENDING = 20;
tr1::unordered_map<string, User> UsersList;
deque<Msg> MsgQueue;
pthread_mutex_t MsgQueueLock;
pthread_mutex_t UserListLock;
int MsgStatus = pthread_mutex_init(&MsgQueueLock, NULL);
int UserStatus = pthread_mutex_init(&UserListLock, NULL);

// Function Prototypes
void* clientThread(void* args_p);
// Function serves as the entry point to a new thread.
// pre: none
// post: none

void InstantMessage(int clientSock);
// Function implements logic for an instant messaging client.
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

void addToMsgQueue(Msg newMsg);
// Function Handles adding messages to the MsgQueue.
// pre: none
// post: none

void processMsg(string &msg, string &cmdName, string &userTo);
// Function strips a msg value for data relating to commands and to users.
// pre: cmdName and userTo should be "" by default.
// post: msg will be reduced in size.

void SaveMsg(string msg, string userFrom);
// Function takes data from Thread and processes the message and then finally adds to a queue.
// pre: none
// post: none

string GetMsgs(string username);
// Function looks through the MsgQueue and compiles a list of messages to send.
// pre: none
// post: MsgQueue will have items removed.

void setUserDisconnected (User newUser);
// Function changes user status to disconnected.
// pre: none
// post: none

void setUserConnected (User newUser);
// Function changes user status to connected.
// pre: none
// post: user's timeConnected value is updated.

bool isUserConnected (User newUser);
// Function returns a user's connected status.
// pre: none
// post: none

bool doesUserExist (string username);
// Function tests the existence of a user in the UserList.
// pre: none
// post: none

void addToUsersList (User newUser);
// Function adds a user to the UserList.
// pre: none
// post: none

bool loginUser (string username, string password);
// Function checks login credentials against the UserList.
// pre: none
// post: none

int main(int argc, char* argv[]){

  // Local Vars

  // Process Arguments
  unsigned short serverPort; 
  if (argc != 2){
    // Incorrect number of arguments
    cerr << "Incorrect number of arguments. Please try again." << endl;
    return -1;
  }
  serverPort = atoi(argv[1]);

  // Create socket connection
  int conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (conn_socket < 0){
    cerr << "Error with socket." << endl;
    exit(-1);
  }

  // Set the socket Fields
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;    // Always AF_INET
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(serverPort);
  
  // Assign Port to socket
  int sock_status = bind(conn_socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if (sock_status < 0) {
    cerr << "Error with bind." << endl;
    exit(-1);
  }

  // Set socket to listen.
  int listen_status = listen(conn_socket, MAXPENDING);
  if (listen_status < 0) {
    cerr << "Error with listening." << endl;
    exit(-1);
  }
  cout << endl << endl << "SERVER: Ready to accept connections. " << endl;


  // Accept connections
  while (true) {
    // Accept connections
    struct sockaddr_in clientAddress;
    socklen_t addrLen = sizeof(clientAddress);
    int clientSocket = accept(conn_socket, (struct sockaddr*) &clientAddress, &addrLen);
    if (clientSocket < 0) {
      cerr << "Error accepting connections." << endl;
      exit(-1);
    }

    // Create child thread to handle process
    struct threadArgs* args_p = new threadArgs;
    args_p -> clientSock = clientSocket;
    pthread_t tid;
    int threadStatus = pthread_create(&tid, NULL, clientThread, (void*)args_p);
    if (threadStatus != 0){
      // Failed to create child thread
      cerr << "Failed to create child process." << endl;
      close(clientSocket);
      pthread_exit(NULL);
    }
    
  }

  return 0;
}

void* clientThread(void* args_p) {
  
  // Local Variables
  threadArgs* tmp = (threadArgs*) args_p;
  int clientSock = tmp -> clientSock;
  delete tmp;

  // Detach Thread to ensure that resources are deallocated on return.
  pthread_detach(pthread_self());

  // Communicate with Client
  InstantMessage(clientSock);

  // Close Client socket
  close(clientSock);

  // Quit thread
  pthread_exit(NULL);
}

void InstantMessage(int clientSock) {

  // Locals
  string clientMsg = "";
  long clientMsgLength = 0;
  int messageID = 1;
  fd_set clientfd;
  struct timeval tv;
  int numberOfSocks = 0;
  bool hasRead = true;

  // Login Credentials
  string userName;
  string userpwd;
  bool hasAuthenticated = false;

  // Login loop
  while(!hasAuthenticated) {
    long msgLength = GetInteger(clientSock);
    if (msgLength <= 0) {
      cerr << "Couldn't get integer from Client." << endl;
      break;
    }
    userName = GetMessage(clientSock, msgLength);
    if (clientMsg == "") {
      cerr << "Couldn't get message from Client." << endl;
      break;
    }
    msgLength = GetInteger(clientSock);
    if (msgLength <= 0) {
      cerr << "Couldn't get integer from Client." << endl;
      break;
    }
    userpwd = GetMessage(clientSock, msgLength);
    if (clientMsg == "") {
      cerr << "Couldn't get message from Client." << endl;
      break;
    }
    if (loginUser(userName, userpwd)) {
      hasAuthenticated = true;
      // Send back a successful message.
      stringstream ss;
      ss << "Login was successful!" << endl;
      string msg = ss.str();
      cout << msg;
      if (!SendInteger(clientSock, msg.length()+1)) {
	cerr << "Unable to send Int. " << endl;
	break;
      }
      if (!SendMessage(clientSock, msg)) {
	cerr << "Unable to send Message. " << endl;
	break;
      }
    } else {
      // Check if user exists, if not then add them.
      if (doesUserExist(userName)) {
	// Send back a failure message.
	stringstream ss;
	ss << "Login has failed.\n";
	string msg = ss.str();
	cout << msg;
	if (!SendInteger(clientSock, msg.length()+1)) {
	  cerr << "Unable to send Int. " << endl;
	  break;
	}
	if (!SendMessage(clientSock, msg)) {
	  cerr << "Unable to send Message. " << endl;
	  break;
	}
      } else {
	hasAuthenticated = true;
	// Send back a successful message.
	stringstream ss;
	ss << "Login was successful!" << endl;
	string msg = ss.str();
	cout << msg;
	if (!SendInteger(clientSock, msg.length()+1)) {
	  cerr << "Unable to send Int. " << endl;
	  break;
	}
	if (!SendMessage(clientSock, msg)) {
	  cerr << "Unable to send Message. " << endl;
	  break;
	}
      }
    }
  }

  // Clear FD_Set and set timeout.
  FD_ZERO(&clientfd);
  tv.tv_sec = 3;
  tv.tv_usec = 100000;

  // Initialize Data
  FD_SET(clientSock, &clientfd);
  numberOfSocks = clientSock + 1;

  while (clientMsg != "/quit" && clientMsg != "/close") {

    if (hasRead) {
      // Send Data.
      stringstream ss;
      ss << "This is Message #" << messageID++ << endl;
      string msg = ss.str();
      cout << msg << endl;
      if (!SendInteger(clientSock, msg.length()+1)) {
	cerr << "Unable to send Int. " << endl;
	break;
      }

      if (!SendMessage(clientSock, msg)) {
	cerr << "Unable to send Message. " << endl;
	break;
      }
      //hasRead = false;
    }

    // Read Data
    int pollSock = select(numberOfSocks, &clientfd, NULL, NULL, &tv);
    tv.tv_sec = 3;
    tv.tv_usec = 100000;
    FD_SET(clientSock, &clientfd);
    if (pollSock != 0 && pollSock != -1) {
      string tmp;
      long msgLength = GetInteger(clientSock);
      if (msgLength <= 0) {
	cerr << "Couldn't get integer from Client." << endl;
	break;
      }
      string clientMsg = GetMessage(clientSock, msgLength);
      if (clientMsg == "") {
	cerr << "Couldn't get message from Client." << endl;
	break;
      }
      tmp = "Client Said: ";
      tmp.append(clientMsg);
      cout << tmp << endl;
      tmp.clear();
      hasRead = true;
    }

  }

  cout << "Closing Thread." << endl;
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

void addToMsgQueue(Msg newMsg) {
  pthread_mutex_lock(&MsgQueueLock);
  MsgQueue.push_back(newMsg);
  pthread_mutex_unlock(&MsgQueueLock);
}

string GetMsgs(string username) {
  stringstream ss;
  pthread_mutex_lock(&MsgQueueLock);
  for (int i = 0; i < MsgQueue.size(); i++) {
    if (MsgQueue[i].to == username) {
      if (MsgQueue[i].cmd == "/msg") {
	// Msg was intended for our user.
	ss << "pm from " << MsgQueue[i].from << ": ";
	ss << MsgQueue[i].msg << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/all") {
	// Msg was intended for all users.
	ss << MsgQueue[i].from << " said: ";
	ss << MsgQueue[i].msg << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      }
    }
  }
  pthread_mutex_unlock(&MsgQueueLock);
  
  return ss.str();
}

void SaveMsg(string msg, string userFrom) {
  
  // Local Variables
  Msg newMsg;
  newMsg.msg = msg;
  newMsg.to = "";
  newMsg.from = userFrom;
  newMsg.cmd = "";
  processMsg(newMsg.msg, newMsg.cmd, newMsg.to);

  if (newMsg.to == "all") {
    // Global Message, need to add a message for all connected users.
  } else {
    // Regular Private message.
    addToMsgQueue(newMsg);
  }
}

void processMsg(string &msg, string &cmdName, string &userTo) {
  
  // Turn
  // "/msg user blahblahbah"
  // into
  // (blahblahblah, /msg, user)

  if (msg.c_str()[0] == '/') {
    // Was a Command, Let's figure out what it was.
    int cmdSize = 0;
    for (int i = 1; i < msg.length(); i++) {
      if (msg.c_str()[i] == ' ') {
	cmdSize = i;
	break;
      }
    }
    // Build Command name
    for (int i = 0; i < cmdSize; i++) {
      stringstream ss;
      ss << msg.c_str()[i];
      cmdName.append(ss.str());
    }

    int userSize = 0;
    if (cmdName == "/msg") {
      // Need to grab user information.
      for (int i = cmdSize; i < msg.length(); i++) {
	if (msg.c_str()[i] == ' ' ) {
	  userSize = i;
	  break;
	}
      }
      // Build UserTo
      for (int i = cmdSize; i < userSize; i++) {
	stringstream ss;
	ss << msg.c_str()[i];
	userTo.append(ss.str());
      }
    } else if (cmdName == "/all") {
      userTo.append("all");
    }
    
    // Set our values
    msg.replace(0, userSize+cmdSize, "");

  } else {
    userTo.append("all");
    cmdName = "/all";
  }
}

bool loginUser (string username, string password) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
    return false;
  } else {
    if (got->second.password == password) {
      got->second.isConnected = true;
      got->second.timeConnected = time(NULL);
      pthread_mutex_unlock(&UserListLock);
      return true;
    } else {
      pthread_mutex_unlock(&UserListLock);
      return false;
    }
  }
}

void addToUsersList (User newUser) {
  pthread_mutex_lock(&UserListLock);
  UsersList.insert (make_pair<string, User> (newUser.username, newUser));
  pthread_mutex_unlock(&UserListLock);
}

bool doesUserExist (string username) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::const_iterator got = UsersList.find (username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
    return false;
  } else {
    pthread_mutex_unlock(&UserListLock);
    return true;
  }
  
}

bool isUserConnected (User newUser) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::const_iterator got = UsersList.find (newUser.username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
    return false;
  } else {
    if (got->second.isConnected) {
      pthread_mutex_unlock(&UserListLock);
      return true;
    } else {
      pthread_mutex_unlock(&UserListLock);
      return false;
    }
  }
}

void setUserConnected (User newUser) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (newUser.username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
  } else {
    got->second.isConnected = true;
    got->second.timeConnected = time(NULL);
    pthread_mutex_unlock(&UserListLock);
  }
}

void setUserDisconnected (User newUser) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (newUser.username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
  } else {
    got->second.isConnected = false;
    pthread_mutex_unlock(&UserListLock);
  }
}
