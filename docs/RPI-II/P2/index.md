# Práctica 2. Sockets TCP y UDP en ESP-IDF

## Objetivos

* Familiarizarse con la API de *sockets* en C.
* Desarrollar esquemas básicos de sistemas cliente/servidor TCP y UDP
  utilizando C.
* Ser capaces de analizar el tráfico generado en una conexión TCP y UDP a 
  través de Wireshark.
* Diseñar un protocolo de capa de aplicación para simular una aplicación
  cliente/servidor utilizando TCP y UDP para interactuar entre un host y 
  la placa ESP32

## Introducción

En la práctica anterior, vimos cómo desarrollar sistemas cliente/servidor
sencillos utilizando Python, tanto en sus variantes TCP como UDP.
El objetivo de la presente práctica es estudiar y desarrollar componentes
de red (clientes y servidores TCP y UDP) que puedan ejecutarse sobre el 
ESP32 haciendo uso de las facilidades propuestas por ESP-IDF. Además, 
demostraremos que es posible hacer interactuar clientes y servidores ejecutándose
indistintamente en la máquina virtual (programados vía Python) y en la propia
placa (utilizando la API de sockets de C).

## La API de sockets en C

### Funciones para ordenacion de bytes

Como TCP/IP es un estándar universal, y permite comunicaciones entre cualquier
plataforma y arquitectura, es necesario disponer de un método de ordenación
de los bytes para que máquinas *big-endian* y *little-endian* puedan comunicarse
de forma transparente y correcta. Para ello, se suelen proporcionar rutinas
de reordenación de bytes. En plataformas donde los datos ya están correctamente
ordenados, estas funciones no tienen ninguna funcionalidad especial, pero en
cualquier caso, es necesario utilizarlas siempre para que la comunicación
entre pares sea correcta. Las funciones típicas de reordenación de datos
son cuatro: `htons`, `htonl`, `ntohs` y `ntohl`. Su nombre explica correctamente
su semántica: *host to network (short)*
*host to network (long)*, *network to host (short)* y *network to host (long)*,
convirtiendo tipos de datos *short* y *long* desde el formato utilizado en
transmisiones de red (*network*) a representación en el *host*. Así, siempre
que enviemos un dato binario por la red, deberá ser transformado utilizando 
`hton*` y cuando lo recibamos y debamos procesarlo, utilizando `ntoh*`.

### Estructuras de datos

Antes de estudiar la API de *sockets* básica, es necesario mostrar el cometido
de un conjunto de estructuras de datos utilizadas comunmente en todas ellas. La
más importante es `sockaddr_in`, que se define como sigue:

```c
struct sockaddr_in
{
    short          sin_family;
    u_short        sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};
```

La estructura `in_addr` utilizada en `sockaddr_in` se define como:

```c
struct in_addr
{
    u_long s_addr;
};
```

Ésta consiste en un campo de tipo `unsigned long int` que contiene la 
dirección IP que se asociará con el socket. 

La estructura `sockaddr_in` contiene dos campos importantes: 

* `sin_family`: que indica al socket qué familia de protocolos se utiliarán 
  (usaremos la constante `AF_INET` para IPv4).
* `sin_port`: que indica el puerto asociado al socket.

### API básica


#### `socket()`

* Prototipo:

```c
int socket(int family, int type, int protocol);
```

* Descripción: Crea un *endpoint* de comunicación y devuelve un descriptor 
  de fichero para manejarlo.

* Parámetros: 
    - `family`:  `AF_INET` (IPv4), `AF_INET6` (IPv6).
    - `type`: `SOCK_DGRAM` (UDP), `SOCK_STREAM` (TCP), `SOCK_RAW`.
    - `protocol`: Típicamente 0 (no usado en sockets de Internet).

* Valor de retorno: Si tiene éxito, devuelve el descriptor de *socket*.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `socket` (`man socket`).

#### `bind()`

* Prototipo:

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

