# Práctica 2. Seguridad (TLS y DTLS)

## Objetivos

* Observar el comportamiento de TLS y DTLS para el intercambio cifrado de mensajes.
* Conocer las diferencias básicas entre TLS y DTLS.
* Conocer una API básica de programación de sistemas cliente/servidor usando TLS y DTLS (WolfSSL).
* Conocer una API básica de programación de sistemas cliente/servidor en ESP-IDF (ESP-TLS/WolfSSL).

## WolfSSL

De las principales librerías que implementan protocolos de seguridad, sin duda alguna la más eficiente es [WolfSSL](https://www.wolfssl.com/) y es por ello que esta práctica se centra en ella.

### Instalación (Linux)

Aunque se pueden emplear paquetes binarios de la propia distribución de Linux, es recomendable partir del código fuente y compilarlo/instalarlo.

Por lo tanto, lo primero es descargar el fichero `wolfssl-5.x.x.zip` de este [enlace](https://www.wolfssl.com/download/) y descomprimirlo en una ruta sin espacios *(esta recomendación es aplicable a todas las prácticas)*.

Antes de proceder a su compilación es necesario instalar una serie de pre-requisitos:

```sh
sudo apt install build-essential libtool-bin autoconf libevent-dev
```
Moverse al directorio descomprimido para configurar y construir la librería:

```sh
./configure --enable-dtls  --enable-dtls13 
make
sudo make install
```

Dependiendo de la versión del fichero comprimido que se emplee es 
posible que el fichero `./configure` no este presente y sea preciso 
generarlo mediante el script `./autogen.sh`.

Cabe destacar el siguiente mensaje que aparece tras realizar la 
instalación mediante `make install`:

```
----------------------------------------------------------------------
Libraries have been installed in:
   /usr/local/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the '-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the 'LD_RUN_PATH' environment variable
     during linking
   - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to '/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------
```

Esta información es preciso tenerla en cuenta a la hora compilar y ejecutar 
binarios que empleen esta librería. En particular si no estuviese ya
el directorio `/usr/local/lib` en las rutas de búsqueda establecidas por la 
variable de entorno `LD_LIBRARY_PATH` sería preciso añadirlo con el siguiente
comando:

```
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

Esto puede realizarse cada vez que sea preciso o de forma automática al inicio 
de sesión modificando el fichero `.bashrc`:

```
echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH" >> $HOME/.bashrc
```

## Cliente/servidor TLS. Ejemplo básico en host

El repositorio [`wolfssl-examples`](https://github.com/wolfSSL/wolfssl-examples.git) contiene aplicaciones de ejemplo, escritas en C, que demuestran cómo usar la biblioteca WolfSSL.

Aunque sólo vamos a usar un par de ellas, lo más sencillo es clonar el repositorio completo:

```sh
git clone https://github.com/wolfSSL/wolfssl-examples.git
cd wolfssl-examples/
```

### Servidor TLS

Analizaremos en primer lugar el código básico del servidor TLS. Para ello, 
observa el contenido del fichero `tls/server-tls.c`.

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
su conexión, la identidad del servidor. Para ello, usamos la función
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
entrantes no difieren de cualquier código que hayas desarrollado previamente
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
WolfSSL es muy similar al visto anteriormente para TLS. Observa el contenido del fichero `dtls/server-dtls.c`.

El código sigue una filosofía similar a TLS, adaptado, obviamente,
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

## TLS en el ESP32. El componente wolfSSL (ESP-IDF 5.5.x)

Este apartado utiliza **ESP-IDF 5.5.x** y el componente **wolfssl/wolfssl 5.8.2~1**. Comprueba el entorno activo con `idf.py --version`. Esta combinación debe verificarse con la revisión concreta instalada; no se presupone su compatibilidad con ESP-IDF 6.1.

La librería WolfSSL está disponible como [componente de Espressif](https://components.espressif.com/components/wolfssl/wolfssl/versions/5.8.2~1). A continuación veremos un ejemplo de uso de cliente TLS apto para conectarse el servidor anterior.

Aunque es posible usar el GUI de la Extensión de ESP-IDF de VSCode, por brevedad vamos a realizar los pasos desde línea de comandos en el terminal `ESP-IDF Terminal`.

Moverse a la carpeta dónde se quiera importar el ejemplo y ejecutar el siguiente comando.

```
idf.py create-project-from-example "wolfssl/wolfssl^5.8.2~1:wolfssl_client"
cd wolfssl_client
idf.py set-target esp32
```

Ejecuta la utilidad `menuconfig` :

```
idf.py menuconfig
```

Y configura los diferentes parámetros para el dispositivo objetivo, junto con la configuración local de WiFi:

* Host de destino: `CONFIG_WOLFSSL_TARGET_HOST` (la IPv4 LAN del host en el que se ejecute el servidor TLS)
* Puerto de destino: `CONFIG_WOLFSSL_TARGET_PORT` (por defecto 11111)
* SSID de WiFi de ejemplo: `CONFIG_EXAMPLE_WIFI_SSID`
* Contraseña de WiFi de ejemplo: `CONFIG_EXAMPLE_WIFI_PASSWORD`

Cabe señalar que los *makefiles* de este ejemplo no requieren la instalación de wolfSSL copiando archivos localmente.

Comprueba en `main/include/client-tls.h` que la dirección configurada se utiliza en `TLS_SMP_TARGET_HOST`. Si aparece una dirección fija, adapta el bloque de esta forma:

```c
/* See main/Kconfig.projbuild for default configuration settings */
#ifdef CONFIG_WOLFSSL_TARGET_HOST
    #define TLS_SMP_TARGET_HOST         CONFIG_WOLFSSL_TARGET_HOST
#else
    #define TLS_SMP_TARGET_HOST         "192.168.1.41"
#endif
```

A continuación se puede compilar y cargar el firmware para ver el ejemplo en acción.

```shell
idf.py build
idf.py flash
idf.py monitor
```

### Uso de un certificado propio

Para utilizar certificados propios, el servidor debe cargar su certificado y su clave privada, y el ESP32 debe confiar en la CA que lo firma. No se debe copiar la clave privada del servidor al ESP32 ni desactivar la verificación del certificado. Este procedimiento también se aplica a la tarea con ESP-IDF 6.1 del apartado siguiente.

En macOS/Linux, con OpenSSL 3, ejecuta en una carpeta de trabajo nueva:

```sh
mkdir certificados
cd certificados
openssl req -x509 -newkey rsa:2048 -nodes -sha256 -days 365 \
  -keyout ca-key.pem -out ca-cert.pem -subj "/CN=CA laboratorio RPI2" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign"
openssl req -newkey rsa:2048 -nodes -sha256 \
  -keyout server-key.pem -out server.csr -subj "/CN=tls-lab.local"
cat > server.ext <<'EXT'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:tls-lab.local
EXT
openssl x509 -req -in server.csr -CA ca-cert.pem -CAkey ca-key.pem \
  -CAcreateserial -out server-cert.pem -days 90 -sha256 -extfile server.ext
chmod 600 ca-key.pem server-key.pem
openssl verify -CAfile ca-cert.pem server-cert.pem
```

La última orden debe devolver `OK`. En macOS, si el OpenSSL del sistema no admite `-addext`, utiliza OpenSSL 3 (por ejemplo, instalado mediante Homebrew). En el servidor `tls/server-tls.c`, configura `CERT_FILE` y `KEY_FILE` con las rutas a `server-cert.pem` y `server-key.pem`, respectivamente. No confundas el certificado del servidor con el de la CA.

Para el cliente de **ESP-IDF 5.5.x**, copia `ca-cert.pem` a `wolfssl_client/main/ca-cert.pem` y añade `EMBED_TXTFILES "ca-cert.pem"` a la llamada **existente** a `idf_component_register(...)` de `main/CMakeLists.txt`. En `main/client-tls.c`, declara:

```c
extern const unsigned char ca_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const unsigned char ca_cert_pem_end[]   asm("_binary_ca_cert_pem_end");
```

Sustituye la carga de la CA de ejemplo (`wolfSSL_CTX_load_verify_buffer`, que utiliza `CTX_CA_CERT`) por:

```c
ret_i = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_pem_start,
    (long)(ca_cert_pem_end - ca_cert_pem_start), WOLFSSL_FILETYPE_PEM);
/* Si ret_i != WOLFSSL_SUCCESS, abortar la conexión y liberar los recursos. */
wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);
```

Asegúrate de que se ejecuta la rama de verificación (`doPeerCheck != 0`) y de que no se sobrescribe después con `WOLFSSL_VERIFY_NONE`. Para autenticación solo del servidor, elimina de esa rama las cargas del certificado y clave de cliente de ejemplo (`CTX_CLIENT_CERT` y `CTX_CLIENT_KEY`); no son necesarias si el servidor no exige autenticación mutua.

Tras crear el objeto `ssl`, antes del handshake, comprueba también la identidad esperada:

```c
ret_i = wolfSSL_check_domain_name(ssl, "tls-lab.local");
/* Si ret_i != WOLFSSL_SUCCESS, abortar la conexión y liberar los recursos. */
```

El socket puede conectarse a la IPv4 del host configurada en `menuconfig`; `tls-lab.local` es el nombre que se comprobará contra el SAN del certificado, por lo que no necesita resolver por DNS en esta variante. Mantén el reloj del ESP32 sincronizado y la comprobación de fechas activa. Recompila y vuelve a cargar el firmware después de cambiar el certificado embebido.

!!! danger "Tarea entregable"
    Compila y ejecuta el cliente TLS en el ESP32, y consigue que interactúe con
    el servidor TLS que probaste anteriormente en el *host*. Comprueba que, efectivamente,
    los datos se transfieren cifrados entre ambos extremos, y que el intercambio de 
    paquetes es similar al que observaste entre cliente y servidor en el *host*. 

!!! danger "Tarea entregable"
    Repite el mismo proceso para el cliente DTLS.

## TLS en el ESP32. El componente esp-wolfssl (ESP-IDF 6.1)

Como alternativa para ESP-IDF 6.1, utilizaremos el port [espressif/esp-wolfssl](https://github.com/espressif/esp-wolfssl), distinto del componente `wolfssl/wolfssl` anterior. Su ejemplo nativo consulta por HTTPS `https://api.github.com/zen`: lo dejaremos con ese destino para comprobar inicialmente la integración, la conexión WiFi y TLS.

