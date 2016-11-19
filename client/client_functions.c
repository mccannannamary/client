#define WINVER 0x0501
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client.h"

#define LEN_CLIENT_REQUEST 300
#define LEN_RESPONSE_STREAM 100
#define WRITE_BIN_MODE "wb"
#define LEN_PATH 200

SOCKET createSocket(void) {

    SOCKET clientSocket;                                        /* handle used to hold reference socket */

    /* create a socket */
    clientSocket = INVALID_SOCKET;                              /* initialise handle clientSocket */
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   /* create the socket to work with streams of bytes, using IPv4 and TCP */
    if (clientSocket == INVALID_SOCKET) {
        printf("\ncreateSocket: error - unable to create socket\n");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Socket created.\n");
    }
    return clientSocket;
}

int connectToServer(SOCKET clientSocket, char *serverIP, int serverPort, char *host, char *path, char fileOut[]) {

    char user_URL[400];
	char *temp;
	char **parsed_URL;

	int count;
	int i;

    int retVal;

    int lenClientRequest;
    char clientRequest[LEN_CLIENT_REQUEST];

	/* ==== Get a URL from the user, parse into host and path ==== */

	printf("\nPlease enter the host and path together, as a URL:\t");
	scanf("%s", user_URL);

    /* Parse URL into host and path */
	parsed_URL = (char **) malloc(sizeof(char *));    /* dynamically allocate memory for the array of tokens */
	temp = strtok(user_URL, "/");                     /* on first call to strtok, provide it with string which is to be parsed */
	count = 0;

	while (temp != NULL) {                  /* strtok returns null pointer when no tokens are left to parse */
        *(parsed_URL + count) = temp;       /* populate parsed_URL with tokens from user_URL */
        count++;
        parsed_URL = realloc(parsed_URL, (count + 1)*sizeof(char *));   /* dynamically update size of parsed_URL */
        temp = strtok(NULL, "/");                                       /* subsequent calls to strtok, first parameter is NULL */
	}

	host = malloc( (sizeof(char) * strlen(*parsed_URL)) );
    host = *parsed_URL;

    path = malloc (sizeof(char)*LEN_PATH);
    sprintf(path, "/%s", *(parsed_URL+1) );


    for (i = 2; i < count; i++) {                                         /* add together the parts of the path that have been parsed above */
        sprintf(path + strlen(path), "/%s", *(parsed_URL+i));             /* retVal contains number of characters written to string, not including null character, use for transmission */
    }

    strcpy(fileOut, *(parsed_URL+count-1));        /* output file name is last token */

    /* =============================================== */


    /* ==== Get the IP address from the host name ==== */

    /* Create a structure of type addrinfo */
    struct addrinfo hints;  /* hints for the getaddrinfo function */
    /* Create a pointer - will point to a similar stucture later */
    struct addrinfo *result = NULL;  /* to hold result of getaddrinfo */

    /* Initialise the hints structure - all zero */
    ZeroMemory( &hints, sizeof(hints) );

    /* Fill in some parts, to specify what type of address we want */
    hints.ai_family = AF_INET;  /* IPv4 address */
    hints.ai_socktype = SOCK_STREAM;  /* stream type socket */
    hints.ai_protocol = IPPROTO_TCP;  /* using TCP protocol */

    /* Try to find the address of the host computer  */
    retVal = getaddrinfo(host, NULL, &hints, &result);
    if (retVal != 0)
    {
        printf("Error in getaddrinfo, code %d\n", retVal);
        WSACleanup();
        return 1;
    }

    /* One or more result structures have been created by getaddrinfo(), */
    /* forming a linked list - a computer could have multiple addesses. */
    /* The result pointer points to the first structure, and this */
    /* example only uses the first address found. */

    /* Create pointer to address structure to hold the address we want */
    struct sockaddr_in *hAddrPtr;

    /* Get pointer to address structure from result, and cast as needed */
    hAddrPtr = (struct sockaddr_in*) result->ai_addr;

    /* Create IP address structure and get IP address from sockaddr_in struct */
    struct in_addr hAddr = hAddrPtr->sin_addr;  // get IP address in network form
    /* this is the form you need to request a connection... */

    /* Free the memory that getaddrinfo allocated for result structure(s) */
    freeaddrinfo(result);

    /* Print the result, after converting to string form (dotted decimal) */
    serverIP = inet_ntoa(hAddr);

    /* ================================================================== */


    /* ==== Connect to the server ==== */
    struct sockaddr_in service;

    /* create sockaddr_in structure */
    service.sin_family = AF_INET;                       /* set family to IP version 4 */
    service.sin_addr.s_addr = inet_addr(serverIP);      /* convert string IP address to 32-bit integer, assign to service */
    service.sin_port = htons(serverPort);               /* convert port number to form used on internet, assign to service */

    /* set up a connection to the server */
    retVal = connect(clientSocket, (SOCKADDR *) &service, sizeof(service));
    if (retVal != 0) {
        printf("\nconnectToServer: error %d.", retVal);
        return 1;
    }

    /* ================================ */

    /* ==== Send a GET request ==== */

    retVal = sprintf(clientRequest, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, host);
        printf("\nconnectToServer, cannot create GET request.");
        return 1;
    }
    free(host);
    free(path);
    lenClientRequest = strlen(clientRequest);
    retVal = send(clientSocket, clientRequest, lenClientRequest, 0);               /* send GET request */
    if (retVal == SOCKET_ERROR) {
        printf("\nconnectToServer: error %d.", retVal);
        return 1;
    }
    else if (retVal != lenClientRequest)  {                                              /* send should return number of bytes sent */
        printf("\nconnectToServer: %d bytes of %d bytes sent.", retVal, lenClientRequest);
        return 1;
    }
    else {
        printf("\nRequest sent.\n");
    }
    return 0;

    /* ======================================================== */
}



