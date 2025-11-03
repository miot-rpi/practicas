# Práctica 4. Protocolos básicos de capa de aplicación. Websockets

## Objetivos

- Estudiar el intercambio de mensajes entre un cliente y un servidor 
  *websockets*, tanto en la fase de *handshake* como de intercambio de datos.
- Conseguir interactuar con un servidor *websockets* utilizando un navegador
  web como cliente.
- Estudiar el componente *websockets client* en ESP-IDF.
- Conocer el módulo Python *websockets* para desarrollar sistemas básicos
  cliente/servidor utilizando *websockets* (opcional).

## Interactuando con un navegador web 

Los *websockets* permiten el envío asíncrono bidireccional de información, entre un cliente y servidor web. La manera más sencilla de usarlos es mediante un navegador web convencional ya que la mayoría de ellos soportan este tipo de comunicación a través de *scripts Javascript*.

Como servidor emplearemos un ejemplo de referencia ([websocket-echo-server](https://github.com/websockets/websocket-echo-server)) que devuelve los mensajes recibidos. El análisis del código de este servidor queda fuera del ámbito de la presente práctica.

Para instalar el servidor se precisa de los siguientes comandos:

```sh
git clone https://github.com/websockets/websocket-echo-server.git
cd websocket-echo-server
npm ci --production
node index.js
```

Alternativamente, si no se quiere instalar software adicional en el host, se puede emplear un *Docker container* siguiendo las instrucciones del mismo repositorio ([enlace](https://github.com/websockets/websocket-echo-server?tab=readme-ov-file#running-the-server-in-a-docker-container)).

La configuración del servidor se lleva a cabo mediante las siguientes variables de entorno:

*  `BIND_ADDRESS`: dirección en la que escucha el servidor, por defecto `::`.
* `BIND_PORT`: puerto de escucha, por defecto `1337`.
*  `HEARTBEAT_INTERVAL`: intervalo (en milisegundos) entre envíos de mensajes de `ping` a los clientes para comprobar el estado de las conexiones, por defecto es `30000`.
* `HIGH_WATER_MARK`: umbral (en bytes) para el buffer de salida de cada conexión, cuando se supera, el envío de datos se suspende hasta que se drene el buffer, por defeco es `16384`.
* `MAX_MESSAGE_SIZE`: máximo tamaño de mensaje (en bytes), por defecto es `65536`.

Como cliente emplearemos la siguiente página web:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WebSocket Test</title>
    <script>
		// Create new WebSocket connection
		var mySocket = new WebSocket("ws://[::1]:1337");

		// Associate listeners
		mySocket.onopen = function(evt) {
		    console.log("WebSocket connection established.");
    
		    // Send data when the connection is open
		    mySocket.send("WebSocket Rocks!");
		};

		mySocket.onmessage = function(evt) {
		    console.log("Received message: " + evt.data);
		};

		mySocket.onclose = function(evt) {
		    console.log("WebSocket closed with status: " + evt.code);
		};

		mySocket.onerror = function(evt) {
		    console.error("WebSocket error:", evt);
		};

		// Optionally close the WebSocket after 5 seconds
		setTimeout(function() {
		    mySocket.close();
		    console.log("WebSocket connection closed.");
		}, 5000);
    </script>
</head>
<body>
    <h1>Check the Console for WebSocket output</h1>
</body>
</html>
```

!!! note "Tarea"
    Ejecuta el servidor en el *host* y, tras guardar el código fuente del cliente en un fichero `cliente.html`, ábrelo con el navegador Chrome y activa las herramientas de desarrollo (*DevTools*) para poder visualizar los mensajes mostrados en consola.

!!! note "Tarea"
    Analiza el flujo TCP, HTTP y WS intercambiado mediante *Wireshark*.

## Websockets en el ESP32

El soporte a nivel de cliente para el protocolo *websockets* está integrado
en ESP-IDF a través del componente *ESP websocket client*, cuya documentación
puede consultarse a través de este [enlace](https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html).

El componente *ESP websocket client* ofrece soporte para el protocolo *websocket*
tanto sobre TCP como sobre TLS. Como todos los componentes
en ESP-IDF, el componente *websocket* emite eventos que pueden ser tratados
por parte de la aplicación, entre los cuales destacan:

* `WEBSOCKET_EVENT_CONNECTED`: se emite una vez el cliente se ha conectado
  al servidor, previo al intercambio de datos.
* `WEBSOCKET_EVENT_ERROR`: en caso de error.
* `WEBSOCKET_EVENT_CLOSED`: la conexión se ha cerrado limpiamente.
* `WEBSOCKET_EVENT_FINISH`: el thread cliente va a cerrarse.
* `WEBSOCKET_EVENT_DATA`: se emite al recibir datos desde el servidor.

Este último evento es de especial interés para nosotros, ya que acarrea la
construcción de una estructura de tipo `esp_websocket_event_data_t` en la que
se almacena el mensaje recibido desde el servidor (tanto en sus campos de
control como de datos). Algunos campos de interés dentro de la estructura son:

- `data_ptr`: puntero a los datos recibidos (*payload*).
- `data_len`: tamaño (en bytes) de los datos recibidos.
- `op_code`: código de operación asociado al mensaje recibido.

La documentación del componente ofrece información sobre campos adicionales,
de menor interés para nosotros.

Observemos el código de una posible función manejadora de eventos del componente
*websocket*:

```c
tatic void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x2) { // Opcode 0x2 indicates binary data
            ESP_LOG_BUFFER_HEX("Received binary data", data->data_ptr, data->data_len);
        } else if (data->op_code == 0x08 && data->data_len == 2) {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        } else {
            ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
        }

        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    case WEBSOCKET_EVENT_FINISH:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
        break;
    }
}
```

Observa el código. En función del parámetro `event_id`, el manejador toma
un camino de ejecución u otro. Centrémonos en la recepción de un evento de
tipo `ẀEBSOCKET_EVENT_DATA`; a través de los distintos campos de la estructura
de información recibida (`esp_websocket_event_data_t`), es posible:

1. Obtener y mostrar el código de la operación (`op_code`).
2. Mostrar el contenido del mensaje recibido   (`data_ptr`).
3. Mostrar el tamaño del mensaje recibido (`data_len` y `payload_len`).

!!! note "Preguntas"
    - ¿Cuál es la diferencia entre los campos `data_len` y `payload_len`?
    - ¿Por qué el programa realiza un tratamiento especial cuando 
    `op_code == 8`?

Dada la anterior función manejadora, la inicialización de un cliente
*websockets* en el ESP32 es sencilla, y se resume en los siguientes pasos:

- **Configuración de URI (host + puerto)**

```c
esp_websocket_client_config_t websocket_cfg = {};
websocket_cfg.uri = "ws://localhost:1337";
esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
```

- **Asociación de manejador a eventos *Websocket***

```c
esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
```

- **Inicialización del cliente**

```c
esp_websocket_client_start(client);
```

A partir de este punto, la interacción con el servidor se puede realizar en 
base a funciones de envío de texto o binario:

```c
int esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout)

