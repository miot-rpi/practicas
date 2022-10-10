# Práctica 2. El protocolo MQTT (II). Despliegue de clientes en el ESP32

## Objetivos

* Familiarizarse con el componente MQTT en ESP-IDF.

* Desplegar un cliente completo MQTT en el ESP32, incluyendo rutinas de publicación y suscripción.

* Implementar QoS y LWT en el ESP32.

## El componente MQTT en ESP-IDF

El componente ESP-MQTT es una implementación del protocolo MQTT en su parte
cliente, que permite la implementación completa de clientes MQTT en el ESP32,
incluyendo rutinas de publicación y suscripción a *brokers* existentes.

El componente soporte MQTT sobre TCP por defecto, así como funcionalidades 
avanzadas como SSL/TLS o MQTT sobre Websockets. Además, permite el despliegue
de múltiples instancias de cliente MQTT sobre la misma placa; el componente
implementa también parámetros avanzados soportados por el protocolo MQTT, como
autenticación (mediante nombre de usuario y contraseña), mensajes *last will* y
tres niveles de calidad de servicio (QoS).

### Eventos

Como otros componentes, la interacción entre el cliente MQTT y la aplicación
se basa en la recepción de eventos, entre los que destacan:

* `MQTT_EVENT_BEFORE_CONNECT`: El cliente se ha inicializado y va a comenzar el
proceso de conexión con el *broker*.
* `MQTT_EVENT_CONNECTED`: El cliente ha establecido de forma exitosa una conexión 
con el *broker* y está listo para enviar y recibir datos.
* `MQTT_EVENT_DISCONNECTED`: El cliente ha abortado la conexión.
* `MQTT_EVENT_SUBSCRIBED`: El *broker* ha confirmado la petición de suscripción
del cliente. Los datos contendrán el ID del mensaje de suscripción.
* `MQTT_EVENT_UNSUBSCRIBED`: El *broker* confirma la petición de *desuscripción*
del cliente. Los datos contendrán el ID del mensaje de *desuscripción*.
* `MQTT_EVENT_PUBLISHED`: El *broker* ha acusado la recepción de un mensaje
previamente publicado por el cliente. Este evento sólo se producirá cuando QoS sea
1 o 2, ya que el nivel 0 de QoS no utiliza acuses de recibo. Los datos asociados
al evento contendrán el ID del mensaje publicado.
* `MQTT_EVENT_DATA`: El cliente ha recibido un mensaje publicado en el *broker*.
Los datos asociados al evento contienen el ID del mensaje, nombre del *topic*,
datos recibidos y su longitud. 

### API

* `esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config)`

