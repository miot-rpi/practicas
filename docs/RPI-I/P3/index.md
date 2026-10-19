# LAB3. Wi-Fi: aprovisionamiento y ahorro de energía

## Objetivos

Esta práctica se divide en dos partes en las que se estudiarán dos
aspectos avanzados de Wi-Fi. Los objetivos de cada parte son los siguientes:

* Aprovisionamiento de credenciales Wi-Fi:
    - Entender y experimentar con distintos modos de aprovisionamiento de 
    credenciales Wi-Fi, vía *BLE* y *SoftAP*.
    - Comprobar el intercambio de claves en claro realizando aprovisionamientos
    desde la línea de comandos, así como observar la utilidad (y necesidad)
    del envío cifrado de credenciales.

* Modos de ahorro de energía:
    - Entender los tres modos de funcionamiento del ESP32 en cuanto a ahorro
    energético en conexiones Wi-Fi.
    - Observar la desviación en latencia de recepción de paquetes en función
    del modo aplicado.

## Parte 1. Aprovisionamiento de credenciales Wi-Fi

Entendemos por aprovisionamiento Wi-Fi el mecanismo o mecanismos mediante los
cuales es posible proporcionar a un dispositivo, de forma externa y segura, el
conjunto de credenciales necesario para conectarse a una red Wi-Fi. Es decir, si
nuestro código en el ESP32 está preparado para conectarse a una Wi-Fi por WPA2
personal, en lugar de configurar una red fija o una serie de redes válidas,
añadimos a nuestro código un componente de aprovisionamiento, que permitirá al
usuario conectarse al ESP32 para indicarle el nombre de la red Wi-Fi (SSID) a la
que conectarse y la contraseña que debe utilizar. El aprovisionamiento, por tanto,
suele hacerse la primera vez que se usa el dispositivo y no suele ser necesario repetirlo.

ESP-IDF proporciona un componente que ofrece un servicio de aprovisionamiento
Wi-Fi por dos mecanismos: Bluetooth Low Energy (modo *BLE*) o a través de un
punto de acceso Wi-Fi temporal (modo *SoftAP*). ESP-IDF proporciona una serie de
APIs (con nombres de función `network_prov_mgr_*`) para implementar de forma sencilla ambos
modos de aprovisionamiento.

!!! note "Nota"
    En las versiones recientes de ESP-IDF, el componente integrado `wifi_provisioning` ha sido
    sustituido por el componente externo `network_provisioning`, por lo que no encontraremos el ejemplo
    a trabajar (***wifi_prov***) en el repositorio de ejemplos base. Para descargarlo, ejecuta el
    siguiente comando desde una terminal ESP-IDF:
    ```
    idf.py create-project-from-example "espressif/network_provisioning=1.3.1:wifi_prov"
    ```

### Inicialización del servicio de aprovisionamiento

La función `network_prov_mgr_init()` se utiliza para configurar e inicializar
el componente de aprovisionamiento y debe invocarse antes de cualquier
otra llamada a funciones de la API `network_prov_mgr*`. Además, cabe
destacar que el componente de aprovisionamiento depende en las funcionalidades
de otros componentes (en particular de NVS, TCP/IP, el bucle de eventos y Wi-FI), por lo que estos
deben inicializarse previamente. Para finalizar el componente de aprovisionamiento,
basta con llamar a la función `network_prov_mgr_deinit()`.

Un ejemplo de inicialización sería:

```c
network_prov_mgr_config_t config = {
  .scheme = network_prov_scheme_ble,
  .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
};

ESP_ERR_CHECK(network_prov_mgr_init(config));
```

La estructura de configuración `network_prov_mgr_config_t` contiene campos que permiten
especificar el comportamiento del componente. El campo `scheme` define el esquema o tipo de aprovisionamiento a utilizar, mientras que el campo `scheme_event_handler` especifica la función
*callback* que tratará los eventos relacionados con el componente de aprovisionamiento.

Hay tres esquemas de aprovisionamiento disponibles:

- `network_prov_scheme_softap`: se crea un punto de acceso temporal desde el nodo a
  aprovisionar. El aprovisionador debe conectarse a este punto de acceso y
  transmitir las credenciales vía Wi-Fi, utilizando el servidor HTTP creado por el
  componente de aprovisionamiento.
- `network_prov_scheme_ble`: el nodo crea un servidor GATT al que se puede conectar
  el aprovisionador utilizando Bluetooth Low Energy (BLE), que usará la tabla
  GATT para comunicar las credenciales Wi-Fi.
- `network_prov_scheme_console`: permite transmitir la información de aprovisionamiento
  a través de la consola serie.

Una vez aprovisionado el nodo, las credenciales se almacenan en la memoria flash,
quedando disponibles tras los siguientes reinicios del dispositivo.

### Comprobación del estado de aprovisionamiento

Es posible comprobar el estado de aprovisionamiento de un dispositivo mediante
la función `network_prov_mgr_is_wifi_provisioned()`, que permite verificar si las credenciales
de conexión Wi-Fi están ya almacenadas en la memoria no volátil (NVS).

Aunque existen varios métodos para eliminar la información de aprovisionamiento
almacenada en la NVS, utilizaremos el mecanismo proporcionado por `idf.py` para
borrar su contenido. Para ello, ejecutaremos el comando:

```sh
idf.py erase-flash
```

### Parámetros de inicialización del servicio de aprovisionamiento

Al inicializar el componente de aprovisionamiento, es necesario especificar
un nombre de servicio y, opcionalmente, una clave. Esto se traduce en:

- SSID y contraseña para el modo *SoftAP* (es decir, cuando el esquema 
  de aprovisionamiento se ha configurado como `network_prov_scheme_softap`).

- Nombre del dispositivo BLE para el modo *BLE* (cuando el esquema 
  de aprovisionamiento se ha configurado como `network_prov_scheme_ble`).

Además, internamente el componente de aprovisionamiento utiliza el mecanismo de
comunicación *protocomm*, que permite configurar tres niveles de seguridad para
la transmisión de las credenciales de aprovisionamiento:

- *Nivel 0*: las credenciales se intercambian en texto claro, sin cifrado ni
  prueba de posesión (*Proof of Possession*, PoP). Un PoP es una cadena secreta
  (por ejemplo, `abcd1234`) que el aprovisionador debe conocer para demostrar
  que está autorizado a establecer una sesión segura.

- *Nivel 1*: se realiza un *handshake* previo entre ambos extremos, con
  intercambio de claves y utilizando cifrado AES-CTR para proteger los datos.
  Admite el uso de PoP.

- *Nivel 2*: se realiza un *handshake* previo entre ambos extremos, con
  intercambio de claves y utilizando cifrado AES-GCM para proteger los datos.
  Este nivel permite utilizar el mecanismo de sal y verificador (salt and
  verifier) para la autenticación.

De esta forma, un ejemplo de inicialización del gestor de aprovisionamiento para
el nivel 2 podría resultar en el siguiente código:

```c
network_prov_security_t security = NETWORK_PROV_SECURITY_2;

network_prov_security2_params_t sec2_params = {};

ESP_ERROR_CHECK(example_get_sec2_salt(&sec2_params.salt, &sec2_params.salt_len));
ESP_ERROR_CHECK(example_get_sec2_verifier(&sec2_params.verifier, &sec2_params.verifier_len));

network_prov_security2_params_t *sec_params = &sec2_params;

char service_name[12];
get_device_service_name(service_name, sizeof(service_name));

const char *service_key = NULL;

ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key));
```

El proceso de aprovisionamiento finalizará automáticamente al conectar el
ESP32 a un AP con éxito (es decir, al obtener una IP). En cualquier caso, 
también puede detenerse de forma manual en cualquier momento mediante la
función `network_prov_mgr_stop_provisioning()`.

### Espera a la finalización del proceso de aprovisionamiento

Habitualmente, las aplicaciones de usuario que se ejecutan en el ESP32 deben
esperar a que finalice el proceso de aprovisionamiento antes de continuar. Una
vez completado el proceso, se librean los recursos utilizados por el gestor de
aprovisionamiento y se ejecuta la lógica principal de la aplicación.

