# Práctica 4. Protocolos básicos de capa de aplicación. Websockets

## Objetivos

- Conocer el módulo Python *websockets* para desarrollar sistemas básicos
  cliente/servidor utilizando *websockets*.
- Estudar el intercambio de mensajes entre un cliente y un servidor 
  *websockets*, tanto en la fase de *handshake* como de intercambio de datos.
- Conseguir interactuar con un servidor *websockets* utilizando un navegador
  web como cliente.
- Estudiar mecanismos de mantenimiento y publicación de estado a clientes 
  conectados, típicos en un entorno IoT.
- Estudiar el componente *websockets client* en ESP-IDF, y desarrollar un 
  *firmware* básico que interactúe con un servidor Python.
- Introducir la gestión de objetos JSON en ESP-IDF.

Los ficheros necesarios para completar la práctica pueden descargarse
[aquí](https://drive.google.com/file/d/1fVWqfg5ZtuAPt-ZLdlt9M9DobHm6T0Jv/view?usp=sharing).

## Ejemplo básico: sistema cliente/servidor usando Websockets en Python (´client1.py, server1.py`)

En primer lugar, introducimos el uso del módulo Python
`websockets`, que proporciona
toda la funcionalidad necesaria para desarrollar sistemas cliente/servidor
utilizando *websockets*.

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
de éste, tal y como hemos visto en otras prácticas. 

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

!!! danger "Tarea entregable"
    Ejecuta el servidor en una terminal de tu máquina virtual, y a continuación
    el cliente en otra. Analiza el tráfico intercambiado y responde a las siguientes
    preguntas:

    1. ¿En qué protocolo de capa de transporte se basa la comunicación vía Websockets?
    2. En la fase de *handshake*, ¿qué peticiones HTTP se intercambian? Analiza
    sus emisores y destinatarios, e investiga el cometido principal de cada uno de
    los campos de sus encabezados (fíjate principalmente en el campo *Upgrade* y 
    los campos específicos para Websockets).
    3. En la fase de intercambio de datos, ¿qué *opcode* se especifica en el encabezado
    de cada paquete? ¿Por qué?
    4. ¿Se envían los datos en claro o cifrados?
    5. ¿Qué *opcode* se añade en los mensajes de cierre de conexión?

## Interactuando con un navegador web (`client_for_web.html`, `server_for_web.py`)

Aunque fuera del interés de esta práctica, es conveniente observar una de las
ventajas de *websockets*: el envío asíncrono bidireccional de información, y 
observarlo a través de un navegador web convencional (la mayoría de navegadores
modernos soportan *websockets* a través de *scripts Javascript*).

En este caso, simplemente observa la interacción de un servidor *websocket* 
que envía mensajes que incluyen la hora actual separados un número aleatorio
de tiempo entre ellos:

```python
#!/usr/bin/env python

# WS server that sends messages at random intervals

import asyncio
import datetime
import random
import websockets

async def time(websocket, path):
    while True:
        now = datetime.datetime.utcnow().isoformat() + "Z"
        await websocket.send(now)
        await asyncio.sleep(random.random() * 3)

start_server = websockets.serve(time, "127.0.0.1", 5678)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

Con un cliente (una página HTML) que establece la conexión vía *websockets*,
y muestra en la misma un elemento de texto con la marca de tiempo recibida 
tras la recepción de cada mensaje:

```html
<!DOCTYPE html>
<html>
    <head>
        <title>WebSocket demo</title>
    </head>
    <body>
        <script>
            var ws = new WebSocket("ws://127.0.0.1:5678/"),
                messages = document.createElement('ul');
            ws.onmessage = function (event) {
                var messages = document.getElementsByTagName('ul')[0],
                    message = document.createElement('li'),
                    content = document.createTextNode(event.data);
                message.appendChild(content);
                messages.appendChild(message);
            };
            document.body.appendChild(messages);
        </script>
    </body>
</html>
```

!!! note "Tarea"
    Ejecuta el servidor en tu máquina virtual y, tras guardar el código fuente
    del cliente en un fichero `cliente.html`, ábrelo con un navegador. Observa
    como la página se actualiza a medida que recibe mensajes a través del 
    socket. Si quieres, puedes observar el intercambio de mensajes. ¿Qué ocurre
    si, en otra pestaña, vuelves a abrir la página cliente?

## Un ejemplo más complejo: sincronización entre múltiples clientes (`server2.py`, `client.html`)

Un servidor *websocket* puede recibir eventos desde distintos clientes, procesarlos
para, por ejemplo, mantener actualizado un estado a nivel de aplicación, y 
sincronizar dicho estado entre todos los clientes conectados, enviándoles mensajes
de forma asíncrona a través del socket bidireccional, a modo de "notificaciones
*push*".

A continuación, se muestra el código de un servidor que mantiene dos tipos 
de información de estado siempre actualizada: el valor de un contador
(`STATE`), que puede
ser modificado por los clientes conectados sumando o restando uno a su valor
a través de mensajes enviados por el *socket*; y el número de clientes 
conectados (`USERS`).

```python
#!/usr/bin/env python

import asyncio
import json
import logging
import websockets

logging.basicConfig()

STATE = {"value": 0}

USERS = set()


def state_event():
    return json.dumps({"type": "state", **STATE})


def users_event():
    return json.dumps({"type": "users", "count": len(USERS)})


async def notify_state():
    if USERS:  # asyncio.wait doesn't accept an empty list
        message = state_event()
        await asyncio.wait([user.send(message) for user in USERS])


async def notify_users():
    if USERS:  # asyncio.wait doesn't accept an empty list
        message = users_event()
        await asyncio.wait([user.send(message) for user in USERS])


async def register(websocket):
    USERS.add(websocket)
    await notify_users()


async def unregister(websocket):
    USERS.remove(websocket)
    await notify_users()


async def counter(websocket, path):
    # register(websocket) sends user_event() to websocket
    await register(websocket)
    try:
        await websocket.send(state_event())
        async for message in websocket:
            data = json.loads(message)
            if data["action"] == "minus":
                STATE["value"] -= 1
                await notify_state()
            elif data["action"] == "plus":
                STATE["value"] += 1
                await notify_state()
            else:
                logging.error("unsupported event: {}", data)
    finally:
        await unregister(websocket)


start_server = websockets.serve(counter, "localhost", 6789)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

Observa el código del servidor. 

El manejador `counter` procesa cada conexión
entrante, registrando a su entrada a un nuevo cliente en el sistema (`register`)
y desregistrándolo antes de finalizar (`unregister`). Ante cada registro
o desregistro, se notifica a los usuarios este hecho, enviando a cada
cliente un pequeño texto en formato JSON cuyo contenido es:

```json
{"type": "users", "count": usuarios}
```

Es decir, un mensaje con dos campos (veremos JSON en la próxima práctica):
campo `type`, con valor fijo `users`, y campo `count`, con un valor entero 
que indica el número de clientes conectados.

A continuación, para cada mensaje recibido a través del socket, éste se procesa,
esperando también un fichero JSON con la acción que el cliente solicita (sumar
o restar 1 al contador), por ejemplo:

```json
 {"action": "minus"}
```

o

```json
 {"action": "plus"}
```

En función de la acción solicitada, el servidor actualiza el valor de `STATE`,
y envía (rutina `notify_state`) dicho valor actualizado a TODOS los clientes
conectados mediante un pequeño mensaje de texto en formato JSON:

```json
{"type": "state", "value": VALOR}
```

La parte cliente sigue la misma filosofía, utilizando de nuevo el navegador
como plataforma para visualizar la interacción con el cliente. El código HTML
que puedes abrir en tu navegador es el siguiente:

```html
<!DOCTYPE html>
<html>
    <head>
        <title>WebSocket demo</title>
        <style type="text/css">
            body {
                font-family: "Courier New", sans-serif;
                text-align: center;
            }
            .buttons {
                font-size: 4em;
                display: flex;
                justify-content: center;
            }
            .button, .value {
                line-height: 1;
                padding: 2rem;
                margin: 2rem;
                border: medium solid;
                min-height: 1em;
                min-width: 1em;
            }
            .button {
                cursor: pointer;
                user-select: none;
            }
            .minus {
                color: red;
            }
            .plus {
                color: green;
            }
            .value {
                min-width: 2em;
            }
            .state {
                font-size: 2em;
            }
        </style>
    </head>
    <body>
        <div class="buttons">
            <div class="minus button">-</div>
            <div class="value">?</div>
            <div class="plus button">+</div>
        </div>
        <div class="state">
            <span class="users">?</span> online
        </div>
        <script>
            var minus = document.querySelector('.minus'),
                plus = document.querySelector('.plus'),
                value = document.querySelector('.value'),
                users = document.querySelector('.users'),
                websocket = new WebSocket("ws://127.0.0.1:6789/");
            minus.onclick = function (event) {
                websocket.send(JSON.stringify({action: 'minus'}));
            }
            plus.onclick = function (event) {
                websocket.send(JSON.stringify({action: 'plus'}));
            }
            websocket.onmessage = function (event) {
                data = JSON.parse(event.data);
                switch (data.type) {
                    case 'state':
                        value.textContent = data.value;
                        break;
                    case 'users':
                        users.textContent = (
                            data.count.toString() + " user" +
                            (data.count == 1 ? "" : "s"));
                        break;
                    default:
                        console.error(
                            "unsupported event", data);
                }
            };
        </script>
    </body>
</html>
```

Observa cómo el *script* envía mensajes de suma o resta en formato JSON acorde
al esperado por el servidor, y procesa los mensajes de entrada actualizando
la información mostrada en pantalla recibida acerca del valor del contador
actualizado y número de usuarios.

!!! note "Tarea"
    Ejecuta el servidor en tu máquina virtual, y múltiples clientes en distintas
    ventanas/pestañas del navegador (con ventanas lo verás mejor). Interactúa
    desde un cliente aumentando o reduciendo el valor del contador, y observa
    cómo dicho valor es actualizado (a través del servidor) en el resto de
    clientes abiertos. Conecta y desconecta nuevos clientes y observa también
    como el campo correspondiente en la página web se actualiza correctamente. 
    Si quieres, puedes analizar el tráfico *Websockets* generado vía 
    Wireshark.

## Websockets en el ESP32

El soporte a nivel de cliente para el protocolo *websockets* está integrado
en ESP-IDF a travésd el componente *websocket client*, cuya documentación
puede consultarse a través de este [enlace](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/protocols/esp_websocket_client.html#).

El componente *websocket client* ofrece soporte para el protocolo *websocket*
sobre TCP y también, opcionalmente, sobre TLS. Como todos los componentes
en ESP-IDF, el componente *websocket* emite eventos que pueden ser tratados
por parte de la aplicación, entre los cuales destacan:

* `WEBSOCKET_EVENT_CONNECTED`: se emite una vez el cliente se ha conectado
  al servidor, sin intercambio de datos.
* `WEBSOCKET_EVENT_DISCONNECTED`: se emite en el instante de la desconexión
  entre cliente y servidor.
* `WEBSOCKET_EVENT_DATA`: se emite al recibir datos desde el servidor.

Este último evento es de especial interés para nosotros, ya que accarrea la
construcción de una estructura de tipo `esp_websocket_event_data_t` en la que
se almacena el mensaje recibido desde el servidor (tanto en sus campos de
control como de datos). Algunos campos de interés dentro de la estructura son:

- `const char * data_ptr`: puntero a los datos recibidos (*payload*).
- `data_len`: tamaño (en bytes) de los datos recibidos.
- `op_code`: código de operación asociado al mensaje recibido.

La documentación del componente ofrece información sobre campos adicionales,
de menor interés para nosotros.

Observemos el código de una posible función manejadora de eventos del componente
*websocket*:

```c
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x08 && data->data_len == 2) {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
        } else {
            ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        }
        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}
```

Observa el código. En función del parámetro `event_id`, el manejador toma
un camino de ejecución u otro. Centrémonos en la recepción de un evento de
tipo `ẀEBSOCKET_EVENT_DATA`; a través de los distintos campos de la estructura
de información recibida (`event_data`), es posible:

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
websocket_cfg.uri = "ws://localhost:123";
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
int esp_websocket_client_send(esp_websocket_client_handle_tclient, const char *data, int len, TickType_t timeout)

esp_websocket_client_send_bin(esp_websocket_client_handle_tclient, const char *data, int len, TickType_t timeout)
```

No existen funciones de recepción, ya que ésta es implícita y se notifica vía
eventos.

### Ejemplo básico: cliente *echo*

Veremos en primer lugar un ejemplo completo de cliente ejecutado sobre
el ESP32. En este punto, configura, compila, flashea y monitoriza el ejemplo
`examples/protocols/websockets`.

El ejemplo simplemente conecta con un servidor *echo* *Websockets* en la nube
(disponible en `ws://websockets.org`). Dicho servidor simplemente espera, por
parte de cada cliente, el envío a través de la conexión de una cadena, 
respondiendo con la misma cadena en sentido contrario, siempre usando el 
mismo *socket*.

!!! note "Tarea"
    Observa el código del ejemplo y su ejecución. Determina cuál es el
    funcionamiento del ejemplo, y comprueba que los fragmentos de código 
    anteriores tienen su función dentro del código completo. ¿Cómo implementa
    el programa la espera limitada en tiempo si no se recibe ningún paquete
    tras cierto período?

## Ejercicio entregable: Comunicación asíncrona

El objetivo del ejercicio entregable es conseguir que el ESP32 se comunique con
el servidor Python que se probó en la sección anterior, y que implementaba 
comunicación bidireccional para mantener y difundir el estado interno (contador
y número de clientes conectados) entre todos los clientes conectados.

Para ello, se pide modificar el ejemplo de cliente *echo* para que:

- El cliente conecte con el servidor Python especificando su IP y puerto.
- El cliente sea cien por cien pasivo, es decir, no envíe nunca mensajes 
al servidor.
- La función de manejo de paquetes recibidos trate de forma especial el tipo de
mensajes esperado por parte del servidor. Recuerda que se pueden recibir dos
tipos de mensajes de texto:
  - Mensajes de estado:
```json
{"type": "users", "count": usuarios}
```
  - Mensajes de usuarios:
```json
{"type": "state", "value": VALOR}
```
Observa que ambos mensajes, pese a ser recibidos como texto, corresponden con
una representación JSON de la información. Para tratarla desde ESP-IDF, 
puedes hacer uso del componente cJSON del *framework*. Por ejemplo, para 
tratar un mensaje de entrada de tipo "state", podríamos añadir la siguiente
secuencia de código en nuestro manejador:

```c
#include "cJSON.h"

// ...

if( data->op_code == 1 ) { // Text frame only.
  cJSON *root = cJSON_Parse((char*)data->data_ptr);
  char *type = cJSON_GetObjectItem(root,"type")->valuestring;
  ESP_LOGI(TAG, "type=%s",type);

  int field = 0;

  if( strcmp( type, "state" ) == 0) {
    field = cJSON_GetObjectItem(root,"value")->valueint;
    ESP_LOGI(TAG, "value=%d",field);
  }
}
```

!!! danger "Tarea entregable"
    Modifica el *firmware* de ejemplo *websockets* para que pueda comunicarse
    en modo lectura con el servidor Python que mantiene y publicita estado, cuyo
    código se te proporciona. El programa ESP-IDF, al menos, mostará por pantalla
    un mensaje con los datos asociados cada vez que se reciban paquetes de tipo
    texto (*state* o *users*). También mostrará un mensaje cada vez que el 
    servidor envíe un mensaje de tipo *ping* o *pong* (para ello, consulta
    el RFC que describe el protocolo para determinar el *opcode* asociado).

    Para comprobar el funcionamiento de la solución, arranca el servidor y al
    menos dos clientes web. Cuando arranques el ESP32, ambos deberán incrementar
    el número de clientes reportado, en respuesta al mensaje enviado por el 
    servidor. Cuando cualquiera de los clientes web incremente el valor del 
    contador, el ESP32 recibirá un mensaje con el valor actualizado, del mismo
    modo que cuando cierres uno de los navegadores web.

!!! note "Tarea opcional"
    Modifica el código para que el cliente, periódicamente, envíe un mensaje
    de petición de suma o resta siguiendo las especificaciones y tipos de
    mensaje que se explicaron anteriormente.
