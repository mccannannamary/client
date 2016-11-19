SOCKET createSocket(void);

int connectToServer(SOCKET, char[], int, char *host, char *path, char fileOut[]);

int processServerResponse(SOCKET, char fileOut[]);

void printError(void);