* Descripción: Asocia un *socket* a una dirección especificada por `addr`.
  Normalmente, es necesario asignar una dirección local vía esta función
  antes de que un socket TCP pueda recibir conexiones.

* Parámetros: 
    - `sockfd`:  descriptor de *socket* (devuelto por `socket`).
    - `addr`: dirección a asociar (véase estructura en sección anterior).
    - `addrlen`: longitud (en bytes) de la anterior estructura.

* Valor de retorno: Si tiene éxito, devuelve 0.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `bind` (`man bind`).


#### `listen()`

* Prototipo:

```c
int listen(int sockfd, int backlog);
```

* Descripción: Marca el *socket* proporcionado como *pasivo*, esto es, 
  un *socket* que podrá ser utilizado para aceptar conexiones entrantes
  usando la llamada `accept`.

* Parámetros: 
    - `sockfd`:  descriptor de *socket* (devuelto por `socket`).
    - `backlog`: longitud máxima que podrá tener la cola de conexiones pendientes
    para el *socket*. Si se sobrepasa, el cliente recibirá un error en su
    intento de conexión.

* Valor de retorno: Si tiene éxito, devuelve 0.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `listen` (`man listen`).

#### `accept()`

* Prototipo:

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

* Descripción: En *sockets* orientados a conexión, extrae la primera solicitud
  de conexión de la cola de conexiones pendientes para el *socket* proporcionado,
  crea un nuevo *socket conectado* y devuelve su descriptor. 

* Parámetros: 
    - `sockfd`:  descriptor de *socket* (devuelto por `socket`).
    - `addr`:  es un puntero a una estructura de tipo `sockaddr`, cuyos campos
      serán rellenados con los datos de dirección del socket remoto.
    - `addrlen`: tamaño de la estructura `addr`.

* Valor de retorno: Si tiene éxito, devuelve el descriptor de socket.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `accept` (`man accept`).

#### `connect()`

* Prototipo:

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

* Descripción: Conecta el *socket* proporcionada a la dirección específicada por
  `addr`. Si el *socket* es UDP, `addr` será la dirección a la que se enviarán
  los datagramas por defecto, y la única desde la que se recibirán datagramas.
  En caso de TCP, esta llamada inicia el proceso de conexión a la dirección
  especificada.

* Parámetros: 
    - `sockfd`:  descriptor de *socket* (devuelto por `socket`).
    - `addr`:  es un puntero a una estructura de tipo `sockaddr`, cuyos campos
      indican la dirección de conexión destino.
    - `addrlen`: tamaño de la estructura `addr`.

* Valor de retorno: Si tiene éxito, devuelve el descriptor de socket.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `connect` (`man connect`).


#### `send()`

* Prototipo:

```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

* Descripción: En un *socket* en estado *conectado* (con receptor conocido)
  transmite mensajes a un socket remoto.

* Parámetros: 
    - `sockfd`:  descriptor de *socket* de envío.
    - `buf`:  *buffer* de envío donde se almacena el mensaje a enviar.
    - `len`: número de bytes a enviar.

* Valor de retorno: Si tiene éxito, devuelve el número de bytes enviados.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad la página de manual de `send` (`man send`).


#### `recv()`/`recvfrom()`

* Prototipos:

```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

