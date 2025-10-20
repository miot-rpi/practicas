# Práctica 5. El protocolo MQTT (I). Despliegue de clientes y servidores/*brokers*. Análisis de tráfico

## Objetivos

* Familiarizarse con el uso de *brokers* y clientes de suscripción/publicación
utilizando MQTT.

* Desplegar un sistema basado en MQTT local, incluyendo *broker* y clientes.

* Utilizar Eclipse Paho para integrar funcionalidad MQTT en programas Python.

* Familiarizarse con el uso de *wildcards* MQTT.

## Publicación/suscripción contra un *broker* en la nube

En la primera parte de la práctica, utilizaremos un servidor/*broker* disponible 
en la nube para su uso libre por parte de los usuarios (*test.mosquitto.org*). 
Este servidor suele utilizarse con fines de pruebas básicas y depuración, y hay
que ser consciente de que toda la información que en él se publica puede ser
leída por cualquier suscriptor. Debe tenerse este dato en cuenta a la hora
de publicar información sensible a través de MQTT cuando se use el servidor de
pruebas.

El servidor escucha en los siguientes puertos:


* **1883**: MQTT, sin encriptación.
* **8883**: MQTT, con encriptación.
* **8884**: MQTT, con encriptación, certificado de cliente requerido.
* **8080**: MQTT sobre WebSockets, sin encriptación.
* **8081**: MQTT sobre WebSockets, con encriptación.

Para realizar publicaciones/suscripciones contra el *broker* utilizaremos
la distribución [*mosquitto*](https://mosquitto.org) del proyecto Eclipse IoT.
Aunque *mosquitto* es principalmente una implementación de *broker* MQTT, nosotros
la utilizaremos en este paso a modo de cliente, lo que nos permitirá suscribirnos
o publicar sobre cualquier *topic* MQTT.

En primer lugar, instala *mosquitto*:

```sh
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients mosquitto-dev libmosquitto*
```

Si todo ha ido bien, deberías disponer de dos binarios listos para ejecución:

* `mosquitto_sub`: permite suscribirse a un determinado *topic* utilizando un 
*broker*.
* `mosquitto_pub`: permite publicar un mensaje asociado a un determinado *topic*
utilizando un *broker*.

!!! note "Tarea 1"
    Observa la ayuda de ambas ordenes, utilizando el argumento `--help`. Identifica
    los parámetros que te permitirán especificar el *broker* destino, el *topic*
    a utilizar y, en el caso de la publicación, el *mensaje* a enviar. 

Suscribámonos al *topic* `#` en el *broker*, utilizando para ello la orden:

```sh
mosquitto_sub -h test.mosquitto.org  -t "#"
```

!!! note "Tarea 2"
    Pausa la salida en cuanto puedas. ¿A qué corresponden los mensajes que estás
obteniendo?

A continuación, vamos a realizar un proceso de publicación/suscripcion con un 
*topic* conocido (por ejemplo, `/MIOT/tunombre/`). Para publicar un mensaje
bajo dicho *topic*:

```sh
mosquitto_pub -h test.mosquitto.org -t "/MIOT/tunombre" -m "Hola, soy tunombre"
```

!!! note "Tarea 3"
    Suscríbete al topic `/MIOT/tunombre` y observa si recibes los resultados 
    tras la publicación correspondiente. ¿Cómo podrías suscribirte a todos
    los mensajes publicados por compañeros?

!!! danger "Tarea entregable"
    Realiza un análisis del intercambio de mensajes necesario para un proceso
    de publicación/suscripción contra el *broker* de test. Incide en el tipo 
    de protocolo de capa de transporte que utiliza MQTT, mensajes de datos
    y control, sobrecarga del protocolo de capa de aplicación, y en general, 
    cualquier aspecto que consideres de interés, incluyendo el uso de opciones
    relativas a QoS.

## Despliegue de un broker local usando Eclipse Mosquitto

El uso de un servidor remoto presenta ventajas (facilidad de uso), pero una gran
cantidad de inconvenientes (seguridad, imposibilidad de configuración avanzada, 
...)

En esta sección, configuraremos un broker *mosquitto* para el despliegue de una
infraestructura MQTT local o remota bajo nuestro control.

El arranque de un *broker* (servidor) *mosquitto* se realiza mediante el propio
comando `mosquitto`:

```sh
mosquitto [-c config file] [ -d | --daemon ] [-p port number] [-v]
```

Sin embargo, en la mayoría de distribuciones Linux, el *broker* arranca por
defecto y se ejecuta constantemente en segundo plano. Para comprobar el estado
de funcionamiento del *broker*, basta con ejecutar:

```sh
sudo service mosquitto status
```

Observarás un mensaje que indica que el servicio está activo. Las opciones
`restart`, `start` o `stop` te permitirán controlar el estado del *broker* en
todo momento.

!!! note "Tarea 4"
    Comprueba que, con el *broker* arrancado, puedes realizar un proceso de
    suscripción/publicación contra el mismo.

El *broker* *mosquitto* permite monitorizar sus propias estadísticas e información
de estado utilizando el protocolo MQTT. Así, los *topics* `$SYS` retornan, bien
periódicamente o bien cuando sucede un evento de interés, la información
de estado del *broker*. Puedes consultar más detalles en la página de manual
de *mosquitto* (comando `man mosquitto`), en el epígrafe *BROKER STATUS*.

!!! note "Tarea 5"
    Comprueba el estado del *broker* mientras realizas procesos de suscripción/publicación
    reportando bytes recibidos/enviados, número de conexiones activas e inactivas, 
    y número de mensajes enviados/recibidos por el *broker*.

## Wildcards

Además de permitir el uso de *topics* completos para el proceso de suscripción,
los topics pueden incluir *wildcards* o comodines en su estructura. `+` es la
*wildcard* utilizada para obtener correspondencias con un único nivel de la 
jerarquía. Así, para un *topic* `a/b/c/d`, las siguientes suscripciones 
corresponderán con éxito:

* `a/b/c/d`
* `+/b/c/d`
* `a/+/c/d`
* `a/+/+/d`
* `+/+/+/+`

Pero no las siguientes:

* `a/b/c`
* `b/+/c/d`
* `+/+/+`

La segunda *wildcard* soportada es `#`, y permite corresponencias con cualquier
nivel sucesivo de la jerarquía.  Así, para un *topic* `a/b/c/d`, las siguientes suscripciones 
corresponderán con éxito:

* `a/b/c/d`
* `#`
* `a/#`
* `a/b/#`
* `a/b/c/#`
* `+/b/c/#`

!!! note "Tarea 6"
    Experimenta con el uso de *wildcards* desde múltiples clientes suscriptores. Resultará de especial
    interés observar (por ejemplo, con capturas de tráfico) el ahorro de mensajes asociado al uso de
    *wildcards* que restrinjan qué mensajes son recibidos y por qué clientes.


## Desarrollo de un clientes locales. Eclipse Paho

Los clientes `mosquitto_pub` y `mosquitto_sub` son básicamente herramientas de
desarrollo y pruebas, pero resulta interesante conocer bibliotecas que permitan
la integración de MQTT en programas existentes. Una de ellas es 
[Eclipse Paho](https://www.eclipse.org/paho). Paho es una infraestructura
desarrollada en el proyecto Eclipse IoT para dar soporte a implementaciones
de protocolos de mensajería M2M e IoT, aunque, en este momento, su uso principal
se centra exclusivamente en MQTT.

En nuestro caso, utilizaremos la versión Python de la biblioteca, instalable
vía:

```sh
pip install paho-mqtt
```

Dispones de la documentación del módulo a través de [este enlace](https://pypi.org/project/paho-mqtt/#usage-and-api).

El despliegue de un ejemplo sencillo para un cliente que se conecta a un *broker*
y se suscribe al tópico `$SYS`, imprimiendo los mensajes recibidos, resultaría,
utilizando Paho, en el siguiente código Python:

```python
import paho.mqtt.client as mqtt

# Funcion callback invocada cuandl el cliente recibe un CONNACK desde el broker.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Suscribirse en on_connect() asegura que si se pierde la conexión y 
    # se reestablece, las suscripciones se renovarán.
    client.subscribe("$SYS/#")

# Funcion callback al recibir un mensaje de publicacion (PUBLISH) desde el 
# broker.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("test.mosquitto.org", 1883, 60)

# Llamada bloqueante que procesa el tráfico de red, invoca callbacks
# y maneja la reconexión al broker. 
client.loop_forever()
```

La clase cliente puede utilizarse para:

* Crear una instancia de cliente MQTT.
* Conectar a un *broker* usando las funciones de la familia `connect*()`.
* Invocar a funciones de la familia `loop*()` para mantener el tráfico de datos con el servidor.
* Utilizar `subscribe()` para suscribirse a un *topic* y recibir mensajes.
* Utilizar `publish()` publicar mensajes en el *broker*.
* Utilizar `disconnect()` para desconectar del *broker*.

Los *callbacks* se invocarán automáticamente para permitir el procesamiento
de eventos. De entre los más utilizados, destacan:

* `ON_CONNECT`: invocado cuando el *broker* responde a nuestra petición de conexión.
Ejemplo:
```python
def on_connect(client, userdata, flags, rc):
    print("Connection returned result: "+connack_string(rc))
```
* `ON_DISCONNECT`: invocado cuando el cliente se desconecta del *broker*.
Ejemplo:
```python
def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Unexpected disconnection.")
```
* `ON_MESSAGE`: invocado cuando se recibe un mensaje en un *topic* al que 
* el cliente está suscrito. 
Ejemplo:
```python
def on_message(client, userdata, message):
    print("Received message '" + str(message.payload) + "' on topic '"
        + message.topic + "' with QoS " + str(message.qos))
```

Para publicar de forma puntual sobre un *broker* (sin mantener
una conexión establecida), es posible utilizar la siguiente secuencia de
ordenes:

```python
import paho.mqtt.publish as publish

publish.single("paho/test/single", "payload", hostname="mqtt.eclipse.org")
```

Del mismo modo, podemos suscribirnos de forma puntual mediante una llamada
bloqueante a:

```python
import paho.mqtt.subscribe as subscribe

msg = subscribe.simple("paho/test/simple", hostname="mqtt.eclipse.org")
print("%s %s" % (msg.topic, msg.payload))
```

Toda la información y documentación asociada al módulo puede consultarse
[aquí](https://pypi.org/project/paho-mqtt/).

!!! danger "Tarea entregable"
    Cada alumno propondrá una solución para monitorizar un edificio inteligente a
    través de un sistema de mensajería MQTT. Para ello, cabe destacar que el
    edificio constará de:

    * Un identificador del tipo EDIFICIO_TUPUESTODELABORATORIO.
    * Un conjunto de plantas, identificadas por la cadena "P_NUMPLANTA".
    * En cada planta, cuatro alas (norte -N-, sur -S-, este -E-, oeste -O-)
    * En cada ala, un conjunto de salas, identificadas por un valor numérico.
    * En cada sala, cuatro sensores: TEMP (temperatura), HUM (humedad), LUX (luminosidad), VIBR (vibración).
    
    Se pide, en primer lugar, diseñar la jerarquía de *topics* que permita una correcta 
    monitorización de los edificios.

    En segundo lugar, se desarrollará un programa Python cliente que publique, periódicamente
    y de forma aleatoria, objetos JSON 
    (opcionalmente puedes utilizar CBOR, usando los módulos correspondientes) que incluyan el valor de temperatura, humedad, luminosidad o
    vibración para una determinada sala del edificio, elegida también aleatoriamente, a través
    del *topic* correspondiente. Estos mensajes 
    estarán espaciados en el tiempo un número aleatorio de segundos.

    En tercer lugar, se piden las *wildcards* que permitan consultar distintos tipos de información
    jerárquica. Por ejemplo:

    * Todos los mensajes de temperatura para el edificio.
    * Todos los mensajes de vibración del ala oeste de la planta 2 del edificio.
    * Todos los mensajes de sensorización de la sala 4 del ala Sur de la planta 7 del edificio.
    * ...

    En último lugar, se pide desarrollar un programa Python que actúe a modo de
    alarma, y que muestre mensajes sólo si algún valor recibido para los 
    datos sensorizados supera un umbral preestablecido. En dicho caso, el programa
    mostrará el edificio, planta, ala, sala y sensor que ha producido la alarma, 
    junto con su valor numérico.

    Puedes utilizar el [módulo JSON](https://docs.python.org/3/library/json.html)
    para parsear los objetos recibidos en Python.
