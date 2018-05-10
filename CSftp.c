#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dir.h"
#include "dataConnection.h"
#include "usage.h"

#define MAXBUFFERSIZE 1024

/*
  Arguments:
    fd - a valid open file descriptor
    buff - buffer containing data to send
    numToSend - number of bytes to send

  Returns
    -1 an error occured while sending
    1 successfully sent numToSend bytes

  This sends specific number of bytes to a socket ensuring all sent
*/
int sendToSocket(int fd, char* buff, int numToSend) {
  int numSent = 0;
  while (numSent != numToSend) {
    if (numSent == -1) {
      // error occured, just return
      return -1;
    }
    numSent = numSent + send(fd, buff, numToSend, 0);
  }
  return 1;
}

/*
  Arguments:
    recvBuff - buffer containing data to send
    command - to store the command portion of buffer
    arg1 - to store the arg1 portion of buffer
    arg2 - to store the arg2 portion of buffer

  Returns
    0

  This function splits the incoming buffer information into command and parameters
*/
int getArguments(char* recvBuff, char** command, char** arg1, char** arg2) {
  // split the buffer into appropriate strings
  *command = strsep(&recvBuff, " ");
  *arg1 = strsep(&recvBuff, " ");
  *arg2 = strsep(&recvBuff, " ");

  // turn command into uppercase
  int maxLength = strlen(*command);
  char commandArray[maxLength+1];
  strncpy(commandArray, *command, maxLength);
  int i = 0;
  char c;
  while(i != maxLength) {
     commandArray[i] = (toupper(commandArray[i]));
     i++;
  }
  commandArray[maxLength] = '\0';
  strncpy(*command, commandArray, maxLength+1);

  return 0;
}

