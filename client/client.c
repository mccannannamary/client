#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client.h"


int main()  {

    WSADATA wsaData;        /* structure to hold winsock data */
    SOCKET clientSocket;    /* handle used to hold reference socket */

    char *serverIP;
    char *host;
    char *path;
    char fileOut[100];

    int serverPort = 80;
    int retVal;

    retVal = WSAStartup(MAKEWORD(2,2), &wsaData);       /* initialise Winsock system */
    if (retVal != 0) {
        printf("\nmain: WSAStartup unsuccessful:\t%d.\n", retVal);
        printError();
        return 1;
    }

    serverIP = (char *) malloc(20*sizeof(char));

    clientSocket = createSocket();

    retVal = connectToServer(clientSocket, serverIP, serverPort, host, path, fileOut);
    if (retVal != 0) {
        printf("\nmain: cannot connect to server.\n");
        exit(EXIT_FAILURE);
    }

    retVal = processServerResponse(clientSocket, fileOut);
    if (retVal != 0) {
        printf("\nmain: cannot process server response.\n");
        exit(EXIT_FAILURE);
    }

    retVal = shutdown(clientSocket, SD_SEND);       /* shutdown sending end of connection first */
    if (retVal == 0) {
        printf("\nmain: shutdown successful.");
    }
    else {
        printf("\nmain: shutdown error %d", retVal);
        exit(EXIT_FAILURE);
    }

    retVal = closesocket(clientSocket);             /* completely close connection */
    if (retVal == 0) {
        printf("\nmain: successfully closed socket.");
    }
    else {
        printf("\nmain: closesocket error %d", retVal);
        exit(EXIT_FAILURE);
    }

    retVal = WSACleanup();                          /* cleanup winsock system */
    if (retVal != 0) {
        printf("main: error cleaning up system\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