int esp_websocket_client_send_bin(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout)
```

No existen funciones de recepción, ya que ésta es implícita y se notifica vía
eventos.

### Ejemplo básico: cliente *echo*

Estudiaremos a continuación el ejemplo de cliente proporcionado por el componente `espressif/esp_websocket_client` para lo cual es preciso descargar el código del repositorio [esp-protocols](https://github.com/espressif/esp-protocols.git) ya sea mediante `git` o mediante fichero zip.

En este punto, configura, compila, flashea y monitoriza el ejemplo
`esp-protocols/components/esp_websocket_client/examples/target/`.

El ejemplo simplemente conecta con un servidor *echo* *Websockets* en la nube
(por defecto `wss://echo.websocket.events`). Dicho servidor simplemente espera, por
parte de cada cliente, el envío a través de la conexión de una cadena, 
respondiendo con la misma cadena en sentido contrario, siempre usando el 
mismo *socket*.

!!! note "Tarea"
    Observa el código del ejemplo y su ejecución. Determina cuál es el
    funcionamiento del ejemplo, y comprueba que los fragmentos de código 
    anteriores tienen su función dentro del código completo. ¿Cómo implementa
    el programa la espera limitada en tiempo si no se recibe ningún paquete
    tras cierto período?

!!! note "Tarea"
    Analiza las cosas especiales (ej. envíos parciales) mediante *Wireshark*.

!!! note "Tarea"
    Modifica el ejemplo para que se envíen y se reciban datos en formato JSON. 
    Nótese que el ejemplo ya dispone parcialmente de código para ello.

