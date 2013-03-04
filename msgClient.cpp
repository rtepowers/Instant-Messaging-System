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
WINDOW *INPUT_SCREEN;
WINDOW *MSG_SCREEN;

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

int main (int argNum, char* argValues[]) {

  // Locals
  string inputStr;

  // Begin User Interface
  initscr();

  // Set timeout of input to 2 miliseconds.
  halfdelay(2);

  // Create new MSG_SCREEN window and enable scrolling of text.
  MSG_SCREEN = newwin(LINES - INPUT_LINES, COLS, 0, 0);
  scrollok(MSG_SCREEN, TRUE);
  wsetscrreg(MSG_SCREEN, 0, LINES - INPUT_LINES - 1);
  wrefresh(MSG_SCREEN);

  // Create new INPUT_SCREEN window. No scrolling.
  INPUT_SCREEN = newwin(INPUT_LINES, COLS, LINES - INPUT_LINES, 0);
  
  // Prepare Input Screen.
  clearInputScreen();
  
  // Enter a loop to 
  while (true) {
    // If the user finished typing a message, get it and process it.
    if (getUserInput(inputStr)) {

      // If it's a command, handle it.
      if (inputStr == "/quit" || inputStr == "/close" || inputStr == "/exit")
	break;
          
      // Display the message in the chat window.
      displayMsg(inputStr);
      inputStr.clear();
          
      // Make sure to reset for the next user input.
      clearInputScreen();
    }
  }

  // Clean up and Close things down.
  delwin(INPUT_SCREEN);
  delwin(MSG_SCREEN);
  endwin();

  return 0;
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
  char userText;

  // Get input from from the input screen. Timeout will occur if no characters are available.
  userText = wgetch(INPUT_SCREEN);

  // Did we return a proper value?
  if (userText == (char)ERR)
    return false;

  // Can we display the text? Add to inputStr if yes.
  if (isprint(userText)) {
    inputStr += userText;
  }
  // Pressing <enter> signals that the user has 'sent' something.
  else if (userText == ENTER_SYM) {
    // If inputStr isn't empty, it should be submitted.
    if (inputStr.size() > 0) {
      success = true;
    }
  }

  return success;
}

void displayMsg(string &msg) {
  
  // Add Msg to screen.
  waddstr(MSG_SCREEN, msg.c_str());
  waddch(MSG_SCREEN, '\n');

  // Show new screen.
  wrefresh(MSG_SCREEN);
}
