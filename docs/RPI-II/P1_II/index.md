# Práctica 1 (apéndice). Introducción a Node-RED

## Objetivos

* Familiarizarse con la herramienta Node-RED para gestión de flujos, que será utilizada
  durante el resto de prácticas como herramienta rápida de despliegue de aplicaciones IoT.
* Desarrollar esquemas básicos de sistemas cliente/servidor TCP y UDP
  utilizando Node-RED.

## Introducción

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

El presente boletín únicamente incluye información e instrucciones
para la instalación de la herramienta Node-RED en la máquina virtual del curso
(alternativamente, es posible instalar Node-RED en cualquier máquina física), 
así como la propuesta de ejercicio entregable a diseñar e implementar. 
Se proporcionarán las explicaciones y demostraciones necesarias durante la 
sesión de laboratorio para entender los conceptos básicos relativos a la 
instalación y uso básico de la herramienta Node-RED. 

## Instalación y puesta en marcha 

Las presentes instrucciones son válidas para cualquier distribución basada en
Debian (incluida Ubuntu). Para otras distribuciones o sistemas operativos, consultad
la documentación oficial de Node-RED.

Para instalar Node-RED en la máquina virtual del curso, es suficiente con 
seguir los siguientes pasos.

En primer lugar, si no lo están ya, instala los requisitos básicos del paquete:

```sh
sudo apt install build-essential git curl
```

La siguiente línea descarga y ejecuta un script que realizará toda la instalación de dependencias (desinstalando versiones antiguas si es necesario) por ti:

```sh
bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)
```

Para ejecutar Node-RED, una vez instalado, es posible utilizar la orden
`node-red-start` desde cualquier terminal. Para detener el proceso, es suficiente
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

!!! note "Tarea 1.3bis"
    Se pide estudiar la documentación asociada a Node-RED, tanto en su web
    oficial como en la guía de programación *Node-RED Guide* (específicamente sus 
    cuatro primeras partes). 
    Junto con la explicación proporcionada por el profesor y los anteriores 
    ejemplos, este estudio permitirá el desarrollo de la tarea. Se sugiere experimentar con los
    nodos TCP (in/out/request) y sus correspondientes versiones UDP.

    La tarea consiste en el diseño y desarrollo de un sistema cliente/servidor similar al 
    desarrollado en la tarea 1.3, pero que, en esta ocasión, despliegue sus componentes 
    utilizando exclusivamente Node-RED. El objetivo final de la tarea es que sea posible
    interactuar entre un cliente/servidor desarrollado en C y un servidor/cliente desarrollado
    en Node-RED. Se aceptará simplificar el protocolo de aplicación desarrollado en la tarea 1.3
    si ello facilita el dearrollo de la tarea.  Puede ser de ayuda el estudio del nodo 
    [buffer-parser](https://flows.nodered.org/node/node-red-contrib-buffer-parser).

    Se entregará el fichero o ficheros JSON que describen los nodos, así como 
    una breve memoria que describa el sistema diseñado y el trabajo realizado, 
    haciendo hincapié en las dificultades encontradas y aquellos aspectos que se 
    consideren de interés por parte del alumno/a. 

