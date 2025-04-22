# Laboratorio 5. Clasificación de imágenes y detección de objetos con el ESP32

## Objetivos

* Entender la estructura y utilidad general del *framework* ESP-DL.
* Experimentar con dos ejemplos básicos de clasificación de imágenes (Mobilenet) y detección de objetos (Yolo).
* Comprender y evaluar el rendimiento (en términos de tiempo y precisión) de distintas variantes del modelo Yolo para detección de objetos.
* Integrar el proceso de inferencia para detección de objetos en un sistema IoT para emular un entorno de detección de peatones para asistencia a conducción.

## ESP-DL

**ESP-DL** es un framework ligero y eficiente para la inferencia de redes neuronales, diseñado específicamente para los chips de la serie **ESP**. Con ESP-DL, puedes desarrollar aplicaciones de inteligencia artificial de forma rápida y sencilla utilizando los **System on Chips (SoCs)** de **Espressif**.

#### Visión general

ESP-DL ofrece **APIs** para cargar, depurar y ejecutar modelos de IA. El framework es fácil de usar y se puede integrar sin problemas con otros SDKs de Espressif. **ESP-PPQ** actúa como la herramienta de cuantización para ESP-DL, siendo capaz de cuantizar modelos procedentes de **ONNX**, **Pytorch** y **TensorFlow**, y exportarlos al formato estándar de modelo de ESP-DL.

#### Formato de modelo ESP-DL

Este formato es similar a ONNX pero utiliza **FlatBuffers** en lugar de **Protobuf**, lo que lo hace más ligero y permite una deserialización sin copia (**zero-copy**). Su extensión de archivo es `.espdl`.

#### Implementación eficiente de operadores

ESP-DL implementa de forma eficiente operadores comunes de IA como `Conv`, `Gemm`, `Add` y `Mul`.  

#### Planificador de memoria estático

Este planificador asigna automáticamente las distintas capas a la ubicación de memoria óptima en función del tamaño de RAM interna especificado por el usuario, garantizando una alta velocidad de ejecución general y un uso mínimo de memoria.

#### Planificación de doble núcleo

La planificación automática en doble núcleo permite que los operadores de mayor carga computacional aprovechen al máximo la capacidad de cómputo de los dos núcleos. Actualmente, `Conv2D` y `DepthwiseConv2D` son compatibles con esta planificación.

#### Activación con LUT de 8 bits

Todas las funciones de activación, excepto `ReLU` y `PReLU`, se implementan en ESP-DL mediante una **tabla de búsqueda (LUT)** de 8 bits para acelerar la inferencia. Puedes utilizar cualquier función de activación sin que aumente la complejidad computacional.

---

La figura que se muestra a continuación ilustra la arquitectura general de ESP-DL.

![ESP-DL architecture](./img/espdl_arch.svg)

#### Preparación del modelo

