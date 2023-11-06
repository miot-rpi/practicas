# Práctica 5. Seguridad (TLS y DTLS)

## Objetivos

* Observar el comportamiento de TLS y DTLS para el intercambio cifrado de mensajes.
* Conocer las diferencias básicas entre TLS y DTLS.
* Conocer una API básica de programación de sistemas cliente/servidor usando TLS y DTLS (WolfSSL).
* Conocer una API básica de programación de sistemas cliente/servidor en ESP-IDF (ESP-TLS).

## Contenido del paquete proporcionado y setup básico

El paquete proporcionado (puedes descargarlo [aquí](https://drive.google.com/drive/folders/1JJZDf151Qpa6ltNKFMMzIF7VRMVu0LA_?usp=share_link)) 
incluye ejemplos de sistemas cliente/servidor
programados utilizando TLS y DTLS mediante el uso de la biblioteca WolfSSL.
Estos códigos están diseñados para ejecutarse exclusivamente en un sistema
Linux, y requieren la instalación de los paquetes de desarrollo WolfSSL. 

Para realizar la instalación, ejecuta en tu máquina virtual (esta línea
podría cambiar si usas otra distribución de Linux, en cuyo caso deberás
buscar los paquetes equivalentes):

```sh
sudo apt-get install libwolfssl-dev libwolfssl*
```

Una vez hecho esto, prueba a realizar la compilación tanto del ejemplo
TLS como DTLS utilizando la orden `make` en el directorio correspondiente. Si
todo ha ido bien, puedes continuar con el análisis de los códigos en la 
siguiente sección.

## Cliente/servidor TLS. Ejemplo básico en host

### Servidor TLS

Analizaremos en primer lugar el código básico del servidor TLS. Para ello, 
observa el contenido del fchero `tls/server-tls.c`.

### Cabeceras y constantes

El uso de WolfSSL requiere la inclusión de dos cabeceras básicas:

```c
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
```

Además, ya que serán necesarios en el desarrollo, definiremos las rutas al
certificado (clave pública) del servidor y su clave privada:

```c
#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"
```

Observa además que el puerto de escucha del servidor será el `11111`.

### Objetos básicos WolfSSL. Contexto y objeto SSL

Definiremos dos objetos básicos que se utilizarán de forma recurrente
en el código:

```c
WOLFSSL_CTX* ctx;
WOLFSSL*     ssl;
```

El contexto (`ctx`) incluye valores globales para cada conexión SSL, incluyendo
información sobre certificados. Es posible utilizar un mismo contexto para 
múltiples conexiones, siempre que compartan características. Para crear un 
nuevo contexto, utilizaremos la función `wolfSSL_CTX_new` como sigue:

```c
/* Create and initialize WOLFSSL_CTX */
if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
  fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
  return -1;
}
```

El argumento proporcionado incluye información sobre la versión de protocolo
a utilizar. Actualmente, WolfSSL soporta SSL 3.0,
TLS 1.1, TLS 1.2,  DTLS 1.0 y DTLS 1.2. En este caso, para la parte cliente,
las funciones a utilizar como argumento serían:

* `wolfSSLv3_server_method();     // SSLv3`
* `wolfTLSv1_server_method();     // TLSv1`
* `wolfTLSv1_1_server_method();   // TLSv1.1`
* `wolfTLSv1_2_server_method();   // TLSv1.2`
* `wolfDTLSv1_server_method();    // DTLS`
* `wolfDTLSv1_2_server_method();  // DTLS 1.2`

En segundo lugar, es necesario cargar nuestra CA (Autoridad Certificadora)
en el contexto, para que cualquier cliente pueda verificar, en el momento de
su conexión, la identidad del sevidor. Para ello, usamos la función
`wolfSSL_CTX_use_certificate_file` de la siguiente manera:

```c
/* Load server certificates into WOLFSSL_CTX */
if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
  fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
  return -1;
}
```

Del mismo modo, el servidor deberá incluir su clave privada en formato PEM:

```c
/* Load server key into WOLFSSL_CTX */
if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
  fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
  return -1;
}
```

A continuación, observa como el código de escucha y aceptación de conexiones
entrantes no difier de cualquier código que hayas desarrollado previamente
para aceptar conexiones entrantes TCP (`bind`, + `listen` + `accept`).

Justo tras la conexión (`accept`), resulta necesario crear un nuevo objeto
SSL, así como asociar el descriptor de socket con la nueva sesión (conexión)
TLS:

```c
/* Create a WOLFSSL object */
if ((ssl = wolfSSL_new(ctx)) == NULL) {
  fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
  return -1;
}

/* Attach wolfSSL to the socket */
wolfSSL_set_fd(ssl, connd);

/* Establish TLS connection */
ret = wolfSSL_accept(ssl);
if (ret != SSL_SUCCESS) {
  fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
  return -1;
}
```

A partir de este punto, podemos enviar y recibir datos a través del socket
(y por tanto de la conexión TLS) de forma muy similar a como lo hacemos 
con el enfoque clásico. Así, para recibir datos:

```c
if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
  fprintf(stderr, "ERROR: failed to read\n");
  return -1;
}
```

Y para enviar datos de vuelta:

```c
/* Reply back to the client */
if (wolfSSL_write(ssl, buff, len) != len) {
  fprintf(stderr, "ERROR: failed to write\n");
  return -1;
}
```

Por último, finalizaremos la conexión con la invocación de la función
`wolfSSL_free(ssl)`.


!!! note Tarea 
    El cliente proporcionado sigue una estrategia de implementación similar.
    Compara ambos códigos (cliente y servidor) y asegúrate de entender las
    diferencias entre ellos.

!!! danger "Tarea entregable"
    Compila y ejecuta el sistema cliente/servidor TLS y obtén capturas de
    tráfico tanto de las fases de establecimiento de conexión como de las
    fases de transferencia de datos. En base a lo aprendido en las clases
    de teoría y la documentación adicional sobre TLS y wolfSSL, redacta 
    un breve informe que resuma el proceso de *handshake* y transferencia
    de datos en TLS tomando como base los paquetes reales observados para
    esta conexión.

## Cliente/servidor DTLS. Ejemplo básico en host

El desarrollo de un sistema básico cliente/servidor con soporte DTLS utilizando
WolfSSL es muy similar al visto anteriormente para TLS. Como detalle adicional,
la inicialización de la infraestructura debe realizarse indicando el soporte
para DTLS en sus versiones 1.0 o 1.2:


* `wolfDTLSv1_client_method();    // DTLS 1.0`

* `wolfDTLSv1_2_client_method();  // DTLS 1.2`

El resto del código sigue una filosofía similar a TLS, adaptado, obviamente,
a las características de UDP (tipo de socket, ausencia de conexión, etc.), por
lo que se deja como ejercicio su análisis y ejecución.

!!! danger "Tarea entregable"
    Analiza, compila y ejecuta los códigos correspondientes al sistema cliente/servidor
    DTLS. Realiza capturas de tráfico y compáralas, paquete a paquete, con las generadas
    para un patrón de tráfico similar en el caso de TLS. Incide en sus similitudes
    y diferencias, tanto a nivel de *handshake* como de transferencia de datos. Observa,
    en este último caso, la aparición de nuevos campos de encabezado en los envíos
    de datos DTLS. ¿Cuál/cuáles son esos campos y por qué aparecen? Realiza una comparativa
    del tráfico total generado en ambos casos para exactamente la misma cantidad de datos 
    transferidos.

## TLS en el ESP32. El componente ESP-TLS

ESP-IDF proporciona un componente (ESP-TLS) que ofrece una interfaz (API)
simplificada para acceder a funcionalidad básica TLS. Aún así, ofrece 
una funcionalidad suficientemente amplia como para implementar casos de uso
comunes en entornos IoT. 

La API de ESP-TLS es sencilla, y se basa en el uso de cuatro funciones básicas:

### Establecimiento de conexión TLS (`esp_tls_conn_new()`)

* Prototipo:

```c
esp_tls_t *esp_tls_conn_new(const char *hostname, int hostlen, int port, constesp_tls_cfg_t *cfg)
```

* Descripción: Crea una nueva conexión TLS/SSL bloqueante, estableciendo dicha
  conexión contra un servidor establecido. 

* Parámetros: 
    - `hostname`: Identificación del host.
    - `hostlen`: Longitud del parámetro `hostname`.
    - `port`: Puerto de conexión con el host.
    - `cfg`: Configuración de la conexión TLS.

* Valor de retorno: Puntero a `esp_tls_t` (manejador de la conexión).
                    Devuelve `NULL` si se produce un error en la conexión.

### Destrucción de conexión TLS (`esp_tls_conn_delete()`)

```c
void esp_tls_conn_delete(esp_tls_t *tls)
```

* Descripción: Cierra la conexión TLS/SSL. 

* Parámetros: 
    - `tls`: Manejador de la conexión.

### Escritura de datos (`esp_tls_conn_read()`)

```c
static ssize_t esp_tls_conn_write(esp_tls_t *tls, const void *data, size_t datalen)
```

* Descripción: Escribe en la conexión TLS/SSL indicada el contenido del buffer
  `data`.

* Parámetros: 
    - `tls`: Manejador de la conexión. 
    - `data`: Buffer de envío.
    - `datalen`: Longitud del buffer de envío (o número máximo 
       de bytes a escribir).

* Valor de retorno: 
    - `>=0`: éxito en el envío. Número de bytes efectivamente enviados.
    - `<0`: error en el envío.

### Lectura de datos (`esp_tls_conn_read()`)

```c
static ssize_t esp_tls_conn_read(esp_tls_t *tls, void *data, size_t datalen)
```

* Descripción: Lee desde la conexión TLS/SSL indicada hacia el buffer `data`.

* Parámetros: 
    - `tls`: Manejador de la conexión. 
    - `data`: Buffer de recepción.
    - `datalen`: Longitud del buffer de recepción (o número máximo 
       de bytes a leer).

* Valor de retorno: 
    - `>0`: éxito en la recepción. Número de bytes efectivamente leídos.
    - `=0`: error en la recepción. La conexión se cerró.
    - `<0`: error en la recepción. 

### Estructura básica de un cliente TCP usando ESP-IDF

Un cliente TCP implementado sobre ESP-IDF para dar soporte TLS, 
requiere ciertas modificaciones con respecto a la versión sin TLS. 
De hecho, el uso de ESP-IDF simplifica el código del cliente. La
estructura básica resultaría:

```c
/// Includes anteriores.
#include "esp_tls.h"

// Puede tomarse desde menuconfig.
#define HOST_IP_ADDR DIRECCION_DE_HOST
#define PORT PUERTO 

static const char *payload = "Hola, mundo via TLS";

// ...

static void tls_client_task( void  *pvParameters )
{
  // ...

  // Configuración de ESP-TLS (vacío para opciones defecto).
  esp_tls_cfg_t cfg = { };

  // Creación de conexión.
  struct esp_tls *tls = esp_tls_conn_new( HOST_IP_ADDR, longitud, PORT, &cfg);

  // Chequeo de errores.
  // ...

  // Envío de datos.
  ret = esp_tls_conn_write(tls, payload, strlen(payload));

  // Chequeo de errores.
  // ...

  // Lectura de datos
  ret = esp_tls_conn_read(tls, (char *)rx_buffer, 128);

  // Chequeo de errores.
  // ...

  // Destrucción de la conexión
  esp_tls_conn_delete( tls );

  vTaskDelete( NULL );
}

void app_main( void )
{
  // ...
}
```

!!! note Tarea
    Compila y ejecuta el cliente TLS en el ESP32, y consigue que interactúe con
    el servidor TLS que probaste anteriormente en el *host*. Comprueba que, efectivamente,
    los datos se transfieren cifrados entre ambos extremos, y que el intercambio de 
    paquetes es similar al que observaste entre cliente y servidor en el *host*.
