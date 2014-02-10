// AUTHOR: Raymond Powers
// DATE: November 26th, 2012
// PLATFORM: C++ 

// DESCRIPTION: This program serves as the server portion of the instant messaging system.

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

void setUserDisconnected (string username);
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

bool hasAuthenticated(int clientSock, string &userName);
// Function handles authentication of users.
// pre: none
// post: none

void broadcastMsg(string userName, string msg, bool isConnected);
// Function allows system to create a broadcast message to all other users.
// pre: none
// post: none

string GrabUsers(string userName);
// Function returns a list of connected users.
// pre: none
// post: none

string GrabTime(string userName);
// Function returns a duration of a user.
// pre: none
// post: none

string GrabJoke();
// Function returns a joke.
// pre: none
// post: none

string GrabPic();
// Function returns a picture
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

  // Login loop
  while (!hasAuthenticated(clientSock, userName)) {
  }

  // Announce That user has connected!
  broadcastMsg( userName, "", true);
  // TODO

  // Clear FD_Set and set timeout.
  FD_ZERO(&clientfd);
  tv.tv_sec = 2;
  tv.tv_usec = 100000;

  // Initialize Data
  FD_SET(clientSock, &clientfd);
  numberOfSocks = clientSock + 1;

  while (clientMsg != "/quit" && clientMsg != "/close") {

    // Send Data.
    string msg = GetMsgs(userName);
    if (msg.length() != 0) {
      //cout << msg << endl;
      if (!SendInteger(clientSock, msg.length()+1)) {
	cerr << "Unable to send Int. " << endl;
	break;
      }

      if (!SendMessage(clientSock, msg)) {
	cerr << "Unable to send Message. " << endl;
	break;
      }
    }
    

    // Read Data
    int pollSock = select(numberOfSocks, &clientfd, NULL, NULL, &tv);
    tv.tv_sec = 1;
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
      
      // Process message and Add to queue
      SaveMsg(clientMsg, userName); 
    }
  }//*/

  cout << "Closing Thread." << endl;
  setUserDisconnected (userName);
  // Announce that user has disconnected
  broadcastMsg(userName, "", false);
}

void broadcastMsg(string userName, string msg, bool isConnected) {

  if (msg == "") {
    // This is a login/logoff announcement.
    msg = "";
    Msg tmp;
    tmp.to;
    tmp.from = userName;
    tmp.msg = msg;
    tmp.cmd = "/all";

    pthread_mutex_lock(&UserListLock);
    tr1::unordered_map<string, User>::iterator got = UsersList.begin();
    for ( ; got != UsersList.end(); got++) {
      if ((got)->second.isConnected == true && (got)->second.username != userName) {
	// Send them a message.
	tmp.to = (got)->second.username;
	msg.append (userName);
	if (isConnected) {
	  msg.append (" has connected! :)\n");
	} else {
	  msg.append (" has disconnected! :(\n");
	}
	tmp.msg = msg;
	msg.clear();
	addToMsgQueue(tmp);
	tmp.msg.clear();
	//SaveMsg(msg, userName);
	
      }
    }
    pthread_mutex_unlock(&UserListLock);
  } else {
    string globMsg = "";
    globMsg.append (userName);
    globMsg.append (" has said: ");
    globMsg.append (msg);
    Msg tmp;
    tmp.from = userName;
    tmp.msg = globMsg;
    tmp.cmd = "/all";
    // This is a global Msg
    pthread_mutex_lock(&UserListLock);
    tr1::unordered_map<string, User>::iterator got = UsersList.begin();
    for ( ; got != UsersList.end(); got++) {
      if ((got)->second.isConnected == true && (got)->second.username != userName) {
	// Send them a message.
	tmp.to = (got)->second.username;
	addToMsgQueue(tmp);
	//SaveMsg(msg, userName);
	
      }
    }
    pthread_mutex_unlock(&UserListLock);
  }
}


