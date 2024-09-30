# Práctica 1 (Parte 3). Programación con sockets en el ESP32

## Objetivos

* Familiarizarse con la API de *sockets* en ESP-IDF.
* Desarrollar esquemas básicos de sistemas cliente/servidor TCP y UDP
  utilizando ESP-IDF.
* Diseñar un protocolo de capa de aplicación para simular una aplicación
  cliente/servidor utilizando TCP y UDP para interactuar entre un host y 
  la placa ESP32

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

Acerca de la tarea principal (función `app_main`) observa que realiza 
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

* Por último, se crea una tarea que ejecutará la lógica del servidor (lo mismo
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
    pertenecerá la misma red. Para conseguirlo, para tu máquina virtual y añade
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

!!! note "Tarea 1.4"
    Experimenta con los ejemplos proporcionados en ESP-IDF (cliente/servidor
    TCP y UDP) y consigue ejecutar todos los elementos en la placa. Si sólo
    dispones de una placa, utiliza tu máquina (o una máquina virtual) como cliente/servidor
    para comprobar el correcto funcionamiento de cada código.

!!! note "Tarea 1.5 (entregable)"
    En este punto, deberías disponer de un conjunto de códigos que implementan
    sistemas cliente/servidor tanto en un host (utilizando C, y en Node-RED, así como
    en Python si has seguido el anexo opcional a la práctica) como en
    la placa ESP32 (utilizando C y ESP-IDF), y deberías haber comprobado su
    correcto funcionamiento.

    Específicamente, dispondrás de:

    * Código básico en C para implementación de un servidor/cliente *echo* 
    programado en C, cuyos códigos se proporcionan en este boletín.

    * Códigos básicos en C/ESP-IDF para implementar servidores/clientes
    *echo* sobre el ESP32.

    Como tarea, se pide que adaptes tu entrega de la tarea 1.3 para
    que tanto cliente como servidor puedan funcionar en el host (bien usando
    tu implementación Node-RED (o Python), o bien utilizando una implementación en 
    C) o en el ESP32. Se entregarán los códigos y una breve memoria con capturas
    de tráfico que demuestren el correcto funcionamiento del sistema.