## Sistema cliente/servidor usando Websockets en Python

El módulo `websockets` proporciona la funcionalidad necesaria tanto a nivel de cliente
como de servidor para implementar sistemas basados en dicho protocolo. Concretamente,
las funciones de alto nivel que proporciona están basadas en una API de bajo
nivel que implementa las dos fases principales del protocolo *websockets*:

1. *Handshake* de apertura de comunicación, en forma de peticiones *HTTP upgrade*.
2. Transferencia de datos, y finalización de la comunicación con un *handshake* de cierre
de conexión.

La primera fase está diseñada para integrarse con software HTTP (cliente y servidor)
existente, y proporciona una implementación mínima para construir, parsear y 
validar peticiones y respuestas HTTP.

La segunda fase implementa el núcleo del protocolo *websockets*, y proporciona
una implementación completa basada en el 
módulo [asyncio](https://docs.python.org/3/library/asyncio.html)) 
de Python. 

Para utilizar el módulo `websockets` de Python, primero lo instalaremos vía
`pip`:

```sh
pip install websockets
```

Un ejemplo básico se puede basar en un cliente que envía una cadena a un 
servidor, y queda a la espera de recibir un mensaje de respuesta por parte
de éste. 

Desarrollar la parte servidora para dicha aplicación resulta sencillo. Observa
el siguiente código:

```python
#!/usr/bin/env python

import asyncio
import websockets

async def hello(websocket, path):
    name = await websocket.recv()
    print(f"< {name}")

    greeting = f"Hello {name}!"

    await websocket.send(greeting)
    print(f"> {greeting}")

start_server = websockets.serve(hello, "localhost", 8765)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

El paradigma de programación utilizado en este ejemplo (basado en el 
módulo [asyncio](https://docs.python.org/3/library/asyncio.html)) queda fuera
del propósito de la práctica (aunque se invita al alumno a estudiarlo, ya
que aporta importantes ventajas a nivel de sencillez de desarrollo en 
aplicaciones de red). En cualquier caso, el anterior servidor ejecuta
una (co)rutina manejadora `hello` para cada conexión *websocket* establecida; 
además, se cierra dicha conexión cuando dicha (co)rutina finaliza.

Concretamente, las funciones de interés en este caso son:

```python
await websockets.server.serve(ws_handler, host=None, port=None, # ...
```

Crea, incializa y devuelve un objeto *servidor Websocket* asociado al 
host y puerto seleccionados. En un contexto de programación asíncrona (como
el del ejemplo, el servidor finaliza automáticamente al salir de dicho contexto).

Cuando un cliente conecta al host y puerto específicados, se acepta la conexión,
que es tratada por la (co)rutina `ws_handler` (en el ejemplo, `hello`). Antes
de delegar la conexión a la (co)rutina, se lleva a cabo el *handshake* de 
apertura *websocket*.

```python
await recv()
```

Recibe el siguiente mensaje, devolviendo una cadena si el *frame* recibido
es de texto, o un *array* de bytes si es binario.

```python
await send(message)
```

Envía un mensaje. `message` puede er una cadena, o un array de bytes. En 
el primer caso, se envía un *frame* de texto; en el segundo caso, 
un *frame* binario.


A continuación se muestra un ejemplo de cliente *websocket* para interactuar
con el anterior servidor:

```python
#!/usr/bin/env python

import asyncio
import websockets

async def hello():
    uri = "ws://localhost:8765"
    async with websockets.connect(uri) as websocket:
        name = input("What's your name? ")

        await websocket.send(name)
        print(f"> {name}")

        greeting = await websocket.recv()
        print(f"< {greeting}")

asyncio.get_event_loop().run_until_complete(hello())
```

El código en este caso es sencillo, ya que únicamente se basa en la planificación
(ejecución) de una (co)rutina llamada `hello`, que establece una conexión con
un servidor *websocket* vía `connect`, enviando y recibiendo un par de mensajes.

```python
await websockets.client.connect(uri, # ...
```

Conecta con un servidor *websocket* en la URI determinada. La conexión se cierra
al abandonar el contexto asíncrono (es decir, la (co)rutina `hello`).

!!! danger "Tarea opcional"
    Crea un servidor Python que interactue con una versión derivada del cliente ESP32 anterior.