# Práctica 7. El protocolo CoAP

# Objetivos

El objetivo de esta práctica es realizar una introducción al protocolo
CoAP, uno de los más extendidos a día de hoy para llevar a cabo comunicaciones
M2M. Los objetivos didácticos de la práctica son:

* Entender la estructura cliente-servidor del protocolo CoAP.
* Comprender los mensajes utilizados para establecer una comunicación CoAP, 
tanto a nivel de sintaxis como de semántica.
* Modificar una aplicación cliente/servidor ejemplo basada en `libcoap` que realice una comunicación 
sencilla a través del protocolo CoAP en un entorno Linux.
* Estudiar el componente `libcoap` en ESP-IDF para reproducir el comportamiento
del servidor CoAP para ofrecer su fucionalidad desde un ESP32. 


# Instalación y requisitos previos

En esta práctica realizaremos un estudio del protocolo CoAP utilizando una 
implementación ya desarrollada para un sistema cliente/servidor que hace uso
del protocolo CoAP, distribuida a través de la biblioteca [*libcoap*](https://libcoap.net). 
Nótese que se trata de una práctica introductoria, ya
que el protocolo CoAP se utilizará, en sucesivas prácticas, para dar soporte
a protocolos de más alto nivel (principalmente LWM2M).

El objetivo principal de la práctica es, pues, introducir a alto nivel las 
características de CoAP como protocolo de capa de aplicación, así como ser
capaces de interactuar con un servidor existente a través de herramientas
ya desarrolladas. De forma adicional, se estudiará la posibilidad de 
implementar un servidor CoAP en el ESP32.

## Instalación de requisitos adicionales y *libcoap*

En primer lugar, instalemos los prerequisitos necesarios para hacer funcionar
{\tt libcoap}. Para ello, en la máquina virtual, ejecutaremos las siguientes
órdenes:

```sh
sudo apt-get update
sudo apt-get install libtool
```

Procedemos ahora con la instalación de {\tt libcoap}. Para ello, descarga la última
versión de la biblioteca desde la página web del proyecto, descomprímelo y pasa a la
fase de compilación e instalación:

```sh
sh autogen.sh
./configure --enable-examples --enable-dtls --with-openssl --disable-documentation
make
make install
```

Si no hay ningún error, *libcoap* se habrá instalado con éxito. Será de especial
interés para nosotros la instalación de programas servidor (*coap-server*)
y cliente (*coap-client*) de ejemplo en el directorio **examples**.

!!! note "Tarea"
    Ejecuta los programas servidor y cliente CoAP del directorio `examples`. 
    Estudia sus opciones y parámetros de configuración. 
    ¿En qué puertos y bajo qué protocolos escucha el servidor CoAP tras su
    arranque?

# Intercambio de mensajes CoAP

!!! danger "Tarea entregable"
    En la presente sección se proponen
    distintos intercambios de mensajes CoAP entre el cliente y el servidor de ejemplo
    proporcionados como parte de la instalación de *libcoap*. Para cada uno de ellos,
    se pide un estudio básico de los paquetes intercambiados, haciendo especial hincapié
    en la pila de protocolos utilizados, contenido de los paquetes y número de paquetes
    intercambiados. Este estudio, incluyendo capturas y comentarios adicionales, 
    conformará el entregable asociado a la práctica.

## Arranque del servidor CoAP

En primer lugar, realizaremos un intercambio básico de mensajes CoAP entre el
cliente y el servidor. Para ello, abriremos dos terminales desde las que ejecutaremos,
respectivamente, el servidor y el cliente. 

!!! note "Tarea"
    Investiga las opciones disponibles
    en el cliente y servidor con respecto a la cantidad de mensajes de depuración a mostrar.
    Ejecuta el servidor CoAP con suficiente nivel de detalle en los mensajes de depuración.

Una vez arrancado el servidor, ejecuta la orden correspondiente desde línea de 
órdenes para averiguar qué puertos ha abierto, y por tanto cómo nos podemos comunicar
con él. Averigua si estos puertos son bien conocidos *well-known*, valor menor a 1024) y, en su caso,
cómo pueden modificarse.

## Obtención de información del servidor (*Resource Discovery*)

