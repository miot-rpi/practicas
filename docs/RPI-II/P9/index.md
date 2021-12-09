# Práctica 9. Node-RED

## Introducción y objetivos

Node-RED es una herramienta de código abierto desarrollada inicialmente por IBM 
y que, estando orientada a flujos de datos, proporciona mecanismos para asociar
dispositivos hardware, APIs y servicios online dentro de un ecosistema IoT. 
Node-RED es una herramienta gráfica, utilizable desde cualquier navegador web, 
que permite la cración y edición de flujos de datos que tomen datos de entrada 
(mediante nodos de entrada), los procesen (mediante nodos de procesamiento) y 
proporcionen salidas (mediante nodos de salida). 
Todos los elementos, incluyendo flujos complejos definidos por el usuario, 
pueden almacenarse en formato JSON para ser importados a continuación en otras 
instalaciones. Node-RED permite la interconexión de elementos software y 
hardware mediante virtualmente cualquier protocolo conocido, facilitando el 
despliegue de infraestructuras IoT.

El desarrollo de la práctica difiere sobre el utilizado en prácticas anteriores. 
En este caso, el presente boletín únicamente incluye información e instrucciones
para la instalación de la herramienta Node-RED en la máquina virtual del curso
(alternativamente, es posible instalar Node-RED en cualquier máquina física), 
así como la propuesta de ejercicio entregable a diseñar e implementar. 
Se proporcionarán las explicaciones y demostraciones necesarias durante la 
sesión de laboratorio para entender los conceptos básicos relativos a la 
instalación y uso básico de la herramienta Node-RED. 

## Instalación y puesta en marcha

Para instalar Node-RED en la máquina virtual del curso, es suficiente con 
utilizar el gestor `npm`:

```sh
sudo npm install -g --unsafe-perm node-red
```

Al finalzar, si todo ha ido bien, deberías observar una salida similar a la
siguiente:

```sh
+ node-red@1.1.0
added 332 packages from 341 contributors in 18.494s
found 0 vulnerabilities
```

En caso de que se muestre un mensaje indicando que la versión de Node.js no es compatible 
y que se requiera al menos una versión 14, es preciso actualizarlo mediante los siguientes
comandos:

```sh
curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
sudo apt-get install -y nodejs
```

Para ejecutar Node-RED, una vez instalado, es posible utilizar la orden
`node-red` desde cualquier terminal. Para detener el proceso, es suficiente
con utilizar `Ctrl-C`:

```sh
$ node-red

Welcome to Node-RED
===================

30 Jun 23:43:39 - [info] Node-RED version: v1.1.0
30 Jun 23:43:39 - [info] Node.js  version: v10.21.0
30 Jun 23:43:39 - [info] Darwin 18.7.0 x64 LE
30 Jun 23:43:39 - [info] Loading palette nodes
30 Jun 23:43:44 - [warn] rpi-gpio : Raspberry Pi specific node set inactive
30 Jun 23:43:44 - [info] Settings file  : /Users/nol/.node-red/settings.js
30 Jun 23:43:44 - [info] HTTP Static    : /Users/nol/node-red/web
30 Jun 23:43:44 - [info] Context store  : 'default' [module=localfilesystem]
30 Jun 23:43:44 - [info] User directory : /Users/nol/.node-red
30 Jun 23:43:44 - [warn] Projects disabled : set editorTheme.projects.enabled=true to enable
30 Jun 23:43:44 - [info] Creating new flows file : flows_noltop.json
30 Jun 23:43:44 - [info] Starting flows
30 Jun 23:43:44 - [info] Started flows
30 Jun 23:43:44 - [info] Server now running at http://127.0.0.1:1880/red/
```

Con el *software* arrancado, es posible acceder al editor Node-RED a través
de la dirección `http://localhost:1880` en cualquier navegador.

Tras arrancarlo, observarás cuatro áreas en el editor:

