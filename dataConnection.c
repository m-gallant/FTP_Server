#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usage.h"

#define MAXBUFFERSIZE 1024

/*
   Arguments:
      connectFd - a valid open file descriptor for the control connection

   Returns
      -1 Data connection failed to open
      - file descriptor for opened data connection


   This function makes a data connection.


 */
int makeDataConnection(int connectFd) {

  // get a random port number
  srand(0);
  int portNumber = 1025 + rand()%64510;

  // initialize sockaddrs to 0
  struct sockaddr_in data_addr, name;
  memset(&data_addr, '0', sizeof(data_addr));
  memset(&name, '0', sizeof(name));

  // create data socket
  int dataBytes;
  fd_set sockSet;
  int dataFd = socket(PF_INET, SOCK_STREAM, 0);
  int connectDataFd;

  FD_ZERO(&sockSet);
  FD_SET(dataFd, &sockSet);

  // set up data_addr info and get the IP address from the control connection
  data_addr.sin_family = AF_INET;
  data_addr.sin_port = htons(portNumber);
  socklen_t namelen = sizeof(name);
  int err = getsockname(connectFd, (struct sockaddr*)&name, &namelen);
  data_addr.sin_addr = name.sin_addr;

  // try to bind socket for data connection, may have to try a few port numbers
  while(bind(dataFd, (struct sockaddr*)&data_addr, sizeof(data_addr)) != 0) {
    portNumber = 1025 + rand()%64510;
    data_addr.sin_port = htons(portNumber);
  }

  // set the socket to listen
  int listening = listen(dataFd, 1);

  // get ip address and format to send to client control connection
  char* addrString = inet_ntoa(data_addr.sin_addr);
  char *section1, *section2, *section3, *section4;
  section1 = strsep(&addrString, ".");
  section2 = strsep(&addrString, ".");
  section3 = strsep(&addrString, ".");
  section4 = strsep(&addrString, ".");

  // use connectFd to communicate where you are listening to client
  char dataSendBuff[MAXBUFFERSIZE];
  memset(&dataSendBuff, '0', sizeof(dataSendBuff));
  sprintf(dataSendBuff, "227 Entering Passive Mode (%s,%s,%s,%s,%d,%d).\r\n", section1, section2, section3,section4, portNumber/256, portNumber%256);
  dataBytes = send(connectFd, dataSendBuff, strlen(dataSendBuff), 0);

  // set dataFd socket timeout to 20 seconds in case client doesn't connect
  struct timeval timeout = {20,0};
  if (select(dataFd + 1, &sockSet, NULL, NULL, &timeout) == 0) {
    // if we timeout, shut down the data socket and return -1
    shutdown(dataFd, SHUT_RDWR);
    close(dataFd);
    return -1;
  }

  // accept connection
  connectDataFd = accept(dataFd, (struct sockaddr*)NULL, NULL);

  // return error or data connection file descriptor
  if (connectDataFd < 0) {
    return -1;
  } else {
    shutdown(dataFd, SHUT_RDWR);
    close(dataFd);
    return connectDataFd;
  }
}
