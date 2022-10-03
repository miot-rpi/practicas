# Práctica 2. Programación con tareas y eventos en ESP-IDF

## Objetivos

El objetivo  de esta práctica es conocer los mecanismos para la gestión de tareas
que ofrece FreeRTOS, concretamente en su porting  ESP-IDF (con alguna
particularidad por el hecho de estar adaptado a tener 2 cores).

Trabajaremos los siguientes aspectos del API de ESP-IDF:

* Familiarizarse con la API de *tareas* y *eventos* en ESP/IDF.
* Uso de *delays* para tareas periódicas (veremos mejores opciones en el futuro)
* Comunicación y sincronización de tareas mediante colas

## Material de consulta
Para ver los detalles de cada aspecto de esta práctica se recomienda la lectura de los siguientes enlaces:

* [API de ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/system/freertos.html)
* [Documentación oficial de FreeRTOS](https://www.freertos.org/features.html}. Documentación oficial de FreeRTOS)
* [Mastering de FreeRTOS Real Time Kernel](https://www.freertos.org/fr-content-src/uploads/2018/07/161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf)

## Introducción

Al desarrollar código para sistemas empotrados, como nuestro nodo basado en ESP32, es habitual organizar la aplicación en torno a diferentes tareas que se ejecutan de forma concurrente. Habrá tareas dedicadas al muestreo de sensores, tareas dedicadas a la conectividad, tareas de *logging*... 

Por tanto, al comenzar un desarrollo con un nuevo *RTOS* (Real-Time Operating System) es importante conocer qué servicios ofrece el sistema para la gestión de hilos/tareas. En ocasiones, puede no haber ningún soporte. En otras ocasiones, el API ofrecida será específica del sistema operativo utilizado (como es el caso con FreeRTOS y, por tanto, con la extensión que usaremos: ESP-IDF). Y, en ocasiones, el sistema ofrecerá algún API estándar, como el de [POSIX](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html).

En los vídeos y transparencias de la asignatura disponibles en el Campus Virtual se hace una breve introducción de los mecanismos de:

* Creación y destrucción de tareas en ESP-IDF.
* Comunicación y sincronización de tareas mediante colas 
* Uso de *eventos* como sistema de comunicación asíncrona.

Los siguientes ejercicios se proponen como una práctica sencilla de esos mecanismos.


## Ejercicios básicos

### Muestreo periódico del sensor de efecto Hall
Un sensor de efecto Hall permite la medición de campos magnéticos o corrientes. En este ejercicio no entraremos a ver los detalles del sensor en sí y simplemente estamos interesados en su uso en el entorno ESP-IDF. El [Hall sensor](https://docs.espressif.com/projects/esp-idf/en/v4.0.3/api-reference/peripherals/adc.html#_CPPv416hall_sensor_readv) está conectado a un canal del *ADC* (Conversor Analógico-Digital) que estudiaremos más adelante. Por ahora, nos basta con saber que para realizar una lectura del sensor basta con invocar a la función `hall_sensor_read()` y que, previamente, es necesario configurar el ADC mediante la llamada `adc1_config_width(ADC_WIDTH_12Bit)` (sólo es necesario invocar esta llamada una vez).

```c
#include <driver/adc.h>
...

    adc1_config_width(ADC_WIDTH_12Bit);
    int val = hall_sensor_read();
```
En este primer ejercicio NO crearemos más tareas y simplemente usaremos un bucle infinito en la función `app_main()` para leer de forma periódica el sensor. Asimismo, usaremos la llamada `void vTaskDelay(const TickType_t xTicksToDelay)` para realizar las esperas entre lecturas.

!!! danger "Tarea"
	Crea una aplicación que lea el valor del sensor de efecto Hall cada 2 segundos y muestre el valor leído por puerto serie.

!!! note "Cuestión"
    ¿Qué prioridad tiene la tarea inicial que ejecuta la función `app_main()`? ¿Con qué llamada de ESP-IDF podemos conocer la prioridad de una tarea?


### Creación de una tarea para realizar el muestreo

Modifica el código anterior para crear una nueva tarea que sea la encargada de realizar el muestreo (denominaremos *muestreadora* a dicha tarea). La tarea muestreadora comunicará la lectura con la tarea inicial (la que ejecuta `app_main()`) a través de una variable global.

!!! danger "Tarea"
	La tarea creada leerá el valor del sensor de efecto Hall con un período que se pasará como argumento a la tarea. La tarea inicial recogerá ese valor y lo mostrará por puerto serie.

!!! note "Cuestión"
	* ¿Cómo sincronizas ambas tareas?¿Cómo sabe la tarea inicial que hay un nuevo dato generado por la tarea muestreadora?
	* Si además de pasar el período como parámetro, quisiéramos pasar como argumento la dirección en la que la tarea muestreadora debe escribir las lecturas, ¿cómo pasaríamos los dos argumentos a la nueva tarea?


### Comunicación mediante colas

Modifica el código anterior para que las dos tareas (inicial y muestreadora) se comuniquen mediante una [cola de ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#queue-api).

!!! danger "Tarea" 
    La tarea creada (muestreadora) recibirá como argumento el período de muestreo y la cola en la que deberá escribir los datos leídos.
  
!!! note "Cuestión"
    Al enviar un dato por una cola, ¿el dato se pasa por copia o por referencia?. Consulta la documentación para responder.

### Uso de eventos

Finalmente, se modificará nuevamente el código de muestreo original (no el que usa una cola para comunicar) para que utilice [eventos](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html) para notificar que hay una nueva lectura que mostrar por el puerto serie.

Para ello se declara un nuevo *event base* llamado *HALL_EVENT* y al menos un `event ID` que se denominará `HALL_EVENT_NEWSAMPLE`.

!!! danger "Tarea" 
    La tarea creada (muestreadora) recibirá como argumento el período de muestreo. Cuando tenga una nueva muestra, la comunicará a través de `esp_event_post_to()`. La tarea inicial registrará un `handler` que se encargará de escribir en el puerto serie.

!!! note "Cuestión"
    ¿Qué debe hacer la tarea inicial tras registrar el *handle*? ¿Puede finalizar?   
 
## Ejercicio adicional
Los ejercicios anteriores son de entrega obligatoria para obtener un 5 en la calificación de la práctica. La realización del siguiente ejercicio permite mejorar la calificación.

### Gestión de múltiples tareas
Se creará un sistema con múltiples tareas que se comunicarán entre sí por eventos:

* Tarea *Sampler* (muestreadora), similar a la del apartado anterior. Comunicará sus muestras mediante eventos (`HALL_EVENT_NEWSAMPLE`).
* Tarea *Filter* que irá acumulando muestras de la tarea *Sampler* y realizará la media  de las últimas 5 muestras recibidas. Enviará dicha media mediante un evento `HALL_EVENT_FILTERSAMPLE`.
* Tarea *Logger* que escribe por puerto serie lo que le comunican otras tareas. Debe ser una tarea diferente a la inicial. Recibirá información a través de los eventos correspondientes (a los que deberá suscribirse).
* Tarea *Monitor* que monitoriza el estado de las otras tareas creadas. Usará el API de Tareas para consultar el espacio de pila restante de cada tarea. Comunicará los datos a la tarea *Logger*, incluyendo nombre de la tarea, prioridad y cantidad de pila restante. Puede usar la llamada [vTaskGetInfo()](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#task-api) que devuelve un tipo [TaskStatus_t](https://www.freertos.org/vTaskGetInfo.html#TaskStatus_t). Enviará la información periódicamente (cada minuto) a través del evento `TASK_EVENT_MONITOR`.

La tarea inicial creará las tareas *Sampler*, *Filter* y *Logger* y, por último, la tarea *Monitor* que recibirá los *handlers* de las tareas anteriores. 

!!! danger "Tarea" 
    Escribe una aplicación que realice la funcionalidad anterior. El código debe organizarse en varios ficheros fuente (.c): uno para la funcionalidad relativa al muestreo del sensor y su filtrado, otro para el *logging* y otro para la monitorización de tareas (además del fichero `main.c`). Si se usa algún tipo de datos propio (una cola circular puede ser perfecta para el filtrado), se incluirá en otro fichero fuente diferente, con su fichero de cabecera (.h) correspondiente

!!! note "Cuestión"
    ¿Qué opinas de esta estructura de código? ¿Es razonable el número de tareas empleado? ¿Qué cambiarías en el diseño? 