Consulta el [documento de soporte de operadores](https://github.com/espressif/esp-dl/blob/12397f3/operator_support_state.md) para asegurarte de que los operadores en un nuevo modelo son compatibles.

ESP-DL requiere el uso de un **formato propietario** para el despliegue de modelos. Los modelos de deep learning deben ser **cuantizados y convertidos** a este formato antes de poder ser utilizados. **ESP-PPQ** (la herramienta de soporte para cuantización) proporciona dos interfaces, `espdl_quantize_onnx` y `espdl_quantize_torch`, para admitir la exportación de modelos ONNX y PyTorch, respectivamente.

Otros frameworks de deep learning, como **TensorFlow**, **PaddlePaddle**, etc., deben convertir primero el modelo a **ONNX**. Por lo tanto, asegúrate de que el modelo puede convertirse a formato ONNX.

Para más detalles, consulta:  

- [Cómo cuantizar un modelo](https://docs.espressif.com/projects/esp-dl/en/latest/tutorials/how_to_quantize_model.html).

#### Despliegue del modelo

ESP-DL proporciona una serie de **APIs** para cargar y ejecutar modelos de forma rápida. Para más detalles, consulta:

- [Cómo cargar, probar y perfilar un modelo](https://docs.espressif.com/projects/esp-dl/en/latest/tutorials/how_to_load_test_profile_model.html).
- [Cómo ejecutar un modelo](https://docs.espressif.com/projects/esp-dl/en/latest/tutorials/how_to_run_model.html).

En las siguientes secciones, partiremos siempre de modelos ya preparados y ejemplos totalmente funcionales, que deberás adaptar para completar las tareas indicadas en el boletín.

## Ejecución básica de un modelo

En primer lugar, clona el proyecto de ESP-DL que puedes encontrar en el [siguiente repositorio](https://github.com/espressif/esp-dl.git). En este primer ejemplo, trabajaremos sobre el directorio `examples/tutorial/how_to_run_model`, por lo que puedes abrirlo en Visual Studio (utiliza al menos la versión 5.4.1 de ESP-IDF).


#### Obtener entradas y salidas del modelo

```cpp
std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
dl::TensorBase *model_input = model_inputs.begin()->second;
std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
dl::TensorBase *model_output = model_outputs.begin()->second;
```

Puedes obtener los nombres de entrada/salida y sus respectivos objetos `dl::TensorBase` usando las APIs `get_inputs()` y `get_outputs()`.  
Para más información, consulta la [documentación de dl::TensorBase](../api_reference/tensor_api).

> **Nota:**  
> El gestor de memoria de ESP-DL asigna un único bloque de memoria para las entradas, resultados intermedios y salidas del modelo. Como se comparte esta memoria, durante la inferencia, los resultados posteriores sobrescriben a los anteriores. Es decir, los datos de `model_input` pueden ser sobrescritos por `model_output` u otros resultados intermedios una vez finalizada la inferencia.

---

#### Cuantizar la entrada

Los modelos cuantizados a 8 bits y 16 bits aceptan entradas de tipo `int8_t` y `int16_t` respectivamente.  
Las entradas en coma flotante (`float`) deben cuantizarse en uno de esos tipos según el `exponent` antes de pasarlas al modelo.

#### Fórmula de cuantización

```math
Q = 	ext{Clip}\left(	ext{Round}\left(rac{R}{	ext{Scale}}
ight), 	ext{MIN}, 	ext{MAX}
ight)
```

```math
	ext{Scale} = 2^{	ext{Exp}}
```

Donde:

- `R` es el número en punto flotante a cuantizar.  
- `Q` es el valor entero tras cuantización, recortado al rango [MIN, MAX].  
- `MIN`: -128 (8 bits), -32768 (16 bits).  
- `MAX`: 127 (8 bits), 32767 (16 bits).

### Cuantizar un solo valor

```cpp
float input_v = VALUE;
// dl::quantize usa el inverso del scale como segundo argumento, por eso usamos DL_RESCALE.
int8_t quant_input_v = dl::quantize<int8_t>(input_v, DL_RESCALE(model_input->exponent));
```

#### Cuantizar un `dl::TensorBase`

```cpp
// Se asume que input_tensor ya contiene los datos en float.
dl::TensorBase *input_tensor;
model_input->assign(input_tensor);
```

---

#### Descuantizar la salida

Los modelos cuantizados a 8 o 16 bits devuelven valores de tipo `int8_t` o `int16_t`, respectivamente.  
Estos valores deben ser descuantizados según el `exponent` para obtener los resultados en punto flotante.

#### Fórmula de descuantización

```math
R' = Q 	imes 	ext{Scale}
```

```math
	ext{Scale} = 2^{	ext{Exp}}
```

Donde:

- `R'` es el valor en punto flotante aproximado tras la descuantización.  
- `Q` es el valor entero resultante de la cuantización.

#### Descuantizar un solo valor

```cpp
int8_t quant_output_v = VALUE;
float output_v = dl::dequantize(quant_output_v, DL_SCALE(model_output->exponent));
```

#### Descuantizar un `dl::TensorBase`

```cpp
// Crear un TensorBase de tipo float con forma [1, 1]
dl::TensorBase *output_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);
output_tensor->assign(model_output);
```

---

#### Inferencia del modelo

Consulta:

- [Ejemplo del proyecto](examples/tutorial/how_to_run_model)
- `void dl::Model::run(runtime_mode_t mode)`
- `void dl::Model::run(TensorBase *input, runtime_mode_t mode)`
- `void dl::Model::run(std::map<std::string, TensorBase*> &user_inputs, runtime_mode_t mode, std::map<std::string, TensorBase*> user_outputs)`

!!! danger "Tarea"
    Estudia la API que se utiliza en el ejemplo para realizar inferencia con ESP-DL. Observa los tres métodos distintos para cuantizar/descuantizar las entradas y salidas del modelo. En el primer método, descomenta las líneas que proporcionan información sobre entrada y salida del modelo, para que se muestren por pantalla. Opcionalmente, temporiza los procesos de inferencia.

## Clasificación de imágenes con ESP-DL

En este segundo ejemplo, estudiaremos (sin modificar de momento) un código básico de inferencia sobre un modelo 
de clasificación de imágenes (MobilenetV2), similar al que utilizaste en prácticas anteriores sobre el acelerador
Google Coral. Específicamente, abre en Visual Studio Code el ejemplo `examples/mobilenetv2_cls`, constrúyelo para la placa
ESP-EYE (esp32-s3) y monitoriza la salida. Verás la clasificación de la imagen `cat.jpg` como perteneciente a una de las
cinco clases de gatos que para las que ha sido entrenado el modelo. 

Observa el código del ejemplo:
```cpp
#include "esp_log.h"
#include "imagenet_cls.hpp"
#include "bsp/esp-bsp.h"

extern const uint8_t cat_jpg_start[] asm("_binary_cat_jpg_start");
extern const uint8_t cat_jpg_end[] asm("_binary_cat_jpg_end");
const char *TAG = "mobilenetv2_cls";

extern "C" void app_main(void)
{
#if CONFIG_IMAGENET_CLS_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_mount());
#endif

    dl::image::jpeg_img_t jpeg_img = {
        .data = (uint8_t *)cat_jpg_start,
        .width = 300,
        .height = 300,
        .data_size = (uint32_t)(cat_jpg_end - cat_jpg_start),
    };
    dl::image::img_t img;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    sw_decode_jpeg(jpeg_img, img, true);

    ImageNetCls *cls = new ImageNetCls();

    auto &results = cls->run(img);
    for (const auto &res : results) {
        ESP_LOGI(TAG, "category: %s, score: %f", res.cat_name, res.score);
    }
    delete cls;
    heap_caps_free(img.data);

#if CONFIG_IMAGENET_CLS_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
#endif
}
```

Observa que la complejidad del tratamiento de las entradas y salidas del modelo se encapsula en una clase `ImageNetCls`. Su ejecución sigue la misma lógica que la que vimos anteriormente pare el ejemplo básico:

```cpp
dl::image::img_t img = {.data=DATA, .width=WIDTH, .height=HEIGHT, .pix_type=PIX_TYPE};
std::vector<dl::cls::result_t> &res = detect->run(img);
```

!!! danger "Tarea"
    Descarga más imágenes similares y observa la salida del modelo. Si te es posible, temporiza la ejecución del mismo y la fase de preparación de la imagen de entrada.

!!! danger "Tarea (opcional)"
    Modifica el código para que las imágenes se tomen directamente desde la cámara del ESP-EYE.

## Detección de objetos con ESP-DL

En este caso, trabajaremos con el ejemplo `yolo11_detect`, disponible en la distribución de ESP-DL. Ábrelo, compílalo y monitoriza su corrección.

El ejemplo proporciona diversas variantes de modelos (en función del tipo de datos que utilizan), que devolverán disintos niveles de precisión/calidad a cambio de mayor o menor tiempo de ejecución. Estas variantes puede configurarse a través de *menuconfig* (sección *models: coco_detect*). 
En todo caso, la salida de un proceso de inferencia deberá ser algo similar a:

```sh
I (28477) yolo11n: [category: 0, score: 0.817575, x1: 24, y1: 196, x2: 111, y2: 453]
I (28477) yolo11n: [category: 5, score: 0.731059, x1: 81, y1: 115, x2: 400, y2: 372]
I (28477) yolo11n: [category: 0, score: 0.731059, x1: 112, y1: 203, x2: 171, y2: 429]
I (28487) yolo11n: [category: 0, score: 0.731059, x1: 336, y1: 196, x2: 404, y2: 436]
I (28497) yolo11n: [category: 0, score: 0.320821, x1: 0, y1: 276, x2: 29, y2: 434]
```

Esta salida muestra la categoría de cada objeto detectado, su probabilidad de pertenencia a la clase, y la posición del *bounding box* que lo contiene.

!!! danger "Tarea"
    Modifica el proyecto para utilizar las distintas variantes del modelo proporcionado, y anota la calidad del proceso de inferencia en términos de precisión y, a ser posible, de tiempo de ejecución.

Observa que la forma de preparar la entrada y reportar la salida de la ejecución del modelo es también sencilla:

```cpp
#include "coco_detect.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

extern const uint8_t bus_jpg_start[] asm("_binary_bus_jpg_start");
extern const uint8_t bus_jpg_end[] asm("_binary_bus_jpg_end");
const char *TAG = "yolo11n";

extern "C" void app_main(void)
{
#if CONFIG_COCO_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_mount());
#endif

    dl::image::jpeg_img_t jpeg_img = {
        .data = (uint8_t *)bus_jpg_start,
        .width = 405,
        .height = 540,
        .data_size = (uint32_t)(bus_jpg_end - bus_jpg_start),
    };
    dl::image::img_t img;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    sw_decode_jpeg(jpeg_img, img, true);

    COCODetect *detect = new COCODetect();
    auto &detect_results = detect->run(img);
    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }
    delete detect;
    heap_caps_free(img.data);

#if CONFIG_COCO_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
#endif
}
```

!!! danger "Tarea entregable (70% de la nota)"
    Partiendo del proyecto original, y probando con varias imágenes de entrada, vas a emular un sistema de ayuda a la conducción que envíe una alarma al usuario si detecta un peatón en la imagen. Para ello, utilizando MQTT y alguno de los modelos de detección probados, se enviará un mensaje vía MQTT a un broker en la red, cuando se cumplan ciertas condiciones de detección (por ejemplo, un peatón detectado en el centro de la imagen, con una precisión de clasificación superior a un umbral. Esta alarma será recogida por un suscriptor MQTT y reportada al usuario. 

!!! danger "Tarea entregable (15% de la nota)"
    Usando Node-Red o cualquier otro entorno, investiga la posibilidad de enviar un mensaje mediante algún sistema de mensajería (por ejemplo, Telegram) cuando se reciba un aviso de alerta.

!!! danger "Tarea entregable (15% de la nota)"
    Modifica el código para que las imágenes se capten periódicamente desde la cámara de tu ESP-EYE. Además de la alerta vía MQTT, se mostrará en el display del ESP-EYE algún tipo de señal de alerta que permita, sin necesidad de uso de MQTT ni de un agente externo, avisar del peligro al conductor.