En primer lugar, obtendremos la información sobre los recursos disponibles en el
servidor CoAP. Para ello, realizaremos una petición `GET` sobre el recurso
`/.well-known/core` del servidor. Esta transacción nos devolverá los recursos
disponibles en el mismo, así como algunas características adicionales.

!!! note "Tarea"
    ¿Qué recursos están disponibles
    en el servidor? Estudia el código fuente del mismo para observar la correlación entre
    los recursos descubiertos y los programados en el código. Averigua el significado 
    de los atributos *rt*, *ct*, *if* y *title.

## Obtención de información desde recursos

Utilizando el cliente CoAP proporcionado, resulta sencillo realizar consultas para 
obtener datos desde el servidor. Para ello, utilizaremos la acción (verbo) {\tt GET},
seguido del recurso a consultar y, opcionalmente, de una consulta concreta. 

!!! note "Tarea"
    Consulta la marca de tiempo
    proporcionada por el servidor en modo legible (por ejemplo, `Dec 13 14:20:43`), y
    también en forma de *ticks* de reloj, utilizando la consulta adecuada. ¿Qué valor 
    de retorno (código) incluye la respuesta CoAP si el proceso ha tenido éxito?

## Modificación de recursos

Al igual que con el verbo GET, es posible realizar modificaciones en el servidor 
utilizando el verbo PUT. Consulta la ayuda del cliente proporcionado para observar
algún ejemplo que dé soporte a esta funcionalidad.

!!! note "Tarea"
    Modifica la marca de tiempo que proporciona el servidor CoAP. 
    ¿Qué valor de retorno (código) incluye la respuesta CoAP si el proceso ha tenido éxito?

## Eliminación y creación de recursos

Es posible eliminar un determinado recurso (en el ejemplo, el temporizador), utilizando
el verbo `DELETE`. Investiga cómo hacerlo desde el cliente proporcionado.

!!! note "Tarea"
    Elimina el recurso *time* del servidor y, a continuación, modifica la marca de tiempo mediante una orden
    `PUT`}. ¿Qué valores de retorno (código) se devuelven en ambos casos?}

## Suscripción (observación) de recursos

Es posible suscribirse a los cambios en el valor de un recurso utilizando la opción
`-s` del cliente. 

!!! note "Tarea"
    Activa la observación sobre el recurso *time* del servidor y analiza tanto la
    frecuencia de respuesta como el intercambio de mensajes producido (a través
    de *Wireshark*). ¿Se producen peticiones periódicas usando `GET`?

## CoAP sobre TCP

Alternativamente, CoAP puede funcionar utilizando el protocolo de capa de transporte
TCP. En este caso, como es lógico, se establecerá una conexión entre cliente y servidor
previa a cualquier intercambio de datos.

!!! note "Tarea"
    Fuerza el uso de TCP en el cliente mediante la opción correspondiente y estudia las principales diferencias entre
    los mensajes intercambiados con respecto al uso de UDP.
    ¿Cuál es la eficiencia al utilizar UDP y TCP como protocolos de transporte para CoAP?

# Tareas entregables

!!! danger "Tarea entregable"
    Deberás entregar una memoria en la que
    se incida en detalles observados y aprendidos acerca del protocolo CoAP, con
    especial atención a las capturas obtenidas a través de Wireshark.

!!! Danger "Tarea entregable"
    Estudia el código del servidor proporcionado, especialmente de la función
    `init_resources`, y añade un nuevo recurso llamado *temperature*. 
    Este recurso aceptará dos consultas distintas: *?celsius* (consulta por defecto)
    devolverá el valor de temperatura expresado en grados centígrados, mientras que
    *\tt fahrenheit* devolverá la temperatura en grados Fahrenheit. En este caso, el valor
    de temperatura se obtendrá directamente a través de un número aleatorio, pero se
    valorará su obtención a partir de un sensor real.

!!! Danger "Tarea entregable"
    ESP-IDF incluye un port de *libcoap*. El ejemplo 
    `examples/protocols/coap_server` implementa un servidor CoAP básico, con 
    un sólo recurso (puedes consultarlo tú mismo/a obteniendo la información
    del recurso `/well-known/core`). Analiza el código y observa que la 
    biblioteca *libcoap* se utiliza de forma exacta a como has estudiado
    en el código del servidor de ejemplo. Se pide modificar el *firmware*
    para dar soporte al recurso *time* de forma idéntica (con la misma 
    semántica) que la utilizada en el *host*.


