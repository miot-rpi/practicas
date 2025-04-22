# Laboratorio 5. Comandos por voz

## Objetivos

* Familiarizarse con el procesamiento de voz en nodos AI-IoT
* Configurar un interfaz por voz offline/on-device para detección de comandos de voz
* Usar el acelerador Coral USB para dicho procesamiento
* Integrar procesamiento con voz en una solución IoT genérica

## Introducción

### Procesamiento de voz

Hoy en día el procesamiento digital de la voz es empleado en múltiples ámbitos, siendo tal vez uno de los más extendidos el de los asistentes de voz virtuales  (Amazon Alexa, Google Assitant, Apple Siri, ...).

![Spoken Dialog System](https://d3i71xaburhd42.cloudfront.net/7a3c58ef132157da8e8de0f2ca2ca4c4fffdb9ef/8-Figure2.1-1.png)

En su mayoría estos sistemas combinan diversas técnicas de inteligencia artificial para proporcionar un dialogo hablado con el usuario (ver esquema). La complejidad computacional y el tamaño de los modelos empleados por estos algoritmos son elevados, por lo que en su mayoría se ejecutan (*online*) en servidores *Cloud*. No obstante, debido a razones de eficiencia energética, ancho de banda, latencia y privacidad se está tendiendo actualmente a trasladar parte de este procesamiento a los propios nodos. 

### *Wake-Up Word (WUW)*

Por ejemplo, para llevar a cabo el reconocimiento de habla, los asistentes virtuales utilizan, en su mayoría, algoritmos de detección de palabras clave *despertador* (Wake-Up Word) para detectar el comienzo de un enunciado por parte del usuario. Estos algoritmos, de menor complejidad que otros, se ejecutan *offline* en el propio dispositivo (*smartphone*, altavoz inteligente, etc.) lo que permite una mayor eficiencia y privacidad, ya que no es necesario transferir todo el audio al *Cloud*. 

Es preciso señalar que, puesto que se están ejecutando constantemente, la complejidad computacional y la energía requeridas por estos algoritmos debe ser las menores posibles. Esto se consigue limitando su capacidad de identificación a una sola palabra o como máximo una única frase, generalmente corta.

### Reconocimiento del habla (ASR/STT)

El reconocimiento del habla, también denominado reconocimiento automático de voz, o en inglés *Automatic [Speech Recognition](https://en.wikipedia.org/wiki/Speech_recognition)* (*ASR*) o también *Speech To Text* (*STT*) consiste en, dado un enunciado delimitado por *WUW* o por otro evento (como la pulsación de un botón en el caso de *push to talk*), transcribirlo a texto para su posterior procesamiento (transcripción, traducción, sistemas de respuesta automática, etc.). La complejidad computacional y el tamaño de los modelos empleados suponen un obstáculo para su procesamiento *offline* en la mayor parte de nodos IoT, exceptuando *smartphones* o dispositivos de similares prestaciones aparte para los que existen algunas implementaciones viables.

### *Wordspotting*

En determinados contextos, como los dispositivos controlados por comandos de voz (*voice command device*, VCD) no es necesario llevar a cabo un reconocimiento completo del habla y tan sólo es necesario identificar un pequeño conjunto de palabras clave (comandos), lo que se conoce con el término genérico de *wordspotting* o *[keyword spotting](https://en.wikipedia.org/wiki/Keyword_spotting)*. Este tipo algoritmos, aunque más complejos que la detección de *WUW*, son mucho más sencillos que los *ASR*/*STT* y con frecuencia pueden ejecutarse *offline* incluso en pequeños microcontroladores.

## Espectrogramas y MFCCs

Todos los algoritmos de procesamiento de voz se basan en el uso de [espectrogramas](https://en.wikipedia.org/wiki/Spectrogram) que son una representación gráfica (3D) del contenido frecuencial de una señal a lo largo del tiempo.

![Espetrograma de las palabras "nineteenth century" ](https://upload.wikimedia.org/wikipedia/commons/c/c5/Spectrogram-19thC.png)

El proceso de obtención de un *espectrograma* adecuado para el procesamiento de voz sería el siguiente:

1. *Pre-emphasis*: pre-amplificación (opcional) de las componentes de alta frecuencia de la señal de audio.
![](https://miro.medium.com/max/1400/1*tZPMsFLe3xfaWxa8RKXi5w.png)
2. *Framing*: división de la señal en un cierto número (*nFrames*) de ventanas temporales, habitualmente de entre 20 y 40 ms de duración.
![](https://miro.medium.com/max/1400/1*E0MiQ74KhklGuwE1PMfLNw.jpeg)
3. *Windowing*: consiste en la aplicación de una *función ventana* a cada *frame*.
![](https://miro.medium.com/max/864/1*O1XY3EEyFZAGHPrfDbrgGw.png)
4. *Transformación y cálculo de la densidad espectral*: consiste en el cálculo de la *Transformada de Fourier de Tiempo Reducido* (*Short-Time Fourier-Transform* o *STFT*) para cada *frame*.
![](https://miro.medium.com/max/1400/1*mjrMIkJuU3YcEdDuJAdvUw.jpeg)
5. *MEL scale mapping*: aplicación de un banco de filtros (*nFilters*) correspondientes a la *[Escala de Mel](https://en.wikipedia.org/wiki/Mel_scale)* al *espectro* obtenido en el paso anterior.
6. *Normalización*: aplicación de la *Transformada Discreta de Coseno* (*DCT*) y normalización mediante la resta de la media. 

El *espectrograma* resultante es lo que se conoce como *Coeﬁcientes Cepstrales en las Frecuencias de Mel* o ***MFCCs*** (Mel Frequency Cepstral Coeﬃcients) y es la entrada que suelen emplear los algoritmos de procesamiento de voz. 

!!!danger "Nota" 
    Cabe mencionar que dependiendo de algoritmo empleado es posible usar directamente el *espectrograma* resultante del paso 5. 

!!! note "Tarea (opcional)"
    Para entender este proceso seguir el ejemplo del artículo ["Speech Processing for Machine Learning: Filter banks, Mel-Frequency Cepstral Coefficients (MFCCs) and What's In-Between" de Haytham M. Fayek](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html)

![](https://miro.medium.com/max/1400/1*Lhx1J0G2CbixMIvQUX3Otw.png)

## Reconocimiento de palabras clave y redes neuronales

Como hemos visto previamente los *espectrogramas* proporcionan una representación gráfica 3D a partir de una señal de audio, lo que ofrece la posibilidad de llevar a cabo el reconocimiento de palabras clave mediante CNNs similares a las empleadas en el reconocimiento de imágenes.

### Ejemplo sencillo de CNN para el reconocimiento de palabras clave

El siguiente [tutorial](https://www.tensorflow.org/tutorials/audio/simple_audio) ilustra el proceso construcción de una CNN sencilla para el reconocimiento de 10 palabras, empleando para el entrenamiento un subconjunto de la base de datos [*"Speech Commands: A Dataset for Limited-Vocabulary Speech Recognition"*](https://www.tensorflow.org/datasets/catalog/speech_commands) y empleado las utilidades propias de Tensorflow para la decodificación de los ficheros audio y la generación de los espectrogramas.

!!! note "Tarea (opcional)"
    Seguir el tutorial.

!!! note "Tarea (opcional)"
    Convertir el modelo entrenado a TFLite, cuantizarlo y ejecutarlo en el acelerador Coral USB.

### Coral KWS

El proyecto *[Coral Keyword Spotter (KWS)](https://github.com/google-coral/project-keyword-spotter)* proporciona un modelo pre-entrenado de un reconocedor de 140 palabras clave listo para su uso con Edge TPU así como scripts que ilustran su uso.

!!! note "Tarea"
    Siguiendo la documentación del proyecto probar el modelo y medir los tiempo de inferencia.

!!! note "Tarea"
    Adaptar los scripts de Coral KWS para enviar los comandos a un broker MQTT y posteriormente visualizarlos en un *dashboard* (por ejemplo, hacer subir o bajar un curva, cambiar iluminación, girar las manecillas de un reloj, etc. todo controlado por voz a distancia).

## Algunas herramientas

A continuación se enumeran algunas de las principales opciones para procesamiento de voz offline.

* Keyword spotting / Wake-Up-Word
	- [Pocketsphinx](https://cmusphinx.github.io/wiki/tutorialpocketsphinx/)
	- [Porcupine](https://github.com/Picovoice/porcupine)
	- [Snowboy](https://github.com/Kitt-AI/snowboy)
	- [Mycroft Precise](https://github.com/MycroftAI/mycroft-precise)
* Speech to text
	- [Pocketsphinx](https://cmusphinx.github.io/wiki/tutorialpocketsphinx/)
	- [Kaldi](https://kaldi-asr.org)
	- [DeepSpeech](https://deepspeech.readthedocs.io/en/r0.9/)

!!! note "Tarea (opcional)"
    Probar algunas de estas herramientas en la Raspberry Pi 4.

## Algunas referencias

* Santosh Singh, ["How speech-to-text/voice recognition is making an impact on IoT development"](https://internetofthingswiki.com/how-speech-to-text-voice-recognition-is-making-an-impact-on-iot-development/1269/), Featured, Internet Of Things, 2018.
* Yuan Shangguan, Jian Li, Qiao Liang, Raziel Alvarez, Ian McGraw, ["Optimizing Speech Recognition For The Edge"](https://arxiv.org/abs/1909.12408), https://arxiv.org/abs/1909.12408
* Thibault Gisselbrecht, Joseph Dureau, ["Machine Learning on Voice: a gentle introduction with Snips Personal Wake Word Detector"](https://medium.com/snips-ai/machine-learning-on-voice-a-gentle-introduction-with-snips-personal-wake-word-detector-133bd6fb568e), Snips Blog, May 2 2018.
* Haytham M. Fayek, ["Speech Processing for Machine Learning: Filter banks, Mel-Frequency Cepstral Coefficients (MFCCs) and What's In-Between"](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html), 2016.
