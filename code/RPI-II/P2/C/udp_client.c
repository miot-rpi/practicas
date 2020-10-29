#include <netinet/ip.h>
#include <unistd.h>

#define TM_BUF_SIZE 1400
#define TM_PACKETS_TO_SEND 10
#define TM_DEST_ADDR "127.0.0.1"
#define TM_DEST_PORT 9999
 
char testBuffer[TM_BUF_SIZE];
char * errorStr;
 
int udpClient(void)
{
    int testSocket;
    unsigned int counter;
    struct sockaddr_in destAddr;
    int errorCode;
    int returnVal;
 
    counter = 0;
    returnVal = 0;
 
/* Specify the address family */
    destAddr.sin_family = AF_INET;
/* Specify the destination port */
    destAddr.sin_port = htons(TM_DEST_PORT);
/* Specify the destination IP address */
    destAddr.sin_addr.s_addr = inet_addr(TM_DEST_ADDR);
 
/* Create a socket */
    testSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 
/* While we havenât yet sent enough packets... */
    while (counter < TM_PACKETS_TO_SEND)
    {
/* Send another packet to the destination specified above */
        errorCode = sendto(testSocket,
                           testBuffer,
                           TM_BUF_SIZE,
                           0,
                           &destAddr,
                           sizeof(destAddr));
 
/*
 * Check if there was an error while sending. If so, break from the
 * loop
 */
/* Increment the number of packets sent by 1 */
        counter++;
    }
 
udpClientEnd:
/* Make sure the socket exists before we close it */
    if (testSocket != -1)
    {
/* Close the socket */
        close(testSocket);
    }
 
    return(returnVal);
}

int main( void ) {

    udpClient();
    
}