!!! note "Compatibilidad"
    Esta ruta incorpora cambios para IDF 6.x, incluida la desactivación de la aceleración criptográfica ESP32 incompatible con ciertas API nuevas. No implica que cualquier revisión de IDF 6.1 esté validada: el CMake del port cita pruebas con IDF master `v6.2-dev`. Los pasos siguientes están contrastados con las fuentes, pero requieren comprobar compilación y ejecución con la versión concreta del laboratorio.

Abre un terminal con **ESP-IDF 6.1** activado y descarga el repositorio en una carpeta nueva. Se fija una revisión para que el ejemplo y sus rutas sean reproducibles:

```sh
idf.py --version
git clone https://github.com/espressif/esp-wolfssl.git
cd esp-wolfssl
git checkout 16e338a7bf817f58416d6a3f520ffa14ce91b6f2
git submodule update --init --recursive
cd examples/wolfssl_client
idf.py set-target esp32
idf.py menuconfig
```

En `Example Connection Configuration`, configura WiFi, SSID y contraseña (`CONFIG_EXAMPLE_WIFI_SSID` y `CONFIG_EXAMPLE_WIFI_PASSWORD`). En `Example Configuration`, conserva activada **Enable Server Certificate Verification** (`CONFIG_EXAMPLE_SERVER_CERT_VERIFY=y`). No es necesario seleccionar el backend ESP-TLS personalizado: este ejemplo usa la API nativa de wolfSSL. Mantén la estructura del repositorio; no copies el ejemplo aislado ni añadas además el componente `wolfssl/wolfssl` del registro.