Existen dos mecanismos para implementar este comportamiento:

- Mediante una llamada **bloqueante** a la función `network_prov_mgr_wait()`, que
  permite esperar a que finalice el proceso de aprovisionamiento:

```c
// Inicialización del servicio de aprovisionamiento
ESP_ERR_CHECK(network_prov_mgr_start_provisioning(security, pop, service_name, service_key));

// Esperar a que finalice el aprovisionamiento
network_prov_mgr_wait();

// Liberar recursos
network_prov_mgr_deinit();

// A partir de aquí, comenzaría la lógica de la aplicación
// ...
```

- El segundo mecanismo estaría basado en eventos (es decir, sería **no bloqueante**),
  interceptando eventos de tipo `NETWORK_PROV_EVENT` y llamando a `network_prov_mgr_deinit()`
  cuando el evento sea `NETWORK_PROV_END`:

```c
static void event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data)
{
    if (event_base == NETWORK_PROV_EVENT && event_id == NETWORK_PROV_END) {
        // Liberar recursos una vez el proceso de aprovisionamiento ha finalizado
        network_prov_mgr_deinit();
    }
}
```

### Herramientas de aprovisionamiento para dispositivos móviles

Existen aplicaciones para móviles preparadas por Espressif para llevar a cabo el
proceso de aprovisionamiento de dispositivos ESP32. Estas aplicaciones están disponibles
para Android e iOS y permiten realizar el aprovisionamiento mediante transporte BLE y SoftAP:

- Android:
    - [Aprovisionamiento *BLE*](https://play.google.com/store/apps/details?id=com.espressif.provble)
    - [Aprovisionamiento *SoftAP*](https://play.google.com/store/apps/details?id=com.espressif.provsoftap)

- iOS:
    - [Aprovisionamiento *BLE*](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141)
    - [Aprovisionamiento *SoftAP*](https://apps.apple.com/in/app/esp-softap-provisioning/id1474040630)
 
Estas aplicaciones se comunican de forma sencilla con el ESP32 que aún no ha
sido aprovisionado. El mecanismo de comunicación depende del tipo de transporte
utilizado. En el caso de BLE, se define una tabla GATT con distintas características
que permiten escribir datos en el dispositivo. Veremos qué es una tabla GATT en las
próximas prácticas. En el caso de SoftAP, se crean una serie de *endpoints* (URIs HTTP)
que permiten leer y escribir de forma sencilla los datos que se desean intercambiar
entre ambos extremos de la comunicación.

La siguiente tabla resume los *endpoints* definidos por la implementación estándar
del protocolo de aprovisionamiento. Estos pueden modificarse o ampliarse en función
de la información adicional que deseemos intercambiar:

|                                     | Endpoint (BLE + Servidor GATT) | URI (SoftAP + HTTP)       |
| ----------------------------------- | ------------------------------ | ------------------------- |
| Establecimiento de sesión           | prov-session                   | http://IP:80/prov-session |
| Escaneo de redes disponibles        | prov-scan                      | http://IP:80/prov-scan    |
| Configuración del aprovisionamiento | prov-config                    | http://IP:80/prov-config  |
| Versión del protocolo               | proto-ver                      | http://IP:80/proto-ver    |

!!! danger "Ejercicio 1"
    Utilizando las aplicaciones móviles, aprovisiona tu ESP32 tanto mediante BLE como
    mediante SoftAP, empleando las credenciales de la red Wi-Fi del laboratorio, de tu
    casa o de un AP creado con otro móvil. Cambia el modo de aprovisionamiento desde
    `menuconfig`. Antes de repetir cada experimento, ejecuta `idf.py erase-flash` para
    borrar también la información de aprovisionamiento de sesiones anteriores.

!!! danger "Ejercicio 2"
    Revisa el código del ejemplo e identifica y explica las variables y los valores
    utilizados para cada uno de los niveles de seguridad (pista: consulta la llamada a
    `network_prov_mgr_start_provisioning()`).

Los detalles de este tipo de protocolos de aprovisionamiento van más allá de los
objetivos de esta práctica. No obstante, es conveniente disponer de algún mecanismo
que permita inspeccionar lo que hacen y determinar, por ejemplo, si el intercambio
de credenciales se realiza en texto plano (en claro) o de forma cifrada. Para ello,
podemos utilizar `esp_prov.py`, una herramienta de línea de comandos incluida en el
componente `network_provisioning` descargado como parte del ejemplo *wifi_prov*
(directorio `managed_components/espressif__network_provisioning/tool/esp_prov`).

!!! note "Nota"
    Antes de poder utilizar `esp_prov.py`, es necesario instalar sus dependencias
    ejecutando el siguiente comando desde una terminal ESP-IDF:
    ```
    python -m pip install protobuf
    ```

Esta herramienta permite aprovisionar dispositivos desde nuestro PC. Además,
ofrece la posibilidad de utilizar un *endpoint* adicional (*custom-data*) para
enviar información adicional de aprovisionamiento, como el nombre del dispositivo
o las credenciales necesarias para acceder a un servidor externo.

Su uso es sencillo y puede consultarse ejecutando `python esp_prov.py -h`.
Básicamente, la herramienta actúa como cliente de aprovisionamiento y permite
comunicarse con el dispositivo mediante BLE o SoftAP.

Si se usa SoftAP, el PC debe conectarse a la Wi-Fi temporal creada por el dispositivo
a aprovisionar. El componente de aprovisionamiento del ESP32 expondrá un servidor HTTP
en 192.168.4.1:80 al que se conectará la herramienta para realizar el aprovisionamiento
y enviar las credenciales indicadas en la línea de comandos:

```sh
python esp_prov.py --transport softap --service_name "192.168.4.1:80" --sec_ver 0 --ssid SSID_EJEMPLO --passphrase CLAVE_EJEMPLO
```

Podemos monitorizar el tráfico de red utilizando herramientas de captura de paquetes
como Wireshark. Esto nos permitirá inspeccionar el contenido de los paquetes
transmitidos y analizar si las credenciales se envían en texto claro o si, por el
contrario, están cifradas.

También podemos utilizar BLE y niveles de seguridad más avanzados:

```sh
python esp_prov.py --transport ble --sec_ver 2
```

Si no especificamos todos los datos necesarios mediante parámetros de línea de comandos,
la herramienta solicitará de forma interactiva la información que falte.

!!! danger "Ejercicio 3"
    Modifica el código del ejemplo para forzar el nivel de seguridad 0 y realiza
    el aprovisionamiento mediante SoftAP utilizando `esp_prov.py`. Captura el tráfico de
    red con Wireshark y comprueba que las credenciales se envían sin cifrar. Después,
    vuelve a configurar el nivel de seguridad 2, repite el proceso y observa cómo las
    claves se intercambian cifradas.

!!! danger "Ejercicio 4"
    Realiza el aprovisionamiento mediante BLE o SoftAP y utiliza el parámetro
    *--custom_data* de `esp_prov.py` para enviar un nombre de dispositivo.
    Antes, modifica el programa para que en el bucle principal se muestre el
    mensaje "*Hello world from ...*", sustituyendo los puntos suspensivos por
    el nombre recibido durante el aprovisionamiento.

## Parte 2. Modos de ahorro de consumo Wi-Fi

ESP-IDF admite distintos modos de ahorro de energía, con soporte tanto a
nivel de dispositivo (*station*) como de punto de acceso (*AP*). Estos
modos se basan en las características de ahorro energético contempladas
en el estándar IEEE 802.11, concretamente en el modo *Modem-sleep*.

Este modo funciona exclusivamente cuando un dispositivo está configurado como
*station* y se encuentra conectado a un AP. Cuando el modo *Modem-sleep* está
activo, el dispositivo alterna periódicamente su estado entre *activo* y
*sleep*. Durante el estado *sleep*, tres de los principales componentes del
subsistema de comunicación inalámbrica (PHY, BB y RF) se desactivan para
reducir el consumo energético. Pese a permanecer inactivos, la estación mantiene
su conexión lógica con el AP en todo momento. Este modo ofrece dos submodos de
ahorro energético: *mínimo* y *máximo*:

- En el modo *mínimo*, la estación se despierta cada intervalo *DTIM* (por ejemplo,
cada tres beacons) para recibir los datos pendientes. El *DTIM* es un indicador
incluido en beacons concretos que señala cuándo se transmitirán las tramas *broadcast*
y *multicast* almacenadas en el AP. En este modo, la estación puede recibir estas tramas
sin problema, siempre que permanezca conectada y siga correctamente este mecanismo de
recepción. Sin embargo, el ahorro energético puede ser limitado si el *DTIM* es corto.
Además, este intervalo está determinado por el AP, por lo que la estación no puede
modificarlo.

- En el modo *máximo*, la estación se despierta cada intervalo de escucha (intervalo
definido por ella misma) para recibir los datos pendientes. Este intervalo no tiene por
qué coincidir con el *DTIM* y, de hecho, suele configurarse con un valor mayor para aumentar
el ahorro energético. En este modo, las tramas *broadcast* y *multicast* pueden perderse si
la estación permanece en estado de reposo cuando el AP las transmite tras el *DTIM*. El
intervalo de escucha puede configurarse mediante una llamada a `esp_wifi_set_config()`
antes de establecer la conexión con el AP.

Cabe destacar que el mecanismo *DTIM* indica cuándo se transmitirán desde el AP las tramas
*broadcast* y *multicast*. Por su parte, las tramas *unicast* dirigidas a una estación se
almacenarán temporalmente en el AP mientras esta permanezca dormida y se solicitarán y
entregarán cuando vuelva a estar activa.

Para activar el modo *mínimo*, es necesario invocar a la función `esp_wifi_set_ps(WIFI_PS_MIN_MODEM)`.
En cambio, para configurar el modo *máximo*, utilizaremos `esp_wifi_set_ps(WIFI_PS_MAX_MODEM)`. En
ambos casos, la llamada debe realizarse después de `esp_wifi_init()`. Los modos de ahorro de energía
se activarán cuando la estación se conecte al AP y dejarán de funcionar cuando se desconecte.

Es posible desactivar el ahorro energético mediante una llamada a `esp_wifi_set_ps(WIFI_PS_NONE)`. Obviamente, esto aumentará el consumo energético, pero reducirá la latencia de recepción de los
datos. Con el modo de ahorro activado, la recepción de los datos se retrasará tanto como el
intervalo *DTIM* (en modo *mínimo*) o el intervalo de escucha (en modo *máximo*). El modo de
ahorro por defecto es `WIFI_PS_MIN_MODEM`.

En el modo *AP*, ESP-IDF no soporta todas las funciones de ahorro energético definidas en el
estándar Wi-Fi. En concreto, un AP programado vía ESP-IDF solo almacenará temporalmente las
tramas *unicast* destinadas a las estaciones conectadas a dicho AP, pero no las tramas
*broadcast* ni *multicast*. Por tanto, las estaciones que tengan activado el ahorro energético
podrían perder estas tramas si permanecen dormidas cuando se transmiten.

El ejemplo ***power_save***, incluido en el directorio *wifi*, ilustra mediante un código
sencillo la configuración de una estación en los dos modos de ahorro energético. Estos modos
pueden seleccionarse a través de `menuconfig`. Además, se ofrece una opción para modificar
el intervalo de escucha en el modo de ahorro *máximo*.

!!! danger "Ejercicio 5"
    Compila, flashea y ejecuta el ejemplo *power_save* utilizando los tres modos
    disponibles: sin ahorro energético, con ahorro *mínimo* y con ahorro *máximo*.
    En todos los casos, conecta el ESP32 a un punto de acceso y, desde un portátil 
    conectado al mismo AP, ejecuta diferentes pings dirigidos al ESP32 y observa cómo
    varía el tiempo de respuesta. En el modo de ahorro *máximo*, prueba distintos
    intervalos de escucha y comprueba cómo afectan a los tiempos de respuesta de los
    pings.

    Para cada modo, representa gráficamente el tiempo de respuesta de los pings.
    Relaciona los resultados con los intervalos *DTIM* y de escucha, y discute los
    resultados obtenidos.