bool hasAuthenticated(int clientSock, string &userName) {

  // Locals
  string loginSuccessMsg = "Login Successful!\n";
  string loginFailureMsg = "Login Failed!\n";
  string userPwd;
  long responseLen = 0;
  string clientResponse;

  // Get UserName
  responseLen = GetInteger(clientSock);
  userName = GetMessage(clientSock, responseLen);

  // Get Password
  responseLen = GetInteger(clientSock);
  userPwd = GetMessage(clientSock, responseLen);
  
  // Need to process username and password
  if (loginUser (userName, userPwd)) {
    // User Exists and password was successful.
    // Send message to client
    SendInteger(clientSock, loginSuccessMsg.length()+1);
    SendMessage(clientSock, loginSuccessMsg);
    cout << "Logged in as: " << userName << endl;
    return true;
  } else {
    // User could not login.
    SendInteger(clientSock, loginFailureMsg.length()+1);
    SendMessage(clientSock, loginFailureMsg);
    cout << "Failed to login as: " << userName << endl;
    return false;
  }
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
	ss << "/\b\n************************************\npm from " << MsgQueue[i].from << ": ";
	ss << MsgQueue[i].msg << endl << "************************************" << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/all") {
	// Msg was intended for all users.
	ss << MsgQueue[i].msg << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/users") {
	ss << MsgQueue[i].msg << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/poke" ) {
	ss << "/\b\n" << MsgQueue[i].from << " has poked you!" << endl;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/time" ) {
	ss << MsgQueue[i].msg;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/joke" ) {
	ss << MsgQueue[i].msg;
	MsgQueue.erase (MsgQueue.begin()+i);
	i--;
      } else if (MsgQueue[i].cmd == "/picture" ) {
	ss << MsgQueue[i].msg;
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

  if (newMsg.cmd == "/all") {
    // Global Message, need to add a message for all connected users.
    broadcastMsg(userFrom, msg, false);
  } else if (newMsg.cmd == "/msg") {
    // Regular Private message.
      if (doesUserExist(newMsg.to)) {
	addToMsgQueue(newMsg);
      }
  } else if (newMsg.cmd == "/users") {
    newMsg.to = userFrom;
    newMsg.from = "SERVER";
    newMsg.msg = GrabUsers(userFrom);
    addToMsgQueue(newMsg);
  } else if (newMsg.cmd == "/poke") {
      if (doesUserExist(newMsg.to)) {
	addToMsgQueue(newMsg);
      }
  } else if (newMsg.cmd == "/time") {
    newMsg.from = "SERVER";
    if (newMsg.to == "") {
      newMsg.msg = GrabTime(userFrom);
    } else {
      newMsg.msg = GrabTime(newMsg.to);
    }
    newMsg.to = userFrom;
    addToMsgQueue(newMsg);
  } else if (newMsg.cmd == "/joke") {
    newMsg.to = userFrom;
    newMsg.from = "SERVER";
    newMsg.msg = GrabJoke();
    addToMsgQueue(newMsg);
  } else if (newMsg.cmd == "/picture") {
    newMsg.to = userFrom;
    newMsg.from = "SERVER";
    newMsg.msg = GrabPic();
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
    if (cmdSize == 0) {
      // must be a non-argument command.
      cmdSize = msg.length();
    }
    // Build Command name
    for (int i = 0; i < cmdSize; i++) {
      stringstream ss;
      ss << msg.c_str()[i];
      cmdName.append(ss.str());
    }

    int userSize = 0;
    if (cmdName == "/msg" || cmdName == "/poke" || cmdName == "/time") {
      // Need to grab user information.
      for (int i = cmdSize+1; i < msg.length(); i++) {
	if (msg.c_str()[i] == ' ' ) {
	  userSize = i;
	  break;
	}
      }
      
      if (userSize == 0) {
	// no message, just action
	userSize = msg.length();
      }

      // Build UserTo
      for (int i = cmdSize+1; i < userSize; i++) {
	stringstream ss;
	ss << msg.c_str()[i];
	userTo.append(ss.str());
      }
    
      // Set our values
      msg.replace(0, userSize+cmdSize-3, "");
    } else if (cmdName == "/users" || cmdName == "/joke" || cmdName == "/picture") {
      // Need to process outside this function.
    } 


  } else {
    userTo.append("all");
    cmdName = "/all";
  }
}

string GrabPic() {

  stringstream welcomeMsg;
  welcomeMsg <<"/\b#############################################################"<<endl;
  welcomeMsg <<"#                    _                                      #"<<endl;  
  welcomeMsg <<"#                  -=\\`\\                                    #"<<endl;  
  welcomeMsg <<"#              |\\ ____\\_\\__                                 #"<<endl;  
  welcomeMsg <<"#            -=\\c`\"\"\"\"\"\"\" \"`)                               #"<<endl;  
  welcomeMsg <<"#               `~~~~~/ /~~`                                #"<<endl;  
  welcomeMsg <<"#                 -==/ /                                    #"<<endl;  
  welcomeMsg <<"#                   '-'                                     #"<<endl;  
  welcomeMsg <<"#                  _  _                                     #"<<endl;  
  welcomeMsg <<"#                 ( `   )_                                  #"<<endl;  
  welcomeMsg <<"#                (    )    `)                               #"<<endl;  
  welcomeMsg <<"#              (_   (_ .  _) _)                             #"<<endl;  
  welcomeMsg <<"#                                             _             #"<<endl;  
  welcomeMsg <<"#                                            (  )           #"<<endl;  
  welcomeMsg <<"#             _ .                         ( `  ) . )        #"<<endl;  
  welcomeMsg <<"#           (  _ )_                      (_, _(  ,_)_)      #"<<endl;  
  welcomeMsg <<"#         (_  _(_ ,)                                        #"<<endl; 
  welcomeMsg <<"#############################################################"<<endl;

  return welcomeMsg.str();
}

