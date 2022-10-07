# Práctica 3. Uso de timers.

# Objetivos

El objetivo  de esta práctica es conocer los mecanismos para la gestión de tareas
que ofrece FreeRTOS, concretamente en su porting  ESP-IDF (con alguna
particularidad por el hecho de estar adaptado a tener 2 cores).

Trabajaremos los siguientes aspectos del API de ESP-IDF:

* Familiarizarse con la API de *High Resolution Timers*  en ESP/IDF.
* Utilización de elementos conectados a pines controlados por GPIO

## Material de consulta
Para ver los detalles de cada aspecto de esta práctica se recomienda la lectura de los siguientes enlaces:

* [API de High Resolution Timers](hhttps://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html)
* [Documentación sobre GPIO - ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)


## Introducción

Debe evitarse el uso de llamadas tipo *delay()* para la gestión de  tareas periódicas. No proporciona la precisión necesaria y obliga a realizar estimaciones del tiempo transcurrido entre dos llamadas para ajustar el tiempo de la siguiente espera.

Para solventar ese problema, lo más aconsejable es usar *Timers*, contadores hardware que interrumpen la ejecución cada cierto tiempo (programable). En esta práctica exploraremos el uso de los *timers* que proporciona ESP-IDF.

Asimismo, comenzaremos a examinar mecanismos básicos de entrada/salida. Concretamente, estudiaremos el uso del controlador de GPIO (*General Puropose Input/Output*) que permite interaccionar con los pines del SoC (algunos de ellos, disponibles en la placa).

## ESP-IDF: High Resolution Timer
Un *Timer* es un temporizador que podemos programar para que nos *avise* transcurrido un cierto tiempo. Es similar a una cuenta atrás con alarma y es un mecanismo perfecto para planificar tareas periódicas. El *aviso* será asíncrono, por lo que no sabemos en qué punto de nuestro código estaremos cuando se dispare la alarma.

ESP-IDF ofrece un API para el uso de *timers* que, a su vez, utlizan los *timers* de 64 bits disponibles en el hardware para garantizar una precisión de hasta 50us. 

Cuando programamos un *timer* podemos optar por 2 comportamientos:

* *One-shot* (`esp_timer_start_once()`), que programrá el *timer* para que genere una única alarma transcurrido el plazo establecido.
* *Continuo* (`esp_timer_start_periodic()`) que re-programará el *timer* de forma automática cada vez que la cuenta llegue a 0. Este mecanismo es el idóneo para muestreos periódicos.

Cuando el *timer* genere la alarma, se ejecutará un *callback*, una función que habremos definido previamente (y asociado a ese *timer*). Dicha función se ejecutará en el contexto de una tarea específica (`ESP_TIMER_TASK`) o en el de una rutina de tratamiento de interrupción (`ESP_TIMER_ISR`). En nuestro caso, es aconsejable usar el primer mecanismo (tarea específica).

En la [documentación de ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html) podéis encontrar el resto de llamadas relevantes para crear y configurar *timers*. Es muy recomendable, asimismo, estudiar los [ejemplos disponibles en la distribución](https://github.com/espressif/esp-idf/tree/master/examples/system/esp_timer)

## Ejercicios básicos

### Uso de timers

!!! danger "Tarea"
	Nuevamente procederemos a muestrear el sensor de efecto Hall, pero esta vez usando un *timer*. Crea un *timer* con un período de 2 segundos y realiza la lectura del sensor en el *callback*. Dicho *callback* generará un evento que desencadenará la escritura del valor leído en el puerto serie.

!!! note "Cuestión"
	¿Qué hará la tarea inicial (la que invoca a `app_main()`) tras configurar el *timer* y el *evento*?