```

* Descripción: Reciben mensajes desde un  *socket*, tanto en sockets orientados
  como no orientados a conexión. `recvfrom`, a diferencia de `recv`, recibe
  parámetrosd de salida adicionales que almacenan información sobre la dirección
  origen del mensaje.

* Parámetros: 
    - `sockfd`:  descriptor de *socket* de recepción.
    - `buf`:  *buffer* de recepción donde se almacena el mensaje a recibir.
    - `len`: número de bytes a recibir.
    - `src_addr`: dirección del extremo remoto del socket (origen de la comunicación).
    - `addrlen`: tamaño de la estructura `src_addr`.

* Valor de retorno: Si tiene éxito, devuelve el número de bytes recibidos.
                    Devuelve `-1` si se produce un error.

* Detalles: consultad las páginas de manual de `recv` y `recv_from` (`man recv` y 
  `man recv_from`).


#### `close()`

* Prototipo:

```c
int close(int fd);
```

* Descripción: Cierra un socket.

* Parámetros: 
    - `fd`:  descriptor de *socket*.

* Detalles: consultad la página de manual de `close` (`man close`).


## Ejemplos

Se proporcionan a continuación ejemplos completos de uso de la API de *sockets*
en C para el desarrollo de sistemas cliente/servidor sencillos. Para cada 
uno de ellos, comprueba que, efectivamente, el uso y secuencia de aplicación
de cada llamada sigue las directivas de la figura:

![flow](img/flow.png)

!!! note "Tarea"
    Compila (`gcc ejemplo.c -o ejemplo.x`) y ejecuta (`./ejemplo.x`) cada par
    de códigos y comprueba su funcionamiento. Estudia con detenimiento el uso
    de cada rutina y como efectivamente siguen las directivas marcadas 
    anteriormente.

### Ejemplo: un cliente TCP

```c
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
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                printf("Error en socket\n");
                return 1;
        }

        if (connect(sock, (struct sockaddr*)&server_address,
                    sizeof(server_address)) < 0) {
                printf("Error en connect\n");
                return 1;
        }

        const char* data_to_send = "Hola, RPI!!";
        send(sock, data_to_send, strlen(data_to_send), 0);

        int n = 0;
        int len = 0, maxlen = 100;
        char buffer[maxlen];
        char* pbuffer = buffer;

        while ((n = recv(sock, pbuffer, maxlen, 0)) > 0) {
                pbuffer += n;
                maxlen -= n;
                len += n;

                buffer[len] = '\0';
                printf("Recibido: '%s'\n", buffer);
        }

        close(sock);
        return 0;
}
```

### Ejemplo: un servidor TCP

```c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
        int SERVER_PORT = 9000;

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;

        server_address.sin_port = htons(SERVER_PORT);

        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        int listen_sock;
        if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                printf("Error en socket\n");
                return 1;
        }

        if ((bind(listen_sock, (struct sockaddr *)&server_address,
                  sizeof(server_address))) < 0) {
                printf("Error en bind\n");
                return 1;
        }

        int wait_size = 16;  

        if (listen(listen_sock, wait_size) < 0) {
                printf("Error en listen\n");
                return 1;
        }

        struct sockaddr_in client_address;
        int client_address_len = 0;

        while (true) {
                int sock;
                if ((sock =
                         accept(listen_sock, (struct sockaddr *)&client_address,
                                &client_address_len)) < 0) {
                        printf("Error en accept\n");
                        return 1;
                }

                int n = 0;
                int len = 0, maxlen = 100;
                char buffer[maxlen];
                char *pbuffer = buffer;

                printf("Cliente conectado con IP: %s\n",
                       inet_ntoa(client_address.sin_addr));

                while ((n = recv(sock, pbuffer, maxlen, 0)) > 0) {
                        pbuffer += n;
                        maxlen -= n;
                        len += n;

                        printf("Recibido: '%s'\n", buffer);

                        send(sock, buffer, len, 0);
                }

                close(sock);
        }

        close(listen_sock);
        return 0;
}
```

!!! note "Tarea"
    Reproduce el funcionamiento del anterior sistema cliente/servidor *echo*
    utilizando UDP.

### Construcción de mensajes

Para enviar mensajes que encapsulen distintos tipos de datos en una sola
invocación, puedes definir un mensaje como el siguiente:

```c
typedef struct {
  int x;
  int y;
} mensaje;
```

Dando valor a cada campo y a continuación enviándolo proporcionando la dirección
de inicio de la estructura, del siguiente modo:

```c
mensaje.x = x; mensaje.y = y;
send( socketfd, &mensaje, sizeof( mensaje ), 0 );
```

!!! note "Tarea"
    Modifica el cliente UDP para que encapsule y envíe una estructura con 
    distintos campos (por ejemplo, dos enteros),
    que sea recibida por un servidor Python siguiendo las
    directivas de la anterior práctica. En este caso, no utilices campos
    de tipo flotante (veremos cómo hacerlo más adelante). El objetivo del
    ejercicio es simplemente comprobar que la comunicación entre un cliente
    programado en C y un servidor programado en Python es posible. No se 
    pretende que desarrolles un sistema complejo.

## Sistemas cliente/servidor en el ESP32

La razón por la que hemos ejercitado el uso de la API de sockets desde C
en Linux es que la implementación de la pila TCP/IP en ESP-IDF 
(llamada [Lightweight TCP/IP (lwIP)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/lwip.html)) implementa al 100% dicha API. Por tanto, tanto
la estructura básica de un *firmware* que implemente un cliente o servidor
como la API utilizada permanece inalterada. 

En esta última sección, se pide trabajar con dos ejemplos básicos de implementación
de sistemas cliente/servidor UDP y TCP sobre el ESP32, con el objetivo de estudiar
su funcionalidad, comprobar su interoperabilidad y realizar modificaciones para
adaptarlas a una hipotética aplicación IoT.

## Cliente/servidor UDP en el ESP32

En esta parte, trabajarás con dos ejemplos proporcionados dentro de la colección
de ejemplos de ESP-IDF. Por tanto, copia en tu espacio de trabajo (fuera del
árbol principal de ESP-IDF) los ejemplos:

* **Servidor UDP**: `examples/protocols/sockets/udp_server/`
* **Cliente UDP**: `examples/protocols/sockets/udp_client/`

### Estructura general

Observa sus códigos (`udp_server.c` para el servidor, y `udp_client.c` para el
cliente). Comprueba que, tanto la estructura básica de ambos componentes como
las invocaciones a la API de sockets concuerdan con las que vimos para el 
sistema *echo* programado en C. 

Acerca de la tarea principal (funcion `app_main`) observa que realiza 
una serie de llamadas a APIs de configuración de algunos subsistemas de
FreeRTOS, principalmente:

```c
// Inicializa la partición NVS (Non-volatile storage) por defecto. 
ESP_ERROR_CHECK(nvs_flash_init());
// Inicializa la infraestructura ESP-NETIF.
ESP_ERROR_CHECK(esp_netif_init());
// Crea un bucle de eventos por defecto.
ESP_ERROR_CHECK(esp_event_loop_create_default());