/* function to receive the server response and store the file requested on computer's disk */
int processServerResponse(SOCKET clientSocket, char fileOut[]) {

    FILE *fpout;

    int stop;
    int nRx;
    int nRxleftover;
    int retVal;
    int i;
    int iSave;

    char *serverResponse;
    char *retChar;

    const char endOfHeader[] = "\r\n\r\n";

    serverResponse = (char *) malloc(LEN_RESPONSE_STREAM*sizeof(char));

    /* loop prints all of http header except the last stream, which contains the 2*<CR><LF> */
    stop = 1;
    do {
        nRx = recv(clientSocket, serverResponse, LEN_RESPONSE_STREAM, 0);       /* put returned stream into serverResponse, returns number of bytes actually received */
        retChar = strstr(serverResponse, endOfHeader);                          /* check if the response stream contains the end of the header string 2x<CR><LF> */
        if (retChar != NULL) {                                                  /* Indicates we have found the end of the header */
            stop = 0;                                                           /* exit the loop that is printing the response to the screen */
        }
        if (nRx != LEN_RESPONSE_STREAM) {
            printf("\nprocessServerResponse: %d bytes received of %d bytes requested.", nRx, LEN_RESPONSE_STREAM);
            return 1;
        }
        else if (nRx == -1) {
            printf("\nprocessServerResponse: error %d.", nRx);
            return 1;
        }
        else if (nRx == 0) {
            printf("\nprocessServerResponse: server has closed connection.");   /* there are no more bytes left to receive */
            stop = 0;
        }
        else {
            if (stop != 0) {
                for (i = 0; i < nRx; i++) {
                    printf("%c", *(serverResponse + i));
                }
            }
        }
    }
    while(stop != 0);

    for (i = 0; i < nRx; i++) {
        printf("%c", *(serverResponse + i) );                          /* print last stream of header up to 2*<CR><LF> */
        if ( *(serverResponse + i) == '\r' ) {                         /* check if this character is a CR */
            if ( (*(serverResponse + (i + 2))) == '\r') {              /* if this character advanced by 2 is also a CR, we have found the end of header "marker" */
                nRxleftover = ( nRx - (i+4) );                         /* number of bytes to give to fwrite to print the start of the file */
                iSave = i+4;                                           /* note location of start of file -> i advanced by 4 for \r\n\r\n */
                i = nRx;                                               /* exit loop */
            }
        }
    }
    fpout = fopen(fileOut, WRITE_BIN_MODE);     /* open file, for writing in binary mode */
    if (fpout == NULL) {                        /* error check */
        printf("\nprocessServerResponse: Cannot open output file");
        return 1;
    }

    if (iSave < nRx)  {                          /* only attempt write start of file if it was actually received in the previous serverResponse stream */
        retVal = (int) fwrite( (serverResponse+iSave), sizeof(char), nRxleftover, fpout);
        if ( retVal != nRxleftover ) {
            printf("\nprocessServerResponse: %d bytes actually written of %d bytes requested", retVal, nRxleftover);
            return 1;
        }
    }
    stop = 1;           /* reset stop value */

    /* receive rest of file from server */
    do {
        nRx = recv(clientSocket, serverResponse, LEN_RESPONSE_STREAM, 0);                  /* return number of bytes received */
        if (nRx == -1) {
            printf("\nprocessServerResponse: error %d.", nRx);
            return 1;
        }
        else if (nRx == 0) {
            printf("\n\nprocessServerResponse: server has closed connection.");
            stop = 0;
        }
        else {
            retVal = (int) fwrite(serverResponse, sizeof(char), nRx, fpout);        /* write serverResponse stream to file */
            if ( retVal != nRx ) {
                printf("\nprocessServerResponse: %d bytes actually written of %d bytes requested", retVal, nRx);
                return 1;
            }
        }
    }
    while (stop != 0);

    free(serverResponse);

    if ((fclose(fpout)) == EOF) {          /* close file */
        printf("\nprocessServerResponse: Cannot close output file.");
        return 1;
    }
    return 0;
}

/* Function to print informative error messages
   when something goes wrong...  */
void printError(void)
{
	char lastError[1024];
	int errCode;

	errCode = WSAGetLastError();  /* get the error code for the last error */
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lastError,
		1024,
		NULL);  /* convert error code to error message */
	    printf("WSA Error Code %d = %s\n", errCode, lastError);
}

