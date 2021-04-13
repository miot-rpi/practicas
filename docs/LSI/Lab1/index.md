# Laboratorio 1. Introducción a TFLite sobre la Raspbery Pi

## Objetivos

* Estudiar las APIs Python y C++ de OpenCV para realizar capturas desde cámara y para realizar transformaciones básicas en imágenes capturadas.
* Desarrollar una aplicación básica de clasificación de imágenes combinando las APIs de OpenCV y TFLite desde Python y desde C++.
* Acelerar el proceso de inferencia utilizando el acelerador Google Coral.
* Diseñar e implementar una aplicación básica para detección de objetos.

## Introducción

## Interacción con la cámara vía OpenCV

Existen múltiples APIs para acceder a la cámara proporcionada junto a la Raspberry Pi en el entorno experimental. 
[Picamera](https://picamera.readthedocs.io/en/release-1.13/) es una API específica y de código abierto 
que proporciona una interfaz Python para el módulo de cámara de la Raspberry Pi. El problema radica en su 
especificidad para este tipo de *hardware*, que limita su aplicación o portabilidad a otros.

OpenCV proporciona, como parte integral de su API básica (tanto desde 
[Python](https://docs.opencv.org/master/d8/dfe/classcv_1_1VideoCapture.html) como desde 
[C++](https://docs.opencv.org/master/d8/dfe/classcv_1_1VideoCapture.html))
una interfaz completa para la interacción con cámaras web, que se describe a continuación.

### Captura de vídeos desde la cámara

En primer lugar, realizaremos una captura de flujo de vídeo desde la cámara, utilizando
la API de OpenCV en Python, realizando una transformación básica y mostrando el flujo de
vídeo transformado.

En Python, para realizar la captura de vídeo, necesitaremos un objeto de tipo
`VideoCapture`; su único argumento puede ser o bien un índice de dispositivo o un fichero
de vídeo. Un índice de dispositivo es simplemente un identificador único para cada una de las
cámaras conectadas al equipo: 

```python
import numpy as np
import cv2

# 0. Configura camara 0.
cap = cv2.VideoCapture(0)

while(True):
    # 1. Adquisicion de frame (TODO: temporizar la adquisicion y reportar Frames por Segundo (FPS)). 
    ret, frame = cap.read()

    # 2. Operaciones sobre el frame (transformaciones. TODO: investigar resize y otras transformaciones).
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # 3. Mostramos el frame resultante.
    cv2.imshow('frame',gray)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 4. Liberamos la captura y destruimos ventanas.
cap.release()
cv2.destroyAllWindows()
``` 

`cap.read()` devuelve un valor *booleano* en función de si el *frame* fue leído correctamente o no. Podemos, 
por tanto, comprobar la finalización de un flujo de vídeo utilizando dicho valor de retorno.

Es posible acceder  aalgunas de las caracterísitcas del vídeo utilizando el método
`cap.get(propId)`, donde `propId` es un número entre 0 y 18. Cada número denota una 
propiedad del vídeo (si dicha propiedad se puede aplicar al vídeo en cuestión). Para más
información, consulta 
[la página de documentación de OpenCV](https://docs.opencv.org/master/d8/dfe/classcv_1_1VideoCapture.html#aa6480e6972ef4c00d74814ec841a2939). Algunos de estos valores pueden ser modificados a través de la función `cap.set(propId, value)`.

Por ejemplo, podemos comprobar la anchura y altura de un *frame* utilizando
`cap.get(cv.CAP_PROP_FRAME_WIDTH)` y `cap.get(cv.CAP_PROP_FRAME_HEIGHT)`. Podemos, por ejemplo, fijar
la resolución de la captura utilizando
`ret = cap.set(cv.CAP_PROP_FRAME_WIDTH,320)` y 
`ret = cap.set(cv.CAP_PROP_FRAME_HEIGHT,240)`.

!!! danger "Tarea"
    Ejecuta el anterior script (está incluido en el paquete proporcionado) utilizando `python3`. Estudia el código y modifícalo
    para introducir nuevas transformaciones en las imágenes capturadas (tranformación a otros espacios de color, redimensionado 
    de imágenes, etc.) Para ello, deberás consultar la documentación de OpenCV.

!!! danger "Tarea"
    Temporiza el tiempo de adquisición (`cap.read()`) y repórtalo a través de línea de comandos, reportando no sólo el tiempo,
    sino los fotogramas por segundo (FPS) obtenidos. 

Desde C++, la lógica de captura es muy similar, como también lo es la API utilizada:

```cpp
// OpenCV includes.
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

// Other includes.
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;

int main(int, char**)
{
    // 0. Declaracion de variables (vease documentacion de Mat).
    Mat frame;
    VideoCapture cap;

    // 1. Configuramos camara 0.
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API

    cap.open(deviceID, apiID);

    // 2. Check error.
    if (!cap.isOpened()) {
        cerr << "ERROR abriendo camara.\n";
        return -1;
    }

    // 3. Bucle de adquisicion.
    cout << "Comenzando adquisicion..." << endl << "Presiona cualquier tecla para terminar..." << endl;
    for (;;)
    {
        // 4. Adquisicion de frame (TODO: temporizar la adquisicion y reportar Frames por Segundo (FPS)). 
        cap.read(frame);

        // TODO: Investigar transformaciones en C++ (ver documentacion).

        // 5. Check error.
        if (frame.empty()) {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        // 6. Mostramos frame en ventana.
        imshow("Stream", frame);
        if (waitKey(5) >= 0)
            break;
    }

    // 7. La camara se liberara en el destructor.
    return 0;
}
```

Para compilar y enlazar el anterior código, nos ayudaremos de la herramienta `pkg-config`, que nos ayudará a
fijar los flags de compilación y enlazado para programas que utilicen OpenCV. Como curiosidad, observa la salida
de la siguiente ejecución:

```sh
pi@raspberrypi:~/Test/Lab1/Camera/CPP $ pkg-config --cflags --libs opencv4
-I/usr/local/include/opencv4 -L/usr/local/lib -lopencv_gapi -lopencv_stitching -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_highgui -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core
```

Estas son las opciones, flags y bibliotecas que será necesario incluir en el proceso de compilación de cualquier prorama
OpenCV. Así, para compilar el anterior programa, puedes utilizar la orden:

```sh
g++ programa.cpp -o programa.x `pkg-config --cflags --libs opencv4`
```

!!! danger "Tarea"
    Compila y ejecuta el anterior programa (está incluido en el paquete proporcionado). Estudia el código y modifícalo
    para introducir nuevas transformaciones en las imágenes capturadas (tranformación a otros espacios de color, redimensionado 
    de imágenes, etc). Para ello, deberás consultar la documentación de OpenCV.

!!! danger "Tarea"
    Temporiza el tiempo de adquisición (`cap.read(frame)`) y repórtalo a través de línea de comandos, mostrando no sólo el tiempo,
    sino los fotogramas por segundo (FPS) obtenidos. Tanto en el caso de C++ como de Python, experimenta con distintas resoluciones
    de captura. ¿Cuál es la resolución máxima soportada por la cámara que os proporcionamos?

### Reproducción de un vídeo desde fichero

Para reproducir un vídeo almacenado en un fichero, el procedimiento a seguir es similar a la captura desde cámara, 
pero reemplazando el índice de cámara por un nombre de fichero. Utilizaremos la función `cv.waitKey()` con un valor
apropiado: si es demasiado corto, el vídeo se reproducirá demasiado rápidamente; 
si es demasiado largo, el vídeo se reproducirá demasiado lentamente:

```python
import numpy as np
import cv2 as cv

cap = cv.VideoCapture('vtest.avi')

while cap.isOpened():
    ret, frame = cap.read()

    if not ret:
        print("Error, saliendo...")
        break

    gray = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)

    cv.imshow('frame', gray)

    if cv.waitKey(1) == ord('q'):
        break

cap.release()
cv.destroyAllWindows()
```

### Almacenamiento de un vídeo en disco

Para almacenar un *frame* capturado desde cámara en disco, utilizaremos `cv.imwrite()` (busca y estudia su 
documentación, y modifica los códigos anteriores para realizar una "fotografía" desde la cámara).

Para almacenar un vídeo, es necesario algo de trabajo adicional. En primera lugar, crearemos un objeto de tipo
`VideoWriter`, especificando el nombre del fichero de salida deseado. A continuación, especificaremos el llamado
código *FourCC*, resolución deseada y frames por segundo deseados. El último de los argumentos de la invocación
es el flag `isColor`, que nos permitirá determinar si el codificador de vídeo espera un frame en color o en escala
de grises (véase la documentación de la función para más información). 

[FourCC](http://en.wikipedia.org/wiki/FourCC) es un código de 4 bytes que permite especificar el codec de vídeo
deseado. Puedes obtener la lista de todos los codigos en [fourcc.org](fourcc.org). Así, por ejemplo, para utilizar
XVID, usaríamos:

```python
fourcc = cv.VideoWriter_fourcc(*'XVID')
```

El siguiente código captura desde la cámara, rota cada frame en la dirección vertical, y almacena el vídeo resultante:

```python
import numpy as np
import cv2 as cv

cap = cv.VideoCapture(0)

# Definimos el codec y creamos el objeto VideoWriter.
fourcc = cv.VideoWriter_fourcc(*'XVID')
out = cv.VideoWriter('output.avi', fourcc, 20.0, (640,  480))

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        print("Error, saliendo...")
        break
    frame = cv.flip(frame, 0)
    
    # Escribimos el frame rotado.
    out.write(frame)
    cv.imshow('frame', frame)
    if cv.waitKey(1) == ord('q'):
        break

cap.release()
out.release()

cv.destroyAllWindows()
```

## TensorFlow Lite

Una vez estudiados de forma básica los códigos que nos permiten realizar capturas e interacción desde la cámara 
proporcionada, veremos cómo aplicar un modelo preentrenado al mismo, que nos permitirá realizar un proceso de 
**clasificación** de los objetos mostrados en el flujo de vídeo. Nótese que el objetivo de esta práctica no es
estudiar en profundidad el proceso en sí de clasificación, sino simplemente servir como una primera toma de 
contacto con la biblioteca [TensorFlow Lite](tensorflow.org/lite). TFLite es un conjunto de herramientas que 
perimtan ejecutar modelos entrenados TensorFlow en dispostivos móviles, empotrados y en entornos IoT. A diferencia
de Tensorflow, TFLite permite realizar procesos de **inferencia** con tiempos de latencia muy reducidos, y un
*footprint* también muy reducido. 

TFLite consta de dos componentes principales:

* El [intérprete de TFLite](https://www.tensorflow.org/lite/guide/inference), que ejecuta modelos especialmente
optimizados en distintos tipos de *hardware*, incluyendo teléfonos móviles, dispositivos Linux empotrados (e.g. Raspberry Pi) y
microcontroladores.

* El [conversor de TFLite](https://www.tensorflow.org/lite/convert/index), que convierte modelos TensorFlow para su posterior uso por parte del intérprete, y que puede introducir optimizaciones para reducir el tamaño del modelo y aumentar el rendimiento.

En este primer laboratorio no incidiremos ni en la creación de modelos ni en su conversión a formato `tflite` propio del 
*framework* (veremos estas fases en futuros laboratorios); así, partiremos de modelos ya entrenados y convertidos, ya que
el único objetivo en este punto es la familiarización con el entorno.

Las principales características de interés de TFLite son:

* Un [intérprete](https://www.tensorflow.org/lite/guide/inference) especialmente optimizado para tareas de 
*Machine Learning* en dispositivos de bajo rendimiento, con soporte para un amplio subconjunto de operadores disponibles
en TensorFlow optimizados para aplicaciones ejecutadas en dicho tipo de dispositivos, enfocados a una reducción del tamaño 
del binario final.

* Soporte para múltiples plataformas, desde dispositivos Android a IOS, pasando por Linux sobre dispositivos empotrados, o
microcontroladores.

* APIs para múltiples lenguajes, incluyendo Java, Swift, Objective-C, **C++** y **Python** (estos dos últimos serán de nuestro especial interés).

* Alto rendimiento, con soporte para **aceleración hardware** sobre dispositivos aceleradores (en nuestro caso, sobre Google Coral) y kernels optmizados para cada tipo de dispositivo.

* Herramientas de optimización de modelos, que incluyen [cuantización](https://www.tensorflow.org/lite/performance/post_training_quantization), técnica qu estudiaremos en futuros laboratorios, imprescindible para integrar el uso de aceleradores como la Google Coral.

* Un formato de almacenamiento eficiente, utilizando [FlatBuffer](https://www.tensorflow.org/lite/convert/index) optimizado para una reducción de tamaño y en aras de la portabilidad entre dispositivos

* Un conjunto amplio de [modelos preentrenados](https://www.tensorflow.org/lite/models) disponibles directamente para su uso en inferencia.

El flujo básico de trabajo cuando estamos desarrollando una aplicación basada en TFLite se basa en cuatro modelos principales:

1. Selección de modelo preentrenado o creación/entrenamiento sobre un nuevo modelo. Típicamente utilizando frameworks existentes, como TensorFlow.

2. Conversión del modelo, utilizando el conversor de TFLite desde Python para adaptarlo a las especificidades de TFLite.

3. Despliegue en el dispositivo, utilizando las APIs del lenguaje seleccionado, e invocando al intérprete de TFLite.

4. Optimización del modelo (si es necesaria), utilizando el [Toolkit de Optimización de Modelos](https://www.tensorflow.org/lite/performance/model_optimization), para reducir el tamaño del modelo e incrementar su eficiencia (típicamente a cambio de cierta pérdida en precisión).

## Clasificación básica de imágenes usando TFLite

En esta parte del laboratorio, mostraremos el flujo de trabajo básico para aplicar un modelo de clasificación (basado
en la red neuronal Mobilenet), que interactúe con imágenes tomadas directamente desde la cámara web integrada en la Raspberry
Pi. 

!!! danger "Tarea"
    Los ficheros que estudiaremos en esta parte están disponibles en el directorio `Clasificacion` del paquete proporcionado.

Como hemos dicho, el objetivo del laboratorio es aplicar inferencia sobre un modelo ya preentrenado, por lo que no incidiremos
en la estructura interna del mismo. Sin embargo, es conveniente saber que [Mobilenet](https://github.com/tensorflow/models/blob/master/research/slim/nets/mobilenet_v1.md) es una familia de redes neuronales de convolución diseñadas para ser pequeñas en tamaño,
y de baja latencia en inferencia, aplicables a procesos de clasificación, detección o segementación de imágenes, entre otras
muchas aplicaciones. En nuestro caso, la red `Mobilenet v1 1.0_224` es una red de convolución que acepta imágenes
de dimensión `224 x 224` y tres canales (RGB), entrenada para devolver la probabilidad de pertenencia a cada una de las 1001
clases para la que ha sido preentrenada.

Antes de comenzar, descarga el modelo, fichero de etiquetas y demás requisitios invocando al script `download.sh` proporcionado:

```sh
bash download.sh Modelos
```

Esta ejecución, si todo ha ido bien, descargará en el directorio `Modelos` tres ficheros que utilizaremos en el resto del laboratorio:

* `mobilenet_v1_1.0_224_quant.tflite`: modelo preentrenado y cuantizado MobileNet.
* `mobilenet_v1_1.0_224_quant_edgetpu.tflite`: modelo preentrenado y cuantizado MobileNet, compilado con soporte para Google Coral.
* `labels_mobilenet_quant_v1_224.txt`: fichero de descripción de etiquetas (clases), con el nombre de una clase por línea. La posición de estas líneas coincide con cada una de las (1001) posiciones del tensor de salida. 

### Desarrollo utilizando Python

El código `classify_opencv.py` contiene el código necesario para realizar inferencia (clasificación) de imágenes partiendo de 
capturas de fotogramas desde la cámara de la Raspberry Pi, que revisamos paso a paso a continuación:

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
python3 classify_opencv.py  --model ../Modelos/mobilenet_v1_1.0_224_quant_edgetpu.tflite   --labels ../Modelos/labels_mobilenet_quant_v1_224.txt
```

!!! danger "Tarea"
    Comprueba el correcto funcionamiento del código sobre tu Raspberry Pi. Si todo ha ido bien, deberías ver una ventana mostrando la salida de la cámara con cierta información sobreimpresionada, y para cada fotograma, el resultado de la inferencia a través de línea de comandos.

La función `load_labels` simplemente lee el fichero de etiqueta y las almacena en memoria para su posterior procesamiento tras la inferencia.

#### Preparación del intérprete TFLite

El siguiente paso es la preparación del intérprete de TFLite:

```python
  interpreter = Interpreter(args.model)
```

Observa que el único parámetro proporcionado es el nombre del modelo a cargar en formato TFLite. 
Observa también que necesitaremos cargar los módulos correspondientes a TFLite antes de hacer uso de esta función:

```python
from tflite_runtime.interpreter import Interpreter
```

A continuación, obtenemos información sobre el tensor de entrada del modelo recién cargado, utilizando la función 
`get_input_details`, y consultando la propiedad `shape` de dicha entrada. Esto nos devolverá en las variables 
`widght` y `height` los tamaños de imagen esperados por el modelo. A partir de ahora, puedes consultar la forma de
trabajar con la API de Python a través de la [documentación oficial](https://www.tensorflow.org/lite/api_docs/python/tf/lite).

Utilizaremos esta información para redimensionar la imagen capturada de la cámara como paso previo a la invocación del modelo.

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

Volviendo a la función `classify_image`, una vez copiada la entrada al tensor de entrada del modelo, se invoca
al modelo TFLite (`interpreter.invoke()`). **Este es el proceso de inferencia o aplicación del modelo**, y su tiempo
de respuesta es crítico.

Por último, se procesa la salida (tensor de salida). En caso de ser una salida cuantizada (esto es, el tipo
de cada elemento del array de salida es *uint8_t*, veremos más sobre cuantización
en futuros laboratorios), ésta debe procesarse de forma acorde a los parámetros de cuantización 
utilizados.

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

### Uso de Google Coral

Para utilizar el acelerador Google Coral (que debe estar conectado a la Raspberry Pi), realizaremos ciertas modificaciones en 
el código, que en este caso son mínimas. 

En primer lugar, añadiremos un `import` en nuestro fichero:

```python
from tflite_runtime.interpreter import load_delegate
```

A continuación, reemplazaremos la construcción del intérprete:

```python
interpreter = Interpreter(args.model)
```

Por la especificación de una biblioteca delegada para realizar la inferencia sobre la Google Coral:

```python
interpreter = Interpreter(args.model,
    experimental_delegates=[load_delegate('libedgetpu.so.1.0')])
```

El fichero `libedgetpu.so.1.0` viene ya instalado en la imagen proporcionada, aunque es directamente instalable siguiendo las instrucciones de instalación proporcionadas por el fabricante.


Finalmente, será necesario aplicar un modelo especialmente compilado para la Google Coral. Normalmente, este modelo se obtiene 
utilizando el [compilador de la Edge TPU](https://coral.withgoogle.com/docs/edgetpu/compiler/), pero en este caso se descarga y proporciona mediante el script `download.sh`. El nombre del modelo es `mobilenet_v1_1.0_224_quant_edgetpu.tflite`.

Así, podremos ejecutar sobre la Edge TPU usando:

```sh
python3 classify_opencv.py \
  --model /tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite \
  --labels /tmp/labels_mobilenet_quant_v1_224.txt
```

!!! danger "Tarea"
    Compara los tiempos de ejecución de la inferencia utilizando el procesdor de propósito general frente al rendimiento utilizando la Google Coral. ¿Qué ganancia de rendimiento observas?

### Desarrollo utilizando C++

El rendimiento es un factor determinante en aplicaciones *Edge computing*, por lo que resultará interesante disponer de una 
base desarrollada en C++ sobre la que trabajar para el ejemplo de clasificación.

El fichero `classification.cpp` proporciona un flujo de trabajo completo para realizar una clasificación de imágenes similar a la realizada anteriormente usando la API de Python. 

En primer lugar, compila y ejecuta el programa para validar su funcionamiento:

```sh
g++ classification.cpp -I /home/pi/tensorflow/ /home/pi/tensorflow/tensorflow/lite/tools/make/gen/linux_aarch64/lib/libtensorflow-lite.a -lpthread -ldl `pkg-config --cflags --libs opencv4` -o classification.x

./classification.x
```

Observa que se utiliza la biblioteca `libtensorflow-lite.a`, disponible en cualquier instalación TensorFlow.

La salida está preparada para mostrar el código numérico de la clase detectada con mayor probabilidad, dicha probabilidad, y 
la descripción textual de la clase.

El código desarrollado es similar, paso a paso, al descrito para Python, por lo que no se incidirá en los detalles más allá
de la [API utilizada](https://www.tensorflow.org/lite/api_docs/cc):

#### Ficheros de cabecera

Incluiremos ficheros de cabecera genéricos, para OpenCV y para TFLite:

```cpp
#include <stdio.h>

// TFLite includes.
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/tools/gen_op_registration.h"

// OpenCV includes.
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

// C++ sorting utils.
#include <vector>
#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort
```

#### Carga del modelo desde un fichero

En este ejemplo, se realiza la carga del modelo directamente desde un fichero en disco, utilizando la 
rutina `BuildFromFile`:

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

Observa que, en el anterior fragmento de código, además de la carga del modelo, se construye un intérprete utilizando
la clase `InterpreterBuilder`, y se aloja espacio para los tensores necesarios para aplicar el modelo.

#### Caracaterización de tensores de entrada y salida

Como hemos hecho en el código Python, será necesario realizar una caracterización de los tensores de entrada y salida. El primero,
para copiar nuestra imagen capturada desde cámara; el segundo, para procesar la salida obtenida:

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

Como en el caso de Python, en función de la cuantización de la salida, deberemos procesarla de forma acorde (trataremos la cuantización en futuros laboratorios). 
En cualquier caso, el array `logits` contiene la probabilidad de pertenencia a cada una de las 1001 clases disponibles. El código que se os proporciona ordena dicho array y muestra por pantalla la clase más probable, junto a su probabilidad asociada.

!!! danger "Tarea"
    Temporiza, utilizando la rutinas de la clase `chrono` de C++, el proceso de inferencia, y compáralo con el de la versión Python.

!!! danger "Tarea"
    De forma opcional, investiga cómo sobreimpresionar la información asociada al proceso de inferencia (clase, probabilidad y tiempo) de forma similar a cómo lo hicimos en Python.

En futuros laboratorios veremos también cómo utilizar el acelerador Google Coral desde código C++.

## Detección de objetos usando TFLite

Como segundo caso de uso, proporcionamos un programa completo para la detección de objetos utilizando una red neuronal de convolución ya entrenada (MobileNet SSD). El ejemplo proporcionado, que utiliza la API de Python, es equivalente en estructura al observado para clasificación de imágenes, pero en este caso, la red neuronal nos proporciona cuatro tensores de salida en lugar de uno, como puedes observar en la función `detect_objects`:

```python
def detect_objects(interpreter, image, threshold):
  """Returns a list of detection results, each a dictionary of object info."""
  set_input_tensor(interpreter, image)
  interpreter.invoke()

  # Get all output details
  boxes = get_output_tensor(interpreter, 0)
  classes = get_output_tensor(interpreter, 1)
  scores = get_output_tensor(interpreter, 2)
  count = int(get_output_tensor(interpreter, 3))

  results = []
  for i in range(count):
    if scores[i] >= threshold:
      result = {
          'bounding_box': boxes[i],
          'class_id': classes[i],
          'score': scores[i]
      }
      results.append(result)
  return results
```

Observa que, tras la invocación del modelo, se obtienen cuatro tensores de salida, que respectivamente
almacenan las *cajas* o *bounding boxes* de los objetos detectados, la identificación de la clase a la que
pertenecen, el *score* de pertenencia a dichas clases y el número de objetos detectado, mostrándose dicha información
por pantalla.

La función `annotate_bojects` superpone la *bounding box* en la imagen capturada, mostrando el resultado.

El resto de funcionalidad del ejemplo es exactamente igual al descrito para la clasificación de imágenes.

!!! danger "Tarea"
    Comprueba el funcionamiento del ejemplo proporcionado, haciendo ver múltiples objetos cotidianos a través de la cámara. Juega en el parámetro `--threshold`, que indica el umbral de *score* mínimo para considerar la detección de un objeto.

!!! danger "Tarea"
    Transforma el código para utilizar el acelerador Google Coral y realiza una comparativa de tiempos de inferencia.

!!! danger "Tarea"
    Obtén una implementación en C++ del código de detección de objetos. En principio, esta implementación no será necesaria hasta el laboratorio correspondiente a detección de objetos, pero resulta conveniente adelantar esta tarea a este laboratorio inicial.
