# Laboratorio 1. Introducción a TFLite sobre la Raspberry Pi

## Objetivos

* Familiarizarse con TensorFlow Lite
* Desarrollar una aplicación básica de clasificación de imágenes combinando las APIs de OpenCV y TFLite desde Python y desde C++.
* Acelerar el proceso de inferencia utilizando el acelerador Google Coral.

Puedes obtener los ficheros necesarios para el desarrollo de la práctica [aquí](https://drive.google.com/file/d/1LS1snQqbnz0sQRNRmD-F0KzEMfBA2zov/view?usp=sharing) .

## 1. TensorFlow Lite

Una vez estudiados de forma básica los códigos que nos permiten realizar capturas e interacción desde la cámara, veremos cómo aplicar un modelo pre-entrenado, que nos permitirá realizar un proceso de clasificación de los objetos en el flujo de vídeo. Nótese que el objetivo de esta práctica no es estudiar en profundidad el proceso en sí de clasificación, sino simplemente servir como una primera toma de contacto con la biblioteca [TensorFlow Lite](https://ai.google.dev/edge/litert), recientemente rebautizada como [LiteRT](https://developers.googleblog.com/es/tensorflow-lite-is-now-litert/). 

### ¿Qué es TFLite/LiteRT?

TFLite o LiteRT es un conjunto de herramientas que permiten ejecutar modelos entrenados en [TensorFlow](https://www.tensorflow.org/) en dispositivos móviles, empotrados y en entornos IoT. A diferencia de Tensorflow, TFLite permite realizar procesos de *inferencia* con tiempos de latencia muy reducidos, y un *footprint* también muy reducido. 

TFLite consta de dos componentes principales:

* El [intérprete de TFLite](https://www.tensorflow.org/lite/guide/inference), que ejecuta modelos especialmente optimizados en distintos tipos de *hardware*, incluyendo teléfonos móviles, dispositivos Linux empotrados (e.g. Raspberry Pi) y microcontroladores.

* El [conversor de TFLite](https://www.tensorflow.org/lite/convert/index), que convierte modelos TensorFlow para su posterior uso por parte del intérprete, y que puede introducir optimizaciones para reducir el tamaño del modelo y aumentar el rendimiento.

En este primer laboratorio no incidiremos ni en la creación de modelos ni en su conversión a formato `tflite` propio del *framework* (veremos estas fases en futuros laboratorios); así, partiremos de modelos ya entrenados y convertidos, ya que el único objetivo en este punto es la familiarizarse con el entorno.

Las principales características de interés de TFLite son:

* Un [intérprete](https://www.tensorflow.org/lite/guide/inference) especialmente optimizado para tareas de *Machine Learning* en dispositivos de bajo rendimiento, con soporte para un amplio subconjunto de operadores disponibles en TensorFlow optimizados para aplicaciones ejecutadas en dicho tipo de dispositivos, enfocados a una reducción del tamaño del binario final.

* Soporte para múltiples plataformas, desde dispositivos Android a IOS, pasando por Linux sobre dispositivos empotrados, o microcontroladores.

* APIs para múltiples lenguajes, incluyendo Java, Swift, Objective-C, C++ y Python (estos dos últimos serán de nuestro especial interés).

* Alto rendimiento, con soporte para aceleración hardware sobre dispositivos aceleradores (en nuestro caso, sobre Google Coral) y kernels optimizados para cada tipo de dispositivo.

* Herramientas de optimización de modelos, que incluyen [cuantización](https://www.tensorflow.org/lite/performance/post_training_quantization), técnica qu estudiaremos en futuros laboratorios, imprescindible para integrar el uso de aceleradores como la Google Coral.

* Un formato de almacenamiento eficiente, utilizando [FlatBuffer](https://flatbuffers.dev) optimizado para una reducción de tamaño y en aras de la portabilidad entre dispositivos

* Un conjunto amplio de [modelos pre-entrenados](https://www.tensorflow.org/lite/models) disponibles directamente para su uso en inferencia.

El flujo básico de trabajo cuando estamos desarrollando una aplicación basada en TFLite se basa en cuatro modelos principales:

1. Selección de modelo pre-entrenado o creación/entrenamiento sobre un nuevo modelo. Típicamente utilizando frameworks existentes, como TensorFlow.

2. Conversión del modelo, utilizando el conversor de TFLite desde Python para adaptarlo a las especificidades de TFLite.

3. Despliegue en el dispositivo, utilizando las APIs del lenguaje seleccionado, e invocando al intérprete de TFLite.

4. Optimización del modelo (si es necesaria), utilizando el [Toolkit de Optimización de Modelos](https://www.tensorflow.org/lite/performance/model_optimization), para reducir el tamaño del modelo e incrementar su eficiencia (típicamente a cambio de cierta pérdida en precisión).

### Instalación TFLite

Lo más sencillo es buscar un binario reciente pre-compilado para Raspberry Pi OS 64 *Bookworm* como por ejemplo los proporcionados por el repositorio [tensorflow-lite-raspberrypi](https://github.com/prepkg/tensorflow-lite-raspberrypi?tab=readme-ov-file). 

El proceso de instalación es sencillo:

```sh
$ wget https://github.com/prepkg/tensorflow-lite-raspberrypi/releases/download/2.16.1/tensorflow-lite_64.deb
$ sudo apt install -y ./tensorflow-lite_64.deb
```

## 2. Clasificación de imágenes usando TFLite

En esta parte del laboratorio, mostraremos el flujo de trabajo básico para aplicar un modelo de clasificación (basado en la red neuronal *Mobilenet*), que interactúe con imágenes tomadas directamente desde la cámara web integrada en la Raspberry Pi. 

!!! danger "Tarea"
    Los ficheros que estudiaremos en esta parte están disponibles en el directorio `Clasificacion` del paquete proporcionado.

Como hemos dicho, el objetivo del laboratorio es aplicar inferencia sobre un modelo ya pre-entrenado, por lo que no incidiremos en la estructura interna del mismo. Sin embargo, es conveniente saber que [Mobilenet](https://github.com/tensorflow/models/blob/master/research/slim/nets/mobilenet_v1.md) es una familia de redes neuronales de convolución diseñadas para ser pequeñas en tamaño,
y de baja latencia en inferencia, aplicables a procesos de clasificación, detección o segmentación de imágenes, entre otras muchas aplicaciones. En nuestro caso, la red `Mobilenet v1 1.0_224` es una red de convolución que acepta imágenes de dimensión `224 x 224` y tres canales (RGB), entrenada para devolver la probabilidad de pertenencia a cada una de las 1001 clases para la que ha sido pre-entrenada.

Antes de comenzar, es preciso descargar el modelo, fichero de etiquetas y demás requisitos invocando al script `download.sh` proporcionado:

```sh
$ sh download.sh Modelos
```

Esta ejecución, si todo ha ido bien, descargará en el directorio `Modelos` tres ficheros que utilizaremos en el resto del laboratorio:

* `mobilenet_v1_1.0_224_quant.tflite`: modelo pre-entrenado y cuantizado MobileNet.
* `mobilenet_v1_1.0_224_quant_edgetpu.tflite`: modelo pre-entrenado y cuantizado MobileNet, compilado con soporte para Google Coral.
* `labels_mobilenet_quant_v1_224.txt`: fichero de descripción de etiquetas (clases), con el nombre de una clase por línea. La posición de estas líneas coincide con cada una de las (1001) posiciones del tensor de salida. 

### Desarrollo utilizando Python

!!! warning "Aviso"
    Debian *Bookworm* incluye por defecto Python 3.11 que exige la utilización de entornos virtuales para la gestión de módulos. Por lo tanto antes de ejecutar alguno de los comandos siguientes es preciso crear un entorno virtual, por ejemplo con los siguientes comandos: 

```sh
$ python3 -m venv LSI_venv --system-site-packages
$ source LSI_venv/bin/activate
```

!!! warning "Aviso"
    Al trabajar con entornos virtuales es preciso instalar los paquetes Python necesarios dentro del entorno virtual mediante ejecutando el siguiente comando desde el directorio donde se encuentre el fichero correspondiente: 

```sh
$ python3 -m pip install -r requirements.txt
```

El fichero `classify_opencv.py` contiene el código necesario para realizar inferencia (clasificación) de imágenes partiendo de capturas de fotogramas desde la cámara de la Raspberry Pi, que revisamos paso a paso a continuación:

#### Invocación y argumentos

Observa el inicio de la función `main` proporcionada:

```python
def main():
  parser = argparse.ArgumentParser(
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument(
      '--model', help='File path of .tflite file.', required=True)
  parser.add_argument(
      '--labels', help='File path of labels file.', required=True)
  args = parser.parse_args()
```

El programa recibirá, de forma obligatoria, dos argumentos:

* El modelo a aplicar, en formato `tflite` (FlatBuffer), a través del parámetro `--model`.
* El fichero de etiquetas, en formato texto con una etiqueta por línea. Este fichero no es estrictamente obligatorio, pero nos permite mostrar no sólo el número de clase inferida, sino también su texto asociado.

Así, podremos ejecutar el programa directamente utilizando la orden (suponiendo que ambos ficheros residen en el directorio
`../Modelos`):

```sh
python3 classify_opencv.py  --model ../Modelos/mobilenet_v1_1.0_224_quant.tflite   \
	--labels ../Modelos/labels_mobilenet_quant_v1_224.txt
```

La función `load_labels` simplemente lee el fichero de etiqueta y las almacena en memoria para su posterior procesamiento tras la inferencia.

#### Preparación del intérprete TFLite

El siguiente paso es la preparación del intérprete de TFLite:

```python
interpreter = Interpreter(args.model)
```

Observa que el único parámetro proporcionado es el nombre del modelo a cargar en formato TFLite. Observa también que necesitaremos cargar los módulos correspondientes a TFLite antes de hacer uso de esta función:

```python
from tflite_runtime.interpreter import Interpreter
```

A continuación, obtenemos información sobre el tensor de entrada del modelo recién cargado, utilizando la función `get_input_details`, y consultando la propiedad `shape` de dicha entrada. Esto nos devolverá en las variables `widght` y `height` los tamaños de imagen esperados por el modelo. A partir de ahora, puedes consultar la forma de trabajar con la API de Python a través de la [documentación oficial](https://www.tensorflow.org/lite/api_docs/python/tf/lite).

Utilizaremos esta información para redimensionar la imagen capturada de la cámara como paso previo a la invocación del modelo.

!!! danger "Tarea"
    Asegúrate de que ambas líneas (creación del intérprete e importación de bibliotecas) son correctas. Comprueba el correcto funcionamiento del código utilizando el modelo `mobilenet_v1_1.0_224_quant.tflite` como entrada al programa `classify_opencv.py` sobre tu Raspberry Pi. Si todo ha ido bien, deberías ver una ventana mostrando la salida de la cámara con cierta información sobreimpresionada, y para cada fotograma, el resultado de la inferencia a través de línea de comandos.

#### Inferencia

En el bucle principal de captura, se invoca a la función `classify_image`. Esta es una función propia, que
recibe simplemente el intérprete TFLite construido y la imagen capturada, pero cuyo cuerpo
contiene cierta funcionalidad de interés:

```python
## Invoke model and process output (quantization-aware).
def classify_image(interpreter, image, top_k=1):
  """Returns a sorted array of classification results."""
  set_input_tensor(interpreter, image)

  interpreter.invoke()

  output_details = interpreter.get_output_details()[0]
  output = np.squeeze(interpreter.get_tensor(output_details['index']))

  # If the model is quantized (uint8 data), then dequantize the results
  if output_details['dtype'] == np.uint8:
    scale, zero_point = output_details['quantization']
    output = scale * (output - zero_point)

  ordered = np.argpartition(-output, top_k)
  return [(i, output[i]) for i in ordered[:top_k]]
```

Observa que la función opera en varias fases. En primer lugar, se obtiene una referencia al tensor de entrada
del modelo (invocando a la función propia `set_input_tensor`). En este caso, se copia, elemento a elemento,
la imagen de entrada (`image`) a dicho tensor:

```python
## Establish input tensor from an image (copying).
def set_input_tensor(interpreter, image):
  tensor_index = interpreter.get_input_details()[0]['index']
  input_tensor = interpreter.tensor(tensor_index)()[0]
  input_tensor[:, :] = image
```

Volviendo a la función `classify_image`, una vez copiada la entrada al tensor de entrada del modelo, se invoca al modelo TFLite (`interpreter.invoke()`). Este es el proceso de inferencia o aplicación del modelo, y su tiempo de respuesta es crítico.

Por último, se procesa la salida (tensor de salida). En caso de ser una salida cuantizada (esto es, el tipo de cada elemento del array de salida es *uint8_t*, veremos más sobre cuantización en futuros laboratorios), ésta debe procesarse de forma acorde a los parámetros de cuantización utilizados.

Al final, la función devolverá un array de tuplas con la posición/clase (`i`) y la probabilidad de pertenencia del objeto
observado a dicha clase (`output[i]`).

!!! danger "Tarea"
    Imprime por pantalla la información sobre el tensor de entrada y el tensor de salida y analiza la salida proporcionada.


Obsérvese que, de forma previa a la invocación del modelo, la imágen capturada se ha reescalado de forma acorde al tamaño
del tensor de entrada del modelo:

```python
image = cv2.resize(frame, (224, 224), interpolation = cv2.INTER_AREA)
```

#### Postprocesamiento

Por último, el programa sobreimpresiona información sobre etiqueta de clasificación, probabilidad de pertenencia a la clase y 
tiempo de inferencia sobre el propio *frame*, mostrando la imagen resultante:

```python
cv2.putText(frame, '%s %.2f\n%.1fms' % ( labels[label_id], prob, elapsed_ms ), bottomLeftCornerOfText, font, fontScale, fontColor, lineType)

cv2.imshow('image',frame)
```

### Desarrollo utilizando C++

El rendimiento es un factor determinante en aplicaciones *Edge computing*, por lo que resultará interesante disponer de una base desarrollada en C++ sobre la que trabajar para el ejemplo de clasificación.

El fichero `classification.cpp` proporciona un flujo de trabajo completo para realizar una clasificación de imágenes similar a la realizada anteriormente usando la API de Python. 

El código desarrollado es similar, paso a paso, al descrito para Python, por lo que no se incidirá en los detalles más allá de la [API utilizada](https://www.tensorflow.org/lite/api_docs/cc):

#### Ficheros de cabecera

Incluiremos ficheros de cabecera genéricos, para OpenCV y para TFLite:

```cpp
#include <stdio.h>

// Cabeceras TFLite 
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

// Cabeceras OpenCV
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

// Otras cabeceras C++
#include <vector>
#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort
```

#### Carga del modelo desde un fichero

En este ejemplo, se realiza la carga del modelo directamente desde un fichero en disco, utilizando la rutina `BuildFromFile`:

```cpp
   // 1. Cargamos modelo desde un fichero.
   std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile("../Modelos/mobilenet_v1_1.0_224_quant.tflite");

   if(!model){
        printf("Failed to mmap model\n");
        exit(0);
   }

   // 2. Construimos el iterprete TFLite.
   tflite::ops::builtin::BuiltinOpResolver resolver;
   std::unique_ptr<tflite::Interpreter> interpreter;
   tflite::InterpreterBuilder(*model.get(), resolver)(&interpreter);

   // 3. Alojamos espacio para tensores.
   interpreter->AllocateTensors();
```

Observa que, en el anterior fragmento de código, además de la carga del modelo, se construye un intérprete utilizando la clase `InterpreterBuilder`, y se aloja espacio para los tensores necesarios para aplicar el modelo.

#### Caracaterización de tensores de entrada y salida

Como hemos hecho en el código Python, será necesario realizar una caracterización de los tensores de entrada y salida. El primero, para copiar nuestra imagen capturada desde cámara; el segundo, para procesar la salida obtenida:

```cpp
    // 4. Identificamos el tensor de entrada y de salida.
    int input_number  = interpreter->inputs()[0];
    uint8_t * input_tensor  = interpreter->typed_tensor<uint8_t>(input_number);

    int output_number = interpreter->outputs()[0];
    uint8_t * output_tensor = interpreter->typed_tensor<uint8_t>(output_number);
```

!!! danger "Tarea"
    ¿Cuál es el índice de los tensores de entrada y salida generados? Coinciden con los observados al mostrar por pantalla la información sobre ellos en el código Python.

#### Invocación del modelo TFLite

Tras comenzar con la captura de vídeo y redimensionar la imagen de entrada, copiaremos al tensor de entrada
la imagen capturada, pixel a pixel:

```cpp
   // 8. Copiamos imagen al tensor de entrada.
   for (int i = 0; i < 224*224; ++i) {
      input_tensor[3*i + 0] = frame.at<cv::Vec3b>(i)[0];
      input_tensor[3*i + 1] = frame.at<cv::Vec3b>(i)[1];
      input_tensor[3*i + 2] = frame.at<cv::Vec3b>(i)[2];
   }
```

A continuación, invocamos al modelo:


```cpp
   // 9. Invocamos al modelo.
   if (interpreter->Invoke() != kTfLiteOk) {
     cerr << "Failed to invoke tflite!";
     exit(-1);
   }
```

#### Clasificación y análisis de salida

El proceso de análisis de salida es ligeramente distinto al usado en Python, aunque sigue una filosofía similar. En primer lugar, analizamos el tensor de salida:

```cpp
   // 10. Analizamos el tamaño del tensor de salida.
   TfLiteIntArray* output_dims = interpreter->tensor(output_number)->dims;
   auto output_size = output_dims->data[output_dims->size - 1];
   cout << output_size << endl;
```

Como en el caso de Python, en función de la cuantización de la salida, deberemos procesarla de forma acorde (trataremos la cuantización en futuros laboratorios). En cualquier caso, el array `logits` contiene la probabilidad de pertenencia a cada una de las 1001 clases disponibles. El código que se os proporciona ordena dicho array y muestra por pantalla la clase más probable, junto a su probabilidad asociada.

#### Compilación y uso

A continuación, compila y ejecuta el programa para validar su funcionamiento:

```sh
g++ classification.cpp -ltensorflow-lite -lpthread -ldl `pkg-config --cflags --libs opencv4` -o classification.x

./classification.x
```

La salida está preparada para mostrar el código numérico de la clase detectada con mayor probabilidad, dicha probabilidad, y la descripción textual de la clase.

!!! danger "Tarea"
    Temporiza, utilizando la rutinas de la clase `chrono` de C++, el proceso de inferencia, y compáralo con el de la versión Python.

!!! danger "Tarea"
    De forma opcional, investiga cómo sobreimpresionar la información asociada al proceso de inferencia (clase, probabilidad y tiempo) de forma similar a cómo lo hicimos en Python.

## Uso de Google Coral

### Runtime EdgeTPU

Para poder usar la Google Coral con TFLite es necesario instalar la librería correspondiente (`libedgetpu`). Las fuentes de esta librería están disponibles el [GitHub de Google-Coral](https://github.com/google-coral/libedgetpu) pero lo más sencillo es descargar binarios pre-compilados para `bookworm` y `arm64` (también denominado `aarch64`). 

Instrucciones de instalación:

```sh
$ wget https://github.com/feranick/libedgetpu/releases/download/16.0TF2.16.1-1/libedgetpu-dev_16.0tf2.16.1-1.bookworm_arm64.deb
$ wget https://github.com/feranick/libedgetpu/releases/download/16.0TF2.16.1-1/libedgetpu1-max_16.0tf2.16.1-1.bookworm_arm64.deb
$ sudo dpkg -i libedgetpu*
```

### Uso de EdgeTPU desde Python

Para utilizar el acelerador Google Coral (que debe estar conectado a la Raspberry Pi), realizaremos ciertas modificaciones en 
el código, que en este caso son mínimas. 

En primer lugar, añadiremos un `import` en nuestro fichero:

```python
from tflite_runtime.interpreter import load_delegate
```

A continuación, reemplazaremos la construcción del intérprete por la especificación de una biblioteca delegada para realizar la inferencia sobre la Google Coral:

```python
interpreter = Interpreter(args.model,
    experimental_delegates=[load_delegate('libedgetpu.so.1.0')])
```


Finalmente, será necesario aplicar un modelo especialmente compilado para la Google Coral. Normalmente, este modelo se obtiene utilizando el [compilador de la Edge TPU](https://coral.withgoogle.com/docs/edgetpu/compiler/), pero en este caso se descarga y proporciona mediante el script `download.sh`. El nombre del modelo es `mobilenet_v1_1.0_224_quant_edgetpu.tflite`.

Así, podremos ejecutar sobre la Edge TPU usando:

```sh
sudo python3 classify_opencv.py \
  --model ../Modelos/mobilenet_v1_1.0_224_quant_edgetpu.tflite \
  --labels ../Modelos/labels_mobilenet_quant_v1_224.txt
```

Obviamente, para que el anterior comando tenga éxito, la Google Coral deberá estar conectada a la Raspberry Pi  y el usuario que lo ejecuta debe tener permisos para usar el dispositivo. Estos permisos se pueden otorgar mediante la siguiente secuencia de comandos:

```sh
$ lsusb -d 18d1:9302
Bus 002 Device 003: ID 18d1:9302 Google Inc.
$ echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="9302", MODE="0666", GROUP="plugdev"' | sudo tee /etc/udev/rules.d/99-edgetpu.rules
$ sudo udevadm control --reload-rules && sudo udevadm trigger
```

!!! warning "Aviso"
    Es posible que para que el cambio surta efecto sea necesario salir y volver a entrar en la sesión.

!!! danger "Tarea"
    Compara los tiempos de ejecución de la inferencia utilizando el procesdor de propósito general frente al rendimiento utilizando la Google Coral. ¿Qué ganancia de rendimiento observas? Ajusta la configuración de la captura de imagenes para que la comparación sea lo más justa:

### Uso de EdgeTPU desde C++

Para usar la Google Coral desde C++ es preciso realizar algunas modificaciones al código visto anteriormente ademas de incluir la correspondiente cabecera (`<edgetpu.h>`). Para más información consultar la [documentación de la API de `libedgetpu`](https://coral.ai/docs/reference/cpp/edgetpu/).

#### Apertura del dispositivo

Antes de poder trabajar con la EdgeTPU es preciso abrir el dispositivo con la función `OpenDevice` de la clase `EdgeTpuManager`:

```c++
    auto tpu_context = edgetpu::EdgeTpuManager::GetSingleton()->OpenDevice();
```

#### Registrar el manejador de EdgeTPU en el resolutor

A continuación es preciso añadir un operador personalizado para que TensorFlow Lite lo reconozca:

```c++
    tflite::ops::builtin::BuiltinOpResolver resolver;
    resolver.AddCustom(edgetpu::kCustomOp, edgetpu::RegisterCustomOp());
```

#### Vincular el contexto de EdgeTPU con el intérprete

Por último es necesario vincular el contexto del dispositivo abierto previamente con el intérprete de TFLite:

````c++
    interpreter->SetExternalContext(kTfLiteEdgeTpuContext,tpu_context.get());
````

Por lo demás el código es esencialmente el mismo que anteriormente.

#### Compilación 

Para poder compilar el código es preciso enlazar con la libraría `libedgetpu`:

````c++
g++ classification.cpp -o classification.x `pkg-config --cflags --libs opencv4` -ltensorflow-lite -lpthread -ldl
````

!!! danger "Tarea"
    Compara las versiones C++ CPU vs Google Coral ¿Qué ganancia de rendimiento observas? Compara el resultado con el obtenido en Python.
