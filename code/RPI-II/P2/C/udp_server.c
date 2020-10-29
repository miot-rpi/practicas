#include <netinet/ip.h>
#include <unistd.h>

#define TM_BUF_SIZE 1500
#define TM_DEST_PORT 9999
 
char testBuffer[TM_BUF_SIZE];
char * errorStr;
 
int udpServer(void)
{
    int testSocket;
    struct sockaddr_in sourceAddr;
    struct sockaddr_in destAddr;
    int errorCode;
    int addrLen;
    int returnVal;
 
    returnVal = 0;
 
/* Specify the address family */
    destAddr.sin_family = AF_INET;
/*
* Specify the dest port (this being the server, the destination
* port is the one weâll bind to)
*/
    destAddr.sin_port = htons(TM_DEST_PORT);
/*
* Specify the destination IP address (our IP address). Setting
* this value to 0 tells the stack that we donât care what IP
* address we use - it should pick one. For systems with one IP
* address, this is the easiest approach.
*/
    destAddr.sin_addr.s_addr = 0;
 
/*
* The third value is the specific protocol we wish to use. We pass
* in a 0 because the stack is capable of figuring out which
* protocol to used based on the second parameter (SOCK_DGRAM =
* UDP, SOCK_STREAM = TCP)
*/
    testSocket = socket(AF_INET, SOCK_DGRAM, 0);
/* Make sure the socket was created successfully */
 
/*
* Bind the socket to the port and address at which we wish to
* receive data
*/
    errorCode = bind(testSocket, &destAddr, sizeof(destAddr));
 
/* Do this forever... */
    while (1)
    {
/* Get the size of the sockaddr_in structure */
        addrLen = sizeof(sourceAddr);
/*
* Receive data. The values passed in are:
* We receive said data on testSocket.
* The data is stored in testBuffer.
* We can receive up to TM_BUF_SIZE bytes.
* There are no flags we care to set.
* Store the IP address/port the data came from in sourceAddr
* Store the length of the data stored in sourceAddr in addrLen.
*The length that addrLen is set to when itâs passed in is
*used to make sure the stack doesnât write more bytes to
*sourceAddr than it should.
*/
        errorCode = recvfrom(testSocket,
                             testBuffer,
                             TM_BUF_SIZE,
                             0,
                             &sourceAddr,
                             &addrLen);
        }
 
udpServerEnd:
/* Make sure we have an actual socket before we try to close it */
    if (testSocket != -1)
    {
/* Close the socket */
        close(testSocket);
    }
 
    return(returnVal);
}

int main( void ) {

    udpServer();
    
}