El archivo del cliente es `examples/wolfssl_client/main/wolfssl_client_demo.c` dentro de `esp-wolfssl`. Conserva `WEB_SERVER`, `WEB_PORT`, `WEB_URL`, `REQUEST` y el certificado embebido originales para la prueba con GitHub. El ejemplo sincroniza el reloj mediante SNTP antes del handshake: necesita acceso a Internet, DNS y NTP.

Para probar **TLS 1.3**, activa la línea `#define WOLFSSL_TLS13` en `esp-wolfssl/port/user_settings.h` (en la revisión indicada está comentada). Así, el ejemplo selecciona `wolfTLSv1_3_client_method()`; sin ese cambio utiliza TLS 1.2. Tras un `wolfSSL_connect()` exitoso y antes de liberar `ssl`, añade:

```c
ESP_LOGI(TAG, "Protocolo: %s; cifrado: %s",
         wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl));
```

Para verificar también la identidad de GitHub, después de crear y comprobar `ssl`, antes de `wolfSSL_connect()`, añade:

```c
if (wolfSSL_check_domain_name(ssl, WEB_SERVER) != WOLFSSL_SUCCESS) {
    ESP_LOGE(TAG, "No se pudo configurar el nombre esperado");
    goto failed4;
}
```

Desde `examples/wolfssl_client`, compila y carga (sustituye el puerto serie):