int main(int argc, char **argv) {

    // This is the main program for the thread version of nc
    int i;

    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    // initialize your sockaddr and buffers to 0
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    char sendBuff[MAXBUFFERSIZE];
    char recvBuff[MAXBUFFERSIZE];
    memset(&sendBuff, '0', sizeof(sendBuff));
    memset(&recvBuff, '0', sizeof(recvBuff));
    int n;
    int bytes;
    // start with no data connection and client not logged in
    int dataConnectionFd = -1;
    int clientLoggedIn = 0;

    // set the info for your sockaddr for the server connection
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // create the server socket
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    int connectedfd;
    if (fd == -1) {
      printf("There was an error creating the server socket");
      return -1;
    }

    // bind the socket to the port it should listen to
    bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    // listen, waiting for 1 active connection
    listen(fd, 1);

    // initialize current working directory (for CDUP)
    char serverDir[MAXBUFFERSIZE];
    char currDirectory[MAXBUFFERSIZE];
    getcwd(serverDir, sizeof(serverDir));

    // initialize variables for client commands and arguments
    char *command, *arg1, *arg2;

    while (1) {
      // accept client connection
      connectedfd = accept(fd, (struct sockaddr*)NULL, NULL);
      if (connectedfd == -1) {
        continue;
      }

      // send welcome message
      sprintf(sendBuff, "220 Service ready for new user.\r\n");
      bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));

      // loop while connected to client
      while (1) {
        n = 0;
        // receiving information in the send buffer
        while (1) {
          bytes = recv(connectedfd, &recvBuff[n], 1, 0);
          // process the information looking for end of message
          if (bytes <= 0) {
            // there is nothing left in the buffer to read or there was an error
            break;
          }
          if (recvBuff[n] == '\n') {
            recvBuff[n] = '\0';
            break;
          }
          if (recvBuff[n] != '\r') {
            // ignore carriage return symbols
            n++;
          }
        }
        if (bytes <= 0) {
          // either nothing in the buffer so the connection was closed as per Beejs
          // or an error receiving. Close connection either way.
          break;
        }

        // parse the information in the buffer
        getArguments(recvBuff, &command, &arg1, &arg2);

        // for any command other than those we support, return error code 500
        if ((strlen(command) == 3 && (strncmp(command, "CWD", 3) !=0))
          || ((strlen(command) == 4) && (strncmp(command, "USER", 4) !=0)
              && (strncmp(command, "QUIT", 4) !=0) && (strncmp(command, "CWD", 3) !=0)
              && (strncmp(command, "CDUP", 4) !=0) && (strncmp(command, "TYPE", 4) !=0)
              && (strncmp(command, "MODE", 4) !=0) && (strncmp(command, "STRU", 4) !=0)
              && (strncmp(command, "RETR", 4) !=0) && (strncmp(command, "PASV", 4) !=0)
              && (strncmp(command, "NLST", 4) !=0))
          || !((strlen(command) == 3 || strlen(command) == 4))) {
            // 500 error code as per TA
            sprintf(sendBuff, "500 Syntax error, command unrecognized\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            continue;
        }

        if (strncmp(command, "USER", 4) == 0) {
        // assuming here no extra spaces between USER and the user name
          if (clientLoggedIn) {
            // as per TA, already logged in
            sprintf(sendBuff, "230 User logged in, proceed.\r\n");
          } else if (arg2 != NULL || arg1 == NULL) {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          } else if (strlen(arg1) == 9 && (strncmp(arg1, "anonymous", 9) == 0)) {
            clientLoggedIn = 1;
            sprintf(sendBuff, "230 User logged in, proceed.\r\n");
          } else {
            sprintf(sendBuff, "530 Not logged in.\r\n");
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        // allow client to quit even if not logged in
        if (strncmp(command, "QUIT", 4) == 0) {
          if (arg1 == NULL && arg2 == NULL) {
            sprintf(sendBuff, "221 Service closing control connection.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            break;
          } else {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            continue;
          }
        }

        // check whether client is logged in to perform other commands
        if (clientLoggedIn == 0) {
          sprintf(sendBuff, "530 Not logged in.\r\n");
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "CWD", 3) == 0) {
          if (arg1 == NULL || arg2 != NULL) {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          } else if ((strlen(arg1) > 1 && (strncmp(arg1, "./", 2) == 0))
          || (strlen(arg1) > 2 && (strstr(arg1, "../") != NULL))
          || (strlen(arg1) == 2 && strncmp(arg1, "..", 2) == 0)
          || (strlen(arg1) == 1 && strncmp(arg1, ".", 1) == 0)
          || (strlen(arg1) > 0 && strncmp(arg1, "/", 1) == 0)) {
            // checks and responses as per class assignment description for security reasons
            sprintf(sendBuff, "550 Requested action not taken.\r\n");
          } else {
            // try to change to requested directory
            if (chdir(arg1) == 0) {
              sprintf(sendBuff, "200 directory changed to %s\r\n", arg1);
            } else {
              sprintf(sendBuff, "550 Requested action not taken.\r\n");
            }
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "CDUP", 4) == 0) {
          getcwd(currDirectory, sizeof(currDirectory));
          if (arg1 != NULL || arg2 != NULL) {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          } else if (strncmp(serverDir, currDirectory, MAXBUFFERSIZE) == 0) {
            // Do not allow client to access parent of server directory
            sprintf(sendBuff, "550 Requested action not taken.\r\n");
          } else {
            // try to move up a directory
            if (chdir("..") == 0) {
              sprintf(sendBuff, "250 Requested file action okay, completed.\r\n");
            } else {
              sprintf(sendBuff, "550 Requested action not taken.\r\n");
            }
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "TYPE", 4) == 0) {
          // treat image and ascii as the same, as per assignment description
          if (arg1 != NULL && arg2 == NULL && strlen(arg1) == 1
            && ((strncmp(arg1, "I", 1) == 0)||(strncmp(arg1, "i", 1) == 0))) {
              sprintf(sendBuff, "200 Command okay.\r\n");
          } else if (arg1 != NULL && (arg2 == NULL || ((strlen(arg2) == 1) && ((strncmp(arg2, "N", 1) == 0)||(strncmp(arg2, "n", 1) == 0))))
            && strlen(arg1) == 1 && ((strncmp(arg1, "A", 1) == 0)||(strncmp(arg1, "a", 1) == 0))) {
              // support second parameter of N for ascii type, doesn't change implementation
              sprintf(sendBuff, "200 Command okay.\r\n");
          } else if (arg1 != NULL && strlen(arg1) == 1 && ((strncmp(arg1, "E", 1) == 0)||(strncmp(arg1, "e", 1) == 0)||(strncmp(arg1, "L", 1) == 0)||(strncmp(arg1, "l", 1) == 0))) {
              // for E and L, we don't support those parameters
              sprintf(sendBuff, "504 Command not implemented for that parameter\r\n");
          } else {
            // Any other parameter or invalid syntax
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "MODE", 4) == 0) {
          if (arg1 != NULL && arg2 == NULL && strlen(arg1) == 1 && ((strncmp(arg1, "S", 1) == 0)||(strncmp(arg1, "s", 1) == 0))) {
              sprintf(sendBuff, "200 Mode set to S.\r\n");
          } else if (arg1 != NULL && arg2 == NULL && strlen(arg1) == 1 && ((strncmp(arg1, "B", 1) == 0)||(strncmp(arg1, "b", 1) == 0)||(strncmp(arg1, "C", 1) == 0)||(strncmp(arg1, "c", 1) == 0))) {
              // for B & C, don't support that parameter
              sprintf(sendBuff, "504 Command not implemented for that parameter\r\n");
          } else {
              // Any other parameter or invalid syntax
              sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "STRU", 4) == 0) {
          if (arg1 != NULL && arg2 == NULL && strlen(arg1) == 1 && ((strncmp(arg1, "F", 1) == 0)||(strncmp(arg1, "f", 1) == 0))) {
              sprintf(sendBuff, "200 Structure set to F.\r\n");
          } else if (arg1 != NULL && arg2 == NULL && strlen(arg1) == 1 && ((strncmp(arg1, "R", 1) == 0)||(strncmp(arg1, "r", 1) == 0)||(strncmp(arg1, "P", 1) == 0)||(strncmp(arg1, "p", 1) == 0))) {
              // for R & P, don't support that parameter
              sprintf(sendBuff, "504 Command not implemented for that parameter\r\n");
          } else {
              // any other parameter or invalid syntax
              sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }

        if (strncmp(command, "RETR", 4) == 0) {
          if (arg1 == NULL || arg2 != NULL) {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          } else if (dataConnectionFd < 0) {
            // either data connection was not created, timed out, or had an error
            sprintf(sendBuff, "425 Can't open data connection.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          } else {
            // arguments ok and we have a data connection
            sprintf(sendBuff, "125 Data connection already open; transfer starting.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            FILE *fp;
            char buff[255];
            // open the requested file
            fp = fopen(arg1, "r");
            int sent = 0;
            // if file exists and is open, send data in file
            if (fp != NULL) {
            int read = fread(buff, 1, 255, fp);
              while (read == 255) {
                sent = sendToSocket(dataConnectionFd, buff, read);
                read = fread(buff, 1, 255, fp);
              }
              // send the last little buffer of data
              sent = sendToSocket(dataConnectionFd, buff, read);
              fclose(fp);
              sprintf(sendBuff, "226 Closing data connection. Requested file action successful.\r\n");
            } else {
              // as per assignment description
              sprintf(sendBuff, "550 Requested action not taken.\r\n");
            }
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            // close down the data connection since finished
            shutdown(dataConnectionFd, SHUT_RDWR);
            close(dataConnectionFd);
            dataConnectionFd = -1;
          }
          continue;
        }

        if (strncmp(command, "PASV", 4) == 0) {
          if (arg1 != NULL || arg2 != NULL) {
            sprintf(sendBuff, "504 Command not implemented for that parameter.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          } else {
            // create a data connection
            dataConnectionFd = makeDataConnection(connectedfd);
          }
          continue;
        }

        if (strncmp(command, "NLST", 4) == 0) {
          if (arg1 != NULL || arg2 != NULL) {
            sprintf(sendBuff, "501 Syntax error in parameters or arguments.\r\n");
          } else if (dataConnectionFd < 0) {
            // either data connection was not created, timed out, or had an error
            sprintf(sendBuff, "425 Can't open data connection.\r\n");
          } else {
            // data connection created and arguments ok
            sprintf(sendBuff, "125 Data connection already open; transfer starting.\r\n");
            bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
            // call listFiles to print the files in the directory
            if (listFiles(dataConnectionFd, ".") < 0) {
              // there was a problem reading the directory and printing
              sprintf(sendBuff, "451 Requested action aborted. Local error in processing.\r\n");
            } else {
              // reading was successful
              sprintf(sendBuff, "226 Closing data connection. Requested file action successful.\r\n");
            }
            // close the data connection since finished
            shutdown(dataConnectionFd, SHUT_RDWR);
            close(dataConnectionFd);
            dataConnectionFd = -1;
          }
          bytes = sendToSocket(connectedfd, sendBuff, strlen(sendBuff));
          continue;
        }
      }
      // close the connection socket and log out user, return to server directory
      clientLoggedIn = 0;
      dataConnectionFd = -1;
      shutdown(connectedfd, SHUT_RDWR);
      close(connectedfd);
      chdir(serverDir);
    }
    // close the server socket
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return 0;

}
