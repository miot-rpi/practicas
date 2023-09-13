#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>


int main() {
        const int server_port = 9000;

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;

        server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_address.sin_port = htons(server_port);

        int sock;
        if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
                printf("Error en socket\n");
                return 1;
        } else {
                printf("Socket created succesfully\n");
        }

        const char* data_to_send = "RPI II";
        int numberOfBytesSenT = sendto(sock, data_to_send, strlen(data_to_send), 0, (struct sockaddr*)&server_address, sizeof(server_address));
        printf("SendTo number of bytes is %d\n", numberOfBytesSenT);

        int n = 0;
        int len = 0, maxlen = 100;
        char buffer[maxlen];
        char* pbuffer = buffer;
	int mSize = sizeof(server_address);

        while((n = recvfrom(sock, pbuffer, maxlen, 0, (struct sockaddr*)&server_address, (socklen_t*) &mSize)) > 0){
                pbuffer += n;
                maxlen -= n;
                len += n;

                buffer[len] = '\0';
                printf("Recibido: '%s'\n", buffer);
        }

        close(sock);
        return 0;
}