string GrabJoke() {

  stringstream ss;
  // Need to get a random number.
  srand (time(NULL));
  int rndNum = rand() % 10 + 1; // between 1 and 10

  // Grab the joke!
  switch (rndNum) {
  case 1:
    ss << "/\bMost people believe that if it ain't broke, don't fix it. Engineers believe that if it ain't broke, it doesn't have enough features yet." << endl;
    break;
  case 2:
    ss << "/\bQ: How does a computer tell you it needs more memory?   A: It says ''byte me''" << endl;
    break;
  case 3:
    ss << "/\bQ: What is the first programming language you learn when studying computer science?  A: Profanity" << endl;
    break;
  case 4:
    ss << "/\bA blind man walks into a bar...   and a chair and a table." << endl;
    break;
  case 5:
    ss << "/\bQ: Why don't cows make large bets?   A: The steaks are too high." << endl;
    break;
  case 6:
    ss << "/\bQ: Why aren't jokes in base 8 funny?   A: Because 7, 10, 11." << endl;
    break;
  case 7:
    ss << "/\bQ: What did people say after two satellite dishes got married?   A: The wedding was dull, but the reception was great." << endl;
    break;
  case 8:
    ss << "/\bQ: If Al Gore tried his hand as a musician, what would his album be called?   A. Algorithms." << endl;
    break;
  case 9:
    ss << "/\bA programmer goes to do groceries. His wife tells him: \n-- Buy a loaf of bread, and if they have eggs, buy a dozen.\n";
    ss << " He comes back with thirteen loaves of bread.\n -- 'But why?', she asks.\n --'They had eggs.'" << endl;
    break;
  case 10:
    ss << "/\bSilly chat person, NO JOKE FOR YOU!" << endl;
    break;
  default:
    ss << "/\bSilly chat person, NO JOKE FOR YOU!" << endl;
    break;
  }
  
  return ss.str();
}

string GrabTime(string userName) {

  stringstream ss;

  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (userName);
 if (got == UsersList.end() ) {
    // User not in list
    ss << "/\bCould not find: " << userName << endl;
 } else {
   // User was found.
   if ((got)->second.isConnected) {
     ss << "/\b" << userName << " has been connected for "
	<< time(NULL) - (got)->second.timeConnected << " seconds." << endl;
   } else {
     ss << "/\b" << userName << " is not connected." << endl;
   }
 }
 pthread_mutex_unlock(&UserListLock);

  return ss.str();
}

string GrabUsers(string userName) {

  stringstream ss;
  int numOfUsers = 1;
  ss << "/\bConnected Users: " << endl;

  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.begin();
  for ( ; got != UsersList.end(); got++) {
    if ((got)->second.isConnected == true) {
      // Add to list
      if ((got)->second.username == userName) {
	ss << numOfUsers++ << ". " << "You" << endl;
      } else {
	ss << numOfUsers++ << ". " << (got)->second.username << endl;
      }
    }
  }
  pthread_mutex_unlock(&UserListLock);

  return ss.str();
}

bool loginUser (string username, string password) {
  // locals
  User newUser;
  newUser.username = username;
  newUser.password = password;
  newUser.isConnected = true;
  newUser.timeConnected = time(NULL);
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (username);
  if (got == UsersList.end() ) {
    // User not in list, so let's add them!
    UsersList.insert (make_pair<string, User> (newUser.username, newUser));
    pthread_mutex_unlock(&UserListLock);
    return true;
  } else {
    if (got->second.password == password) {
      if (got->second.isConnected) {
	// someone else is already connected.
	pthread_mutex_unlock(&UserListLock);
	return false;
      } else {
	// Password matches, and not connected.
	got->second.isConnected = true;
	got->second.timeConnected = time(NULL);
	pthread_mutex_unlock(&UserListLock);
	return true;
      }
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

void setUserDisconnected (string username) {
  pthread_mutex_lock(&UserListLock);
  tr1::unordered_map<string, User>::iterator got = UsersList.find (username);
  if (got == UsersList.end() ) {
    pthread_mutex_unlock(&UserListLock);
  } else {
    got->second.isConnected = false;
    pthread_mutex_unlock(&UserListLock);
  }
}