```sh
idf.py build
idf.py -p /dev/PUERTO_SERIE flash monitor
```

Comprueba que se obtiene respuesta HTTP de GitHub y que se registra `TLSv1.3` con la suite negociada. Si falla la validación, revisa hora, vigencia y CA del servidor; no la desactives. Fuentes del ejemplo: [cliente](https://github.com/espressif/esp-wolfssl/blob/16e338a7bf817f58416d6a3f520ffa14ce91b6f2/examples/wolfssl_client/main/wolfssl_client_demo.c) y [configuración del port](https://github.com/espressif/esp-wolfssl/blob/16e338a7bf817f58416d6a3f520ffa14ce91b6f2/port/user_settings.h).

### Adaptación al servidor de la práctica y certificado propio

Para la conexión local, genera los certificados como se describe en «Uso de un certificado propio» y configura en el servidor sus rutas `CERT_FILE` y `KEY_FILE`. Copia **`ca-cert.pem`** a `esp-wolfssl/examples/wolfssl_client/main/server_root_cert.pem`, sustituyendo la CA de GitHub: este archivo ya está embebido por el CMake del ejemplo. Conserva `CONFIG_EXAMPLE_SERVER_CERT_VERIFY=y` y `WOLFSSL_VERIFY_PEER`.

En `wolfssl_client_demo.c`, cambia `WEB_SERVER` por la IPv4 LAN del host y `WEB_PORT` por `11111`. Sustituye el bloque `REQUEST` de HTTP por un mensaje sencillo (`#define REQUEST "Hola desde ESP32\n"`), acorde con el servidor de las primeras secciones. Si añadiste la comprobación de nombre para GitHub, cambia su segundo argumento por `"tls-lab.local"`, el SAN del certificado propio. Recompila y recarga el firmware; la CA privada del laboratorio no sirve para autenticar GitHub.

El servidor original utiliza TLS 1.2. Para exigir **TLS 1.3 en ambos extremos**, recompila la biblioteca wolfSSL del host con `--enable-tls13`, sustituye en `server-tls.c` el método por `wolfTLSv1_3_server_method()` y recompila el servidor. Arráncalo en macOS/Linux antes de conectar la placa; permite TCP/11111 en el cortafuegos y comprueba que escucha en la interfaz LAN. Usa una red WiFi sin aislamiento entre clientes.

!!! danger "Tarea entregable"
    Adapta el cliente para conectarlo con el servidor TLS desarrollado en las primeras secciones, ejecutado en macOS o Linux. Usa un certificado propio y conserva su verificación. Entrega los cambios, las versiones utilizadas y los registros que acrediten intercambio de mensajes y TLS 1.3. Captura el tráfico en la interfaz LAN del servidor con Wireshark (`tcp.port == 11111`; si es necesario, «Decode As… → TLS»). Comprueba en ServerHello que Supported Versions selecciona `0x0304` (TLS 1.3), aunque el campo legacy_version indique `0x0303`. Explica por qué los mensajes posteriores a ServerHello y los datos de aplicación no aparecen en claro. No entregues claves privadas ni la contraseña WiFi.

## TLS en el ESP32. El componente ESP-TLS

ESP-IDF proporciona un componente (ESP-TLS) que ofrece una interfaz ([API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_tls.html))
simplificada para acceder a funcionalidad básica TLS. Aún así, ofrece 
una funcionalidad suficientemente amplia como para implementar casos de uso
comunes en entornos IoT. 

La API de ESP-TLS es sencilla, y se basa en el uso de cuatro funciones básicas:

### Establecimiento de conexión TLS (`esp_tls_conn_new_sync()`)

* Prototipo:

```c
int esp_tls_conn_new_sync(const char *hostname, int hostlen, int port, const esp_tls_cfg_t *cfg, esp_tls_t *tls)
```

* Descripción: Crea una nueva conexión TLS/SSL bloqueante, estableciendo dicha
  conexión contra un servidor establecido. 

* Parámetros: 
    - `hostname`: Identificación del host.
    - `hostlen`: Longitud del parámetro `hostname`.
    - `port`: Puerto de conexión con el host.
    - `cfg`: Configuración de la conexión TLS.
    - `tls`: Puntero a `esp_tls_t` (manejador de la conexión). Devuelve `NULL` si se produce un error en la conexión.

### Destrucción de conexión TLS (`esp_tls_conn_delete()`)

```c
 void esp_tls_server_session_delete(esp_tls_t *tls)
```

* Descripción: Cierra la conexión TLS/SSL. 

* Parámetros: 
    - `tls`: Manejador de la conexión.

### Escritura de datos (`esp_tls_conn_read()`)

```c
ssize_t esp_tls_conn_write(esp_tls_t *tls, const void *data, size_t datalen)
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
ssize_t esp_tls_conn_read(esp_tls_t *tls, void *data, size_t datalen)
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
  struct esp_tls tls;
  ret = esp_tls_conn_new_sync( HOST_IP_ADDR, longitud, PORT, &cfg, &tls);

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
  esp_tls_server_session_delete( tls );

  vTaskDelete( NULL );
}

void app_main( void )
{
  // ...
}
```

!!! danger "Tarea entregable"
    Estudia y prueba el ejemplo base `examples/protocols/https_request`. 
    Opcionalmente, modifica este ejemplo para que interactue con el servidor 
    TLS empleando previamente.


#### Desactivación de la comprobación de certificado de servidor

Basta con modificar la configuración del proyecto del siguiente modo:

```
# CONFIG_EXAMPLE_CLIENT_SESSION_TICKETS is not set
CONFIG_ESP_TLS_INSECURE=y
CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE is not set
```

Y comentar estás líneas del fichero `https_request_example_main.c` para borrar la configuración de la entidad de certificación (de lo contrario prevalecería sobre la opción `CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY`):

```c
  esp_tls_cfg_t cfg = {
  //    .cacert_buf = (const unsigned char *) server_root_cert_pem_start,
  //    .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
  ...
  };
```

#### Servidor web de test con certificado autofirmado

Para comprobar el ejemplo `examples/protocols/https_request` con un servidor https local con certificado autofirmado se pueden emplear los siguientes comandos de `openssl`.

* *Generación del certificado autofirmado.* Es preciso proporciona cierta información, entre otras cosas la dirección IP o nombre del host.

```sh
openssl openssl genrsa -out server.key 2048
```

```sh
openssl openssl req -new -x509 -key server.key -out server.crt -days 365

You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:ES
State or Province Name (full name) [Some-State]:Madrid
Locality Name (eg, city) []:Madrid
Organization Name (eg, company) [Internet Widgits Pty Ltd]:UCM
Organizational Unit Name (eg, section) []:FDI
Common Name (e.g. server FQDN or YOUR name) []:<IP HOST>
Email Address []:lpinuel@ucm.es
```

* *Arranque del servidor web*.

```sh
openssl openssl s_server -accept 8443 -cert server.crt -key server.key -www

Using default temp DH parameters
ACCEPT
```
