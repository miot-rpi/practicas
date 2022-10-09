# Práctica 3. Uso de timers y GPIO.

## Objetivos

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

A continuación se incluye un ejemplo de uso, extraíado de la distribución de ESP-IDF:

```c
void app_main() { 
     ...
    const esp_timer_create_args_t periodic_timer_args = {
         .callback = &periodic_timer_callback,
         .name = "periodic" };
    esp_timer_handle_t periodic_timer; esp_timer_create(&periodic_timer_args, &periodic_timer);
     ...
    esp_timer_start_periodic(periodic_timer, 500000); ....
    esp_timer_stop(periodic_timer); ...
    esp_timer_delete(periodic_timer); 
}

static void periodic_timer_callback(void* arg) { 
    int64_t time_since_boot = esp_timer_get_time();
    printf("Periodic timer called, time since boot: %lld us",time_since_boot);
}
```
## ESP - IDF: GPIO
Los controladores de GPIO (*General Purpose Input-Ouput*) permiten controlar ciertos pines de nuestro dispositivo para usarlos como entrada (por ejemplo, para conectar un botón) o salida (por ejemplo para conectar un LED) o con funciones *especiales* (que forme parte de un bus serie, por ejemplo). 

El SoC ESP32 que usamos proporciona 40 GPIO *pads* (el SoC no tiene pines propiamente dichos, sino conectores, normalmente de superfície, que se deminan *pad*). El módulo WROOM-32 que usamos expone 38 de ellos, que son accesibles a traves de los pines (los conectores físicos a ambos lados de la placa) que incorpora nuestra placa DevKitC.

En la siguiente figura se muestra la disposición de los pines en la placa ESP32-DevKitC que usamos en nuestras prácticas: 

![pinout](img/esp32-devkitC-v4-pinout.png)

En [la web de Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html) se pueden encontrar más detalles de la placa.

[Como se indica en la documentación de  ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html), algunos de esos pines tiene un propósito específico. Por ejemplo, GPIO6-11 y 16-17 no deben usarse porque están internamente conectados a la memoria SPI flash. También nos indica que los pines del canal 2 del ADC (ADC2) NO deben usarse mientras se utiliza Wi-Fi.  Es muy conveniente leer todas las restricciones para evitar problemas en nuestros desarrollos.

La documentación muestra también la API que ofrece ESP-IDF para configurar los pines (entrada o salida, uso de pull-up/pull-down) establecer un valor lógico (0 ó 1)en un pin (previamente configurado como salida) o leer el valor lógico de un pin (configurado como entrada).

El siguiente código, [extraído del ejemplo de GPIO proporcionado en la distribución ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/gpio/generic_gpio), muestra cómo configurar los pines GPIO18 y GPIO19 como salida. Observa cómo se construye la máscara de bits `GPIO_OUTPUT_PIN_SEL` para indicar a `gpio_config()` qué pines se configuran.


```c
#define GPIO_OUTPUT_IO_0 18
#define GPIO_OUTPUT_IO_1 19
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
gpio_config_t io_conf;
io_conf.intr_type = GPIO_PIN_INTR_DISABLE; 
io_conf.mode = GPIO_MODE_OUTPUT; 
io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; 
io_conf.pull_down_en = 0; 
io_conf.pull_up_en = 0; 
gpio_config(&io_conf);
``` 
Posteriormente, podemos establecer el valor lógico de la salida con una llamada similar a `gpio_set_level(GPIO_OUTPUT_IO_1, valor);`, siendo `valor` igual a 0 ó 1.

De forma similar el siguiente código configura los pines 4 y 5 como entrada:

```c

    #define GPIO_INPUT_IO_0 4
    #define GPIO_INPUT_IO_1 5
    #define GPIO_INPUT_PIN_SEL ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
    gpio_config_t io_conf;  
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);
```

Posteriormente, podremos leer el valor lógico de esos pines con una llamada a `gpio_get_level()`.


## Ejercicios básicos

### Uso de timers

!!! danger "Tarea"
	Nuevamente procederemos a muestrear el sensor de efecto Hall, pero esta vez usando un *timer*. Crea un *timer* con un período de 2 segundos y realiza la lectura del sensor en el *callback*. Dicho *callback* generará un evento que desencadenará la escritura del valor leído en el puerto serie.

!!! note "Cuestión"
	¿Qué hará la tarea inicial (la que invoca a `app_main()`) tras configurar el *timer* y el *evento*?

### Encendido de LEDs con GPIO
!!! danger "Tarea"
	Configura un GPIO como salida y conéctalo a un LED del entrenador del laboratorio. Programa un *timer* para cambiar el estado del LED cada segundo.

!!! note "Cuestión"
	¿Qué diferencia hay entre habilitar el *pull-up* o el *pull-down* del GPIO elegido?

### Lectura  con GPIO
!!! danger "Tarea"
	Configura un GPIO como entrada y conéctalo a una fuente de voltaje del entrenador, asegurándote de que nunca supera 3.3V. Nuevamente, usa un *timer* para muestrear la entrada cada 10ms. Cuando se produzca un cambio de más de 1V respecto a la lectura anterior, genera un evento que, a su vez, desencadene una escritura en el puerto serie informando de la última lectura.

!!! note "Cuestión"
	¿Qué voltaje se observa en los botones y swithches del entrenador? ¿Podemos conectarlos a los pines del ESP32? 


## Ejercicios avanzados

### Interrupciones

Las interrupciones son un mecanismo más eficiente que el muestreo para atender eventos externos. ESP-IDF ofrece la posibilidad de configurar los GPIO como entrada y fuente de interrupciones (no son la única fuente de interrupciones).

!!! danger "Tarea"
    Modifica el ejercicio de lectura del GPIO para que funcione por interrupciones. 