/* Esta función configura WiFi o Ethernet, tal y como seleccionemos via menuconfig.
*/
ESP_ERROR_CHECK(example_connect());

xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
```

* `example_connect()`, función que no forma parte de ESP-IDF, establece una 
conexión WiFi o Ethernet. La función es bloqueante, y retorna cuando se ha
conseguido establecer una conexión.

* Las características de la conexión WiFi (SSID y contraseña) se deben proporcionar
a través de `menuconfig`.

* El objetivo de ESP-NETIF es proporcionar una capa de abstracción por encima de 
  la pila TCP/IP, de modo que pueda migrarse la pila sin que los códigos del 
  usuario cambien. Puedes consultar su documentación en la 
  [página oficial](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html).

* Por último, se cerea una tarea que ejecutará la lógica del servidor (lo mismo
  ocurre en el cliente).

* Observa que, en todo el código, los mensajes de error se anotan utilizando
  la macro `ESP_LOGE` y los informativos con `ESP_LOGI`; intenta seguir este 
  convenio en tus códigos.

### Despliegue. Opción 1

En este caso, desplegarás el cliente en un ESP32 y el servidor en otro. Si
no dispones de dos ESP32, puedes trabajar con un compañero.

En cualquier caso, ambos ESP32 deben pertenecer a la misma red inalámbrica,
por lo que deberán conectarse a un mismo punto de acceso (el profesor te
proporcionará los datos, o simplemente puedes utilizar tu punto de acceso
doméstico). Configura los siguientes puntos de la infraestructura:

* Configura el SSID y contraseña del punto de acceso vía `menuconfig` antes
de compilar y flashear el código tanto en el cliente como en el servidor. 

* En el servidor, configura vía `menuconfig` el puerto sobre el que escuchará.

* Arranca primero el nodo servidor y apunta la IP proporcionada por el punto de 
 acceso; utilízala en el cliente para
configurar la IP destino de la comunicación. No olvides configurar también
el puerto destino de acuerdo al configurado en el servidor vía `menuconfig`.

Ên este punto, podrás arrancar el cliente y deberías estar comunicando dos
nodos ESP32 vía UDP.

### Despliegue. Opción 2

Si sólo dispones de un nodo, o si simplemente quieres probar otra forma de 
comunicación en la que uno de los equipos es un PC, puedes utilizar alguna
de las herramientas del sistema:

!!! danger "Nota"
    Ten en cuenta que portátil (es decir, máquina virtual) y ESP32 deben
    pertenecera la misma red. Para conseguirlo, para tu máquina virtual y añade
    una nueva interfaz de red de tipo *bridge* conectada a la interfaz Wifi
    física de tu PC. Así, tendrás una interfaz con IP en la misma red, otorgada
    directamente por tu punto de acceso.

* Para recibir un paquete UDP a través de un puerto (es decir, emular un
  servidor UDP):

```sh
nc -ul -p 3333
```

* Para enviar un paquete UDP a una IP/puerto remotos (es decir, emular un cliente):

```sh
nc -u IP_REMOTA 3333
```

En el directorio `scripts` dispones también de pequeños ejemplos de clientes y
servidores UDP Python que puedes también utilizar.

## Cliente/servidor TCP en el ESP32

El despliegue de cliente y servidor TCP es equivalente al UDP.

* Para recibir un paquete TCP a través de un puerto (es decir, emular un
  servidor TCP):

```sh
nc -l IP -p 3333
```

* Para enviar un paquete TCP a una IP/puerto remotos (es decir, emular un cliente):

```sh
nc IP 3333
```

En el directorio `scripts` dispones también de pequeños ejemplos de clientes y
servidores TCP Python que puedes también utilizar.

!!! note "Tarea"
    Experimenta con los ejemplos proporcionados en ESP-IDF (cliente/servidor
    TCP y UDP) y consigue ejecutar todos los elementos en la placa. Si sólo
    dispones de una placa, utiliza la máquina virtual como cliente/servidor
    para comprobar el correcto funcionamiento de cada código.

!!! danger "Tarea entregable"
    En este punto, deberías disponer de un conjunto de códigos que implementan
    sistemas cliente/servidor tanto en un host (utilizando Python y/o C) como en
    la placa ESP32 (utilizando C y ESP-IDF), y deberías haber comprobado su
    correcto funcionamiento.

    Específicamente, dispondrás de:

    * Sistema cliente/servidor desarrollado en la Práctica 1, escrito en Python
    e implementando un protocolo básico de aplicacion (tipo de mensaje) propuesto
    por ti.

    * Código básico en C para implementación de un servidor/cliente *echo* 
    programado en C, cuyos códigos se proporcionan en este boletín.

    * Códigos básicos en C/ESP-IDF para implementar servidores/clientes
    *echo* sobre el ESP32.

    Como tarea entregable, se pide que adaptes tu entrega de la Práctica 1 para
    que tanto cliente como servidor puedan funcionar en el host (bien usando
    tu implementación Python, o bien utilizando una nueva implementación en 
    C) o en el ESP32. Se entregarán los códigos y una breve memoria con capturas
    de tráfico que demuestren el correcto funcionamiento del sistema.
