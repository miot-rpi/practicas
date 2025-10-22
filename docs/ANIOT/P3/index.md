# Práctica 3. Programación con tareas y eventos en ESP-IDF

## Objetivos

El objetivo  de esta práctica es conocer los mecanismos para la gestión de tareas
que ofrece FreeRTOS, concretamente en su porting  ESP-IDF.

Trabajaremos los siguientes aspectos del API de ESP-IDF:

* Familiarizarse con la API de *tareas* y *eventos* en ESP/IDF.
* Comunicación y sincronización de tareas mediante colas.

## Material de consulta
Para ver los detalles de cada aspecto de esta práctica se recomienda la lectura de los siguientes enlaces:

* [API de ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html)
* [Documentación oficial de FreeRTOS](https://www.freertos.org/Documentation/00-Overview)
* [Documentación del componente console](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/console.html)


## Introducción

Al desarrollar código para sistemas empotrados, como nuestro nodo basado en ESP32, es habitual organizar la aplicación en torno a diferentes tareas que se ejecutan de forma concurrente. Habrá tareas dedicadas al muestreo de sensores, tareas dedicadas a la conectividad, tareas de *logging*... 

Por tanto, al comenzar un desarrollo con un nuevo *RTOS* (Real-Time Operating System) es importante conocer qué servicios ofrece el sistema para la gestión de hilos/tareas. En ocasiones, puede no haber ningún soporte. En otras ocasiones, el API ofrecida será específica del sistema operativo utilizado (como es el caso con FreeRTOS y, por tanto, con la extensión que usaremos: ESP-IDF). Y, en ocasiones, el sistema ofrecerá algún API estándar, como el de [POSIX](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html).

En los vídeos y transparencias de la asignatura disponibles en el Campus Virtual se hace una breve introducción de los mecanismos de:

* Creación y destrucción de tareas en ESP-IDF.
* Comunicación y sincronización de tareas mediante colas 
* Uso de *eventos* como sistema de comunicación asíncrona.

Los siguientes ejercicios se proponen como una práctica sencilla de esos mecanismos.


## Primera sesión: Ejercicios básicos



### Creación de una tarea para realizar el muestreo

Escribe una aplicación que creará una tarea para muestrear un sensor. Denominaremos *muestreadora* a dicha tarea y deberá muestrear periódicamnete el sensor [SHTC3 de Sensirion](https://sensirion.com/resource/datasheet/shtc3).  La tarea muestreadora comunicará la lectura con la tarea inicial (la que ejecuta `app_main()`) a través de una variable global. 

!!! danger "Tarea"
	La tarea creada leerá el valor del sensor  con un período que se pasará como argumento a la tarea. Dicha tarea tendrá un bucle infinito en el que realizará la lectura del sensor, modificará la variable global y dormirá durante el tiempo establecido. La tarea inicial (app_main) recogerá el valor muestreado y lo mostrará por puerto serie.

!!! note "Cuestión"
    * ¿Qué prioridad tiene la tarea inicial que ejecuta la función `app_main()`? ¿Con qué llamada de ESP-IDF podemos conocer la prioridad de una tarea?
	* ¿Cómo sincronizas ambas tareas? ¿Cómo sabe la tarea inicial que hay un nuevo dato generado por la tarea muestreadora?
	* Si además de pasar el período como parámetro, quisiéramos pasar como argumento la dirección en la que la tarea muestreadora debe escribir las lecturas, ¿cómo pasaríamos los dos argumentos a la nueva tarea?


### Comunicación mediante colas

Modifica el código anterior para que las dos tareas (inicial y muestreadora) se comuniquen mediante una [cola de ESP-IDF]https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#queue-api).

!!! danger "Tarea" 
    La tarea creada (muestreadora) recibirá como argumento el período de muestreo y la cola en la que deberá escribir los datos leídos.
  
!!! note "Cuestión"
    Al enviar un dato por una cola, ¿el dato se pasa por copia o por referencia?. Consulta la documentación para responder.

### Uso de eventos

Finalmente, se modificará nuevamente el código de muestreo original (no el que usa una cola para comunicar) para que utilice [eventos](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html) para notificar que hay una nueva lectura que mostrar por el puerto serie.

Para ello se declara un nuevo *event base* llamado *SENSOR_EVENT* y al menos un `event ID` que se denominará `SENSOR_EVENT_NEWSAMPLE`.

!!! danger "Tarea" 
    La tarea creada (muestreadora) recibirá como argumento el período de muestreo. Cuando tenga una nueva muestra, la comunicará a través de `esp_event_post_to()`. La tarea inicial registrará un `handler` que se encargará de escribir en el puerto serie.

!!! note "Cuestión"
    ¿Qué debe hacer la tarea inicial tras registrar el *handle*? ¿Puede finalizar?   


!!! note "ENTREGA"
    **NO ES NECESARIO HACER UNA ENTREGA DE LAS TAREAS DE ESTA SESIÓN** 

## Segunda sesión: ejercicio de estructuración de código


Queremos montar un sistema monitorice la temperatura y humedad con un cierto período, y envíe los datos por red.  Así mismo, monitorizará la pulsación de un botón para detectar la cercanía de un operador, lo que llevará al sistema a un modo de funcionamiento diferente, mostrando una consola por puerto serie.

La funcionalidad del sistema será la siguiente:

* El sistema tendrá dos modos de funcionamiento: `monitorización`  o `consola`. En el primero, se monitorizará y enviará la temperatura y humedad. En el segundo se permitirá el uso de una consola de comandos (puedes ver la   [documentación de este componente en este enlace](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/console.html))

* Monitorizará la temperatura y la humedad cada `n` segundos, siendo éste un parámetro seleccionable por `menuconfig`. La lectura de cada medida se *comunicará mediante eventos*. Todo el código relacionado con las lecturas del sensor estará en un componente separado. Se valorará la modularización del código (uso de más componentes) para el resto de funcionalidad de este punto.

* Las lecturas se enviarán por red mediante WiFi. En este caso, no usaremos realmente la WiFi pero se programará un componente que lo simule. Ofrecerá un API similar a:
    * `wifi_connect()` trata de conectar a WiFi. Cuando la conexión se produce, recibiremos un evento. Una vez conseguida, tratará de conseguir una IP (sin que hagamos ninguna otra llamada)  y recibiremos un evento al conseguirla.
     * `wifi_disconnect()`. Desconecta de la WiFi.
    * `esp_err_t send_data_wifi(void* data, size_t size)`. Permite enviar un dato mediante la conexión WiFI. Devolverá un error si el envío no se pudo realizar. Imprimirá el dato por puerto serie (pasaremos siempre una cadena de caracteres como dato de entrada).

    El componente enviará los siguientes eventos:
    * `WIFI_CONECTADO`. Se enviará cuando el módulo de WiFi consiga conexión (equivalente a conectar al SSID)
    * `WIFI_DESCONECTADO`. Se enviará cuando se pierda la conectividad. Será necesario llamar a `wifi_connect()` nuevamente para volver a conectar.
    * `IP_CONSEGUIDA`. Se enviará cuando se haya conseguido IP. Hasta entonces, la aplicación no debería llara a `send_data_wifi` pues fallará siempre. En otro caso, esa llamada siempre dará éxito.

    El componente simulará la conexión WiFI y tendrá un parámetro que modelará la latencia de conexión (tiempo desde que se llama a `wifi_connect()` hasta que se conecta a WiFi), latencia para conseguir IP y la tasa de desconexión (probabilidad de que la conexión falle y tengamos que volver a conectar. Se modelará como un número de segundos tras la conexión)

* Cuando no se disponga de conectividad WiFi, las lecturas del sensor se seguirán realizando, pero se almacenarán en memoria Flash. Para simular esta parte, se creará un nuevo componente que emulará el uso de la memoria Flash mediante el siguiente API:
    *  `esp_err_t writeToFlash(void* data, size_t size)`  permite escribir en memoria flash el dato `data` de tamaño `size` bytes La siguiente llamada a  `writeToFlash()` escribirá a continuación del dato anterior sin sobreescribirlo.
    *  `void* readFromFlash(size_t size)`. Lee el dato (`size` bytes) más antiguo almacenado en la flash. Esos bytes quedan marcados como leídos y se podrán usar en futuras escrituras.
    * `size_t getDataLeft()` nos devuelve cuántos bytes hay pendientes de ser leídos en la flash.

    Se realizará un componente que emule así el comportamiento de una memoria Flash. Como simplificación, se asumirá que el tamaño de lectura/escritura siempre será el mismo (el tamaño de un `float`, que será el tipo usado en las lecturas del sensor). Los datos se almancenarán en un buffer circular (no hay que usar la flash de verdad).

* Cuando consigamos conexión (WiFi + IP), enviaremos los datos que tengamos pendientes en la memoria flash (si hay alguno).

* La aplicación monitorizará (cada `nbutton` segundos; parametrizable) un pin de GPIO para detectar pulsaciones de un botón. Si se produce una pulsación, pasaremos al modo `consola`. Se escribirá un componente para esta funcionalida. Si se detecta una pulsación,  *se enviará un evento*.

* Cuando estemos en el modo `consola` se utilizará el componente `consola` de ESP-IDF para leer comandos del usuario. En concreto habrá 3 comandos disponibles:
    * `help` que mostrará los comandos disponibles
    * `monitor` que volverá nuevamente al modo `monitorización`, tratando de conectar a WiFi de nuevo.
    * `quota` que nos informará de cuántos bytes tiene ocupadas la flash simulada (es decir, cuántos no se han leído)

    Durante este modo de funcionamiento, no se monitorizará el sensor [SHTC3 de Sensirion](https://github.com/esp-rs/esp-rust-board?tab=readme-ov-file#:~:text=SHTC3-,Datasheet,-Link)  y nos desconetaremos de la WiFi.

!!! danger "Tarea" 
    Escribe una aplicación que realice la funcionalidad anterior. Se valorará especialmente la modularidad y estructura del código, de modo que sea extensible y reutilizable.
    