Rutina de inicialización del cliente MQTT. Devuelve un manejador de la conexión,
o `NULL` en caso de error. El parámetro `config` es una estructura con los 
parámetros que regirán la conexión, entre los que destacan 
(véase [la documentación del componente](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html#_CPPv424esp_mqtt_client_config_t)
para parámetros adicionales):

  - `esp_event_loop_handle_t event_loop_handle`: manejador para eventos MQTT.
  - `const char *uri`: URI del *broker* MQTT.
  - `uint32_t port`: puerto del *broker* MQTT.
  - `const char *username`: nombre de usuario, en caso de estar soportado por el
  *broker*.
  - `const char *password`: contraseña, en caso de estar soportada por el *broker*.
  - `const char *lwt_topic`: topic del mensaje LWT (*Last Will and Testament*).
  - `const char *lwt_msg`: contenido del mensaje LWT.
  - `int lwt_qos`: QoS del mensaje LWT.
  - `int lwt_retain`: flag *retain* para el mensaje LWT.
  - `int lwt_msg_len`: longitud del mensaje LWT.
  - `int keepalive`: valor del temporizador de *keepalive* (por defecto 120 segundos).

* `esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client)`

Rutina de arranque del cliente MQTT. Su único parámetro es el manejador devuelto 
por la anterior rutina.

* `int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos)`

Realiza la suscripción del cliente a un topic con el QoS determinado a través de 
su tercer parámetro. El cliente debe estar conectado al *broker* para enviar
el mensaje de suscripción.

* `int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic)`

*Desuscribe* al cliente de un determinado topic. El ciente debe estar conectado
al *broker* para poder enviar el mensaje correspondiente.

* `int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)`

El cliente publica un mensaje en el *broker*. El cliente no tiene que estar conectado
al *broker* para enviar el mensaje de publicación. En dicho caso, si `qos=0`, los
mensajes se descartarán, y si `qos>=1`, los mensajes se encolarán a la espera de 
ser enviados. Devuelve el identificador del mensaje publicado (si `qos=0`, el valor
de retorno siempre será 0), o `-1` en caso de error.

Parámetros de interés:

  - `client`: manejador del cliente MQTT.
  - `topic`: topic (en forma de cadena) bajo el cual se publicará el mensaje.
  - `data`: contenido del mensaje a publicar (es posible publicar un mensaje sin
  contenido, en cuyo caso se proporcionará un valor `NULL` en este parámetro).
  - `len`: longitud de los datos a enviar. Si se proporciona el valor `0`, se calcula
  su longitud a partir de la cadena `data`.
  - `qos`: nivel de QoS deseado.
  - `retain`: flag *Retain*.

!!! note "Tarea 2.8"
    Analiza el ejemplo `examples/protocols/mqtt/tcp`, y configuralo para que 
    utilice como *broker* el que desplegaste en la máquina virtual (asegúrate
    de que tanto máquina virtual como ESP32 pertenecen a la misma red).
    
    Realiza procesos de publicación y suscripción en la máquina virtual que 
    permitan visualizar los mensajes publicados por el ESP32 en tu terminal
    Linux, y los mensajes publicados desde el terminal Linux en la salida
    de monitorización del ESP32.

    Modifica el ejemplo y analiza el tráfico generado (a través de Wireshark)
    para los siguientes casos:

    1. Publicación de mensajes con niveles de QoS 0, 1 y 2.
    2. Activación o desactivación del flag *retain* en la publicación desde el
    ESP32.
    3. Configuración de un mensaje LWT con el topic */disconnected*. Para ello,
    reduce el valor de *keepalive* a 10 segundos, para que la detección de 
    desconexión sea más rápida. Deberás observar el envío del mensaje con 
    dicho *topic* transcurrido dicho tiempo desde una desconexión forzada del
    ESP32 si estás suscrito al mismo desde tu terminal Linux.

!!! danger "Tarea 2.9"
    Modifica el ejemplo proporcionado para que se integre en tu entorno de 
    monitorización de un edificio. Así, el *firmware* procederá creando una
    tarea que, periódicamente (cada *interval* segundos), publique un valor
    aleatorio para los cuatro parámetros monitorizados.

    Además, deberás diseña un sistema basado en MQTT mediante el cual puedas 
    controlar, externamente, el comportamiento del sensor, atendiendo a los
    siguientes criterios:

    1. El tiempo (*interval*) mediante que transcurrirá entre publicaciones
    será configurable a través de un proceso de publicación desde tu terminal 
    Linux y suscripción del ESP32 a un topic determinado.
    2. La sensorización (y publicación de datos) 
    podrá activarse o desactivarse bajo demanda
    a través de la publicación desde tu terminal Linux y suscripción del 
    ESP32 a un topic determinado.

    Por ejemplo, imagina que tu sensor publica mensajes de sensorización
    en el topic `/EDIFICIO_3/P_4/N/12/(TEMP|HUM|LUX|VIBR)`. Para controlar el 
    intervalo de publicación de datos desde dicho ESP32 y fijarlo a 1 segundo, 
    podríamos publicar un mensaje utilizando la orden:

    `mosquitto_pub -t /EDIFICIO_3/P_4/N/12/interval` -m "1000" -h IP_BROKER

    Para desactivar el sensor, podríamos utilizar:

    `mosquitto_pub -t /EDIFICIO_3/P_4/N/12/disable` -m "" -h IP_BROKER

    Para activar el sensor, podríamos utilizar:

    `mosquitto_pub -t /EDIFICIO_3/P_4/N/12/enable` -m "" -h IP_BROKER

    3. Opcionalmente, puedes ampliar tu solución para que cada sensor se active
    o desactive individualmente bajo demanda. En este caso, elige y documenta
    el topic utilizado.