1. **Barra principal**, en la parte superior, con los botones *Deploy* y 
de *Menú principal*.

2. **Panel de nodos**, en la parte
izquierda, que proporciona acceso directo a todos los nodos
disponibles en Node-RED. Es posible instalar nuevos nodos a través de 
la Paleta de Nodos, disponible a través del menú principal (*Manage Palette*).
Estos nodos pueden ser arrastrados al editor para conformar nuevos
flujos de datos.

3. **Panel de edición o espacio de trabajo**, en la parte central de la pantalla, donde podrás
arrastrar y unir nuevos nodos. Es posible crear nuevos flujos en pestañas
independientes.

4. **Panel de información**, en la parte derecha de la pantalla, donde
destaca el botón *Debug*, mediante el cual veremos la salida de los
nodos de tipo *Debug* en nuestros flujos.

## Ejemplo 1

Se muestra a continuación un breve ejemplo básico de utilización del editor
Node-RED, que incluye el uso de nodos *Inject*, *Debug* y *Function*.

### Nodo *Inject*

El nodo *Inject* permite inyectar mensajes en un flujo, bien pulsando en 
el botón asociado al nodo, o estableciendo un intervalo de tiempo entre
inyecciones. Busca en el panel izquierdo un nodo de tipo *Inject* y 
arrástralo al espacio de trabajo. En el panel de información podrás
consultar los datos asociados al nodo, así como información de ayuda para 
utilizarlo.

### Nodo *Debug*

El nodo *Debug* permite que cualquier mensaje entrante se muestre en 
el panel de depuración, en la parte derecha de la pantalla. Por defecto, 
únicamente muestra el *payload* del mensaje, aunque puede configurarse
para mostrar el objeto completo. Arrastra un nodo de tipo *Debug* al
espacio de trabajo.

### Unión y despliegue (*Deploy*)

Conecta los nodos *Inject* y *Debug* estableciendo un enlace (*Wire*) entre
ambos. Despliega el flujo usando el botón *Deploy* en la barra principal de
Node-RED. Esto desplegará el fujo en el servidor. 

Selecciona la opción *Debug* en el panel de información, y presiona el
botón del nodo *Inject*. Deberías ver números aparecer en el panel. Por
defecto, el nodo *Inject* emite el número de milisegundos desde
el 1 de enero de 1970 como *payload*.

Modifica (temporalmente) el nodo *Debug* para que muestre todo el mensaje
en lugar del *payload*. Despliega de nuevo el flujo y observa las diferencias.

Vuelve a configurar el nodo *Debug* tal y como estaba cuando lo insertaste.

### Nodo *Function*

El nodo *Function* permite procesar el mensaje de entrada mediante una
función JavaScript. Borra el *Wire* existente y añade un nodo *Function*
entre los nodos *Inject* y *Debug*.

Haz doble clic en el nuevo nodo para abrir el diálogo de edición. Copia el
siguiente código en el campo *Function*:

```javascript
// Create a Date object from the payload
var date = new Date(msg.payload);
// Change the payload to be a formatted Date string
msg.payload = date.toString();
// Return the message so it can be sent on
return msg;
```

Clica en *Done* y despliega el flujo. Observa que, ahora, los mensajes
de depuración muestran marcas de tiempo en formato visible. Ten en cuenta
que un nodo siempre recibe un mensaje (*msg*) de entrada y devuelve un 
mensaje (*msg*) de salida. Ambos objetos contienen, por convenio, un 
campo *payload*.

Para más información sobre el uso de funciones y trabajo con mensajes, 
incluyendo múltiples valores de retorno y trabajo con valores globales a 
todo el entorno, se recomienda estudiar la siguiente documentación:

* **Trabajo con mensajes**: [enlace a documentación](https://nodered.org/docs/user-guide/messages).
* **Trabajo con funciones**: [enlace a documentación](https://nodered.org/docs/user-guide/writing-functions).

## Ejemplo 2

### Nodo *Inject*

En el anterior ejemplo, vimos cómo crear un nodo *Inject* para activar el
flujo cuando se pulsaba su botón asociado. En este ejemplo, vamos a configurar
el nodo *Inject* para que active el flujo en intervalos regulares. 

Arrastra un nuevo nodo *Inject* en el espacio de trabajo. Clica en él dos
veces y, en el diálogo de edición, usala opción *Repeat interval*, fijándolo
en un intervalo regular. Cierra el diálogo de edición.

### Nodo *HTTP Request*

El nodo de tipo *HTTP Request* puede utilizarse para descargar una página web o
recurso HTTP. Añade uno al espacio de trabajo, y edítalo para que su propiedad
*URL* apunte a `https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/significant_month.csv`.

Esta URL es un repositorio de terremotos en el último mes, pulicado por 
el organismo oficial correspondiente, devueltos en formato CSV.

### Nodo *CSV*

Añade un nuevo nodo *CSV* y edita sus propiedades. Activa la opción 
*First row contains column names* y finaliza la edición.

### Nodo *Debug* y cableado

Añade un nodo *Debug* y une los nodos creados:

* Conecta la salida del nodo *Inject* a la entrada del nodo *HTTP Request*.
* Conecta la salida del nodo *HTTP Request* a la entrada del nodo *CSV*.
* Conecta la salida del nodo *CSV* a la entrada del nodo *Debug*.

### Nodo *Switch*

Añade un nodo *Switch* al espacio de trabajo. Edita sus propiedades y configuralo
para comprobar la propiedad `msg.payload.mag`, usando la operación `>=` sobre
un valor numérico y el valor `6.2`, por ejemplo.

Añade un segundo *Wire* entre el nodo *CSV* y el nodo *Switch*.

### Nodo *Change*

Añade un nodo *Change*, conectado a la salida del nodo *Switch*. Configuralo para
establecer el valor de `msg.payload` a `ALARMA`.

### Nodo *Debug*

Añade un nodo *Debug* y despliega el flujo.

En el panel de *Debug*, deberás obsevar, para cada activación del nodo 
*Inject*, una salida similar a esta:

```sh
msg.payload : Object
{"time":"2017-11-19T15:09:03.120Z","latitude":-21.5167,"longitude":168.5426,"depth":14.19,"mag":6.6,"magType":"mww","gap":21,"dmin":0.478,"rms":0.86,"net":"us","id":"us2000brgk","updated":"2017-11-19T17:10:58.449Z","place":"68km E of Tadine, New Caledonia","type":"earthquake","horizontalError":6.2,"depthError":2.8,"magError":0.037,"magNst":72,"status":"reviewed","locationSource":"us","magSource":"us"}
```

Puedes clicar en la pequeña flecha a la izquierda de cada propiedad para 
expandirla y examinar sus contenidos. 

Si existe cualquier terremoto de magnitud mayor a `6.2`, observarás una
salida adicional:

```sh
msg.payload : string(6)
"ALARMA"
```

Para más información sobre los nodos básicos en Node-RED, puedes consultar:

* **The Core Nodes**: [enlace a documentación](https://nodered.org/docs/user-guide/nodes).

## Cliente MQTT y despliegue de un panel de control

El nodo *MQTT in* permite realizar suscripciones a *topics* determinados en
*brokers* MQTT. 

Arrastra un nuevo nodo *MQTT in* en tu espacio de trabajo y configura el 
*broker* asociado a *localhost*, puerto por defecto. Establece un *topic*
de interés. Conecta un nodo *Debug* y despliega el flujo.

Desde tu consola, publica mensajes vía *mosquitto_pub* y comprueba que, efectivamente,
son visibles en Node-RED.

A continuación, crearemos un pequeño panel de control para la representación
gráfica del valor publicado. En primer lugar, deberás instalar el 
nodo `node-red-dashboard` desde el menú principal, opción 
*Manage palette*. Tras su instalación, verás que aparecen nuevos nodos
en el panel de nodos; éstos nos permitirán diseñar e implementar un panel
de control básico basado en *Widgets*.

Arrastra un nodo de tipo *Gauge* al espacio de trabajo, y configura sus valores
por defecto. Conecta la salida de tu nodo *MQTT in* a la entrada del nuevo
nodo *Gauge*.

Despliega el flujo, y navega hasta `http://localhost:1880/ui`, donde deberás
observar el panel de control con el *widget* que has creado. Interactúa con
él publicando mensajes vía MQTT.

Para más información sobre el despliegue de paneles de control, puedes
consultar:

* **Node-Red-Dashboard**: [enlace a documentación](https://flows.nodered.org/node/node-red-dashboard).

## Documentación adicional

Las guías de usuario oficiales de Node-RED son un buen punto de partida para 
profundizar en el uso de la infraestructura. De entre ellas, la parte más 
importante para comenzar es la que introduce los conceptos básicos de
Node-RED, incluyendo el trabajo con nodos, flujos, contexto (importante
para trabajar con valores globales y compartidos por todos los nodos en un 
flujo, por ejemplo), mensajes, *wires*, etc.:

* **Node-RED Concepts**: [enlace a documentación](https://nodered.org/docs/user-guide/concepts).

La guía *Node-RED Guide* contiene interesante documentación adicional/avanzada
tanto en el despliegue de flujos como en el uso de paneles de control locales
o usando servicios remotos (por ejemplo, *Freeboard*):

* **Node-RED Guide**: [enlace a documentación](http://noderedguide.com).

## Ejercicio entregable

Se pide estudiar la documentación asociada a Node-RED, tanto en su web
oficial como en la guía de programación *Node-RED Guide* (específicamente sus 
cuatro primeras partes. 
Junto con la explicación proporcionada por el profesor y los anteriores 
ejemplos, este estudio permitirá el desarrollo de la práctica.

!!! danger "Tarea entregable"
    La práctica consiste en el diseño y desarrollo de un sistema basado en 
    flujos de datos construido sobre Node-RED, que implemente un mecanismo de 
    monitorización de parámetros ambientales (por ejemplo, temperatura) y 
    notificación (alarmas) ante ciertas circunstancias (por ejemplo, superar una 
    determinada temperatura fijada como umbral).  

    Los alumnos diseñarán el sistema y lo implementarán, cumpliendo las siguientes premisas:

    * **(2 puntos)**. El sistema utilizará, **al menos**, un dispositivo externo 
    (ESP32, SensorTag, Teléfono Móvil, ...) para la recolección de datos. 
    Se valorará el uso de más de un dispositivo.

    * **(2 puntos)**. El sistema depositará o interactuará con, **al menos**, 
    un sistema externo (servidor de correo, Twitter, Telegram, IBM Bluemix, ...). 

    * **(2 puntos)**. El sistema depositará los datos observados en algún medio 
    persistente (Base de Datos no relacional, ficheros, ...) para permitir 
    su posterior análisis y en un panel de control.

    * ** (2 puntos)**. El sistema actuará como un sistema de alarma únicamente 
    ante ciertas condiciones de entrada (por ejemplo, al recibir un valor desde 
    un sensor superior a un umbral establecido; dicho valor podría, por ejemplo,
    configurarse vía MQTT o a través de un panel de control).

    * **(2 puntos)**. El sistema utilizará, al menos, un tipo de nodo no 
    instalado por defecto en la instalación básica de Node-RED.


    Se entregará el fichero o ficheros JSON que describen los nodos, así como 
    una breve memoria que describa el sistema diseñado y el trabajo realizado, 
    haciendo hincapié en las dificultades encontradas y aquellos aspectos que se 
    consideren de interés por parte del alumno/a. 

