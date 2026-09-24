#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* UDP: socket() -> bind() -> recvfrom -> (sendTo) -> close */


int main(int argc, char *argv[]) {
        int SERVER_PORT = 9000;

/* Direccion del servidor **/
        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;

        server_address.sin_port = htons(SERVER_PORT);

        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

/**
struct sockaddr_in
{
    short          sin_family;  //familia de protocolos. AF_INET para IPv4
    u_short        sin_port;    //puerto asociado al socket
    struct in_addr sin_addr;    //struct in_addr { u_long s_addr;} s_addr dirección IP del socket
    char           sin_zero[8];
};
**/

/* Creacion del socket */

/*
Es un fichero
*/

/**
int socket(int family, int type, int protocol);
@param family: AF_INET (IPv4), AF_INET6 (IPv6).
@param type: SOCK_DGRAM (UDP), SOCK_STREAM (TCP), SOCK_RAW
@param protocol: Tipicamente 0 (no usado en sockets de Internet).
**/
        int listen_sock;
        if ((listen_sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
                printf("Error en socket\n");
                return 1;
        } else {
                printf("Socket created successfully\n");
        }

/* Bind */

/**
Asocia un socket a una direccion especificada por addr. 
Normalmente, es necesario asignar una direccion local via 
esta funcion antes de que un socket TCP/UDP pueda recibir conexiones.
**/

/**
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
@param sockfd: descriptor de socket (devuelto por socket)
@param addr: direccion a asociar. Estructura sockaddr_in.
@param addrlen: longitud (en bytes) de la anterior estructura sockaddr_in.

**/

        if (bind(listen_sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
                printf("Error en bind\n");
                return 1;
        } else {
                printf("bind successfully\n");
        }

/* Receive */
/*
Recibe mensajes desde un socket, tanto en sockets orientados como no orientados a conexion
*/
/*
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
@param sockfd: descriptor del socket.
@param *buf: buffer de recepcion. Donde se almacena el mensaje.
@param len: numero de bytes a recibir.
@param flags: tipicamente 0.
@param struct sockaddr *src_addr: direccion del extremo remoto del socket (origen de la comunicacion).
                                  Me guardara la informacion del cliente.
@param socklen_t *addrlen: tamano de la estrutura src_addr.

@return ssize_t: Si tiene exito, devuelve el numero de bytes recibidos. Devuelve -1 si se produce un error.
*/
        struct sockaddr_in client_address;
        int client_address_len = 0;

        while (true) {
                int n = 0;
                int len = 0, maxlen = 100;
                char buffer[maxlen];
                char *pbuffer = buffer;
		int client_addrlen = sizeof(client_address);

                while ((n = recvfrom(listen_sock, pbuffer, maxlen, 0, (struct sockaddr*)&client_address, &client_addrlen)) > 0) {
                        pbuffer += n;
                        maxlen -= n;
                        len += n;
                        printf("Recibido: '%s'\n", buffer);
                        printf("Ip del cliente %u\n", client_address.sin_addr.s_addr);
                        /* SendTo */
                        /*
                        En un socket en estado conectado (con receptor conocido) transmite mensajes a un socket remoto.
                        */
                        /*
                        ssize_t send(int sockfd, const void *buf, size_t len, int flags);
                        @param sockfd: descriptor del socket.
                        @param *buf: buffer de envio donde se almacena el mensaje a enviar.
                        @param len: numero de bytes a enviar

                        @return ssize_t: numero de bytes enviados. -1 si da error.
                        */

                        int bytesSent = sendto(listen_sock, buffer, len, 0, (struct sockaddr*)&client_address, client_address_len);
                        printf("Number of bytes sent %d", bytesSent);
                }

                close(listen_sock);
        }

        close(listen_sock);
        return 0;
}