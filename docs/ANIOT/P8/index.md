# Práctica 8. Arranque seguro y encriptación


## Objetivos

El objetivo de esta práctica es familiarizarse con las técnicas y herramientas que ofrece ESP-IDF
para integrar arranque seguro y encriptación de flash en nuestros proyectos.

Trabajaremos los siguientes aspectos:

* QEMU como plataforma de emulación para desarrollar aspectos relacionados con arranque seguro y encriptación.
* Arranque seguro y firmado de binarios.
* Encriptación de flash y NVS.

 
## Material de consulta
* [QEMU con soporte para ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/qemu.html)
* [Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/security/secure-boot-v2.html)
* [Encriptación FLASH](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/security/flash-encryption.html)
* [Encriptación NVS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_encryption.html)

## Emulador QEMU para ESP32

Debido a que, durante esta prática, modificaremos algunos aspectos de nuestros proyectos
que no son reversibles, es conveniente no utilizar una placa física para su desarrollo.

!!! danger "Nota importante"
    Mantén desconectadas tus placas durante **toda esta práctica**. No las necesitarás y 
    cualquier error podría ser irreversible o dejar la placa inutilizable.

!!! danger "Recordatorio de la nota importante"
    Repetimos: mantén desconectadas tus placas durante **toda esta práctica**. No las necesitarás y 
    cualquier error podría ser irreversible o dejar la placa inutilizable.

QEMU es un emulador de procesadores de código abierto. Proporciona multitud
de soporte para hardware y dispositivos, con capacidad para ejecutar en ellos
cualquier sistema operativo. En conjunción con KVM, QEMU es capaz de ejecutar
máquinas virtuales con un rendimiento similar al nativo. 

Espressif ha desarrollado y mantiene un *fork* del proyecto QEMU, dando soporte tanto
a procesadores con arquitectura Xtensa (ESP32) como RISC-V (ESP32-C3 y similares). Además,
el *fork* de Espressif para el ESP32 proporciona no solo emulación de CPU, sino de los
periféricos más comunes para el ESP32. Para más información sobre las capacidades
del *fork*, se sugiere consultar la [documentación específica](https://github.com/espressif/esp-toolchain-docs/tree/main/qemu/esp32) proporcionada por Espressif.

La herramienta `idf.py`, proporcionada en la instalación por defecto de ESP-IDF, 
proporciona la funcionalidad necesaria para ejecutar y depurar aplicaciones en QEMU.
Es la forma más conveniente de testear aplicaciones y funcionalidades críticas sin 
necesidad de tener que flashearlas en *hardware* real.

!!! danger "Nota importante"
    Se sugiere el uso de una instalación nativa, virtual o a través de WSL de Linux para
    el desarrollo de esta práctica. También se ofrece soporte en MacOS.

### Prerequisitos e instalación de QEMU

Antes de comenzar, es necesario instalar ciertos prerequisitos necesarios de cara a
la instalación de QEMU. Dependiendo de tu distribución o sistema operativo:

- Ubuntu/Debian:

```sh
sudo apt-get install -y libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0
```

- Arch:

```sh
sudo pacman -S --needed libgcrypt glib2 pixman sdl2 libslirp
```

- macOS:

```sh
brew install libgcrypt glib pixman sdl2 libslirp
```

A continuación, instala los binarios de QEMU con el siguiente comando:

```sh
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

Observa que con esto instalarás el emulador para Xtensa (ESP32) y RISC-V (ESP32-C3),
por lo que podrás emular tus dos placas generando proyectos como si fueras a utilizarlas
de la forma habitual.

Tras la instalación, y antes de su uso en cualquier sesión, recuerda ejecutar
`. ./export.sh` en el directorio de la instalación de ESP-IDF.

### Uso de QEMU

#### Ejecución de una aplicación

Para ejecutar una aplicación IDF en QEMU, simplemente es necesario construirla de 
forma habitual para una de las arquitecturas soportadas por el emulador (en nuestro
caso, ESP32 o ESP32-C3), y ejecutar el siguiente comando:

```sh
idf.py qemu monitor
```

Este comando construirá la aplicación (si no lo está ya), arrancará QEMU y abrirá
un monitor IDF, conectándolo al puerto UART emulado. Así, veremos la salida por
consola igual que si se tratase de una ejecución en una placa física.

!!! note "Tarea"
    En esta práctica, se sugiere trabajar con un proyecto sencillo que no incluya
    demasiada funcionalidad adicional (uso de periféricos, por ejemplo). Un buen ejemplo
    es `blink`. Clona el proyecto, constrúyelo y ejecútalo con el anterior comando.
    Anota la salida que observas, e intenta determinar (a la vista de la invocación
    a `qemu` que se realiza) cuáles son sus principales parámetros de ejecución. 

#### Depuración (actividad opcional)

Para depurar una aplicación en QEMU, utiliza el siguiente comando:

```sh
idf.py qemu gdb
```

Este comando permite construir la aplicación, arranca QEMU con un servidor GDB
activado, y abre una terminal interactiva GDB. Este comando es muy útil para depurar
un binario construido, por ejemplo, para nuestros ESP32, ya que no proporcionan
capacidades de depuración en chip (en el caso de los ESP32-C3, esto sí es posible).

En este caso, para observar la salida del programa mientras estás depurando, utiliza
dos terminales:

- En la primera, ejecuta el siguiente comando, que arranca QEMU y el monitor IDF, 
e indica al primero que espere a una conexión entrante GDB:

```sh
idf.py qemu --gdb monitor
``` 

- En la segunda, ejecuta el siguiente comando, que inicia una sesión GDB y la conecta
a QEMU. Desde esta sesión puedes depurar el código, y la salida será visible en la 
primera:

```sh
idf.py gdb
```

!!! note "Tarea (opcional)"
    Depura tu proyecto `blink` estableciendo un *breakpont* en la función `app_main`, y ejecútalo paso a paso observando la salida por pantalla.

#### Emulación de eFuse

QEMU soporta también la emulación de eFuse, clave para el desarrollo de la práctica, ya
que es una forma muy adecuada de probar aspectos de seguridad como arranque seguro y
encriptación de flash, sin dejar a las placas físicas en estados irreversibles.

La herramienta `idf.py` es la encargada de programar eFuses. Al ejecutar cualquiera de
los siguientes comandos, se programan los eFuses del procesador emulado por QEMU a través
del fichero `qemu_efuse.bin`, que se utiliza como habrás observado como
argumento en la ejecución. Por ejemplo:

```sh
idf.py qemu efuse-burn FLASH_CRYPT_CNT 1
idf.py qemu efuse-burn-key flash_encryption my_flash_encryption_key.bin
```

Para mostrar un resumen del contenido de los eFuse, ejecuta:

```sh
idf.py qemu efuse-summary
```

#### Especificación del fichero de imagen

Por defecto, QEMU usa un fichero de imagen llamado `qemu_flash.bin` que se construye
en el directorio de construcción (`build`). Este fichero se genera en base a la información
que se incluye en un fichero llamado `flash_args` que reside en el mismo directorio. Para
usar otro fichero alternativo, podemos ejecutar:

```sh
idf.py qemu --flash-file fichero.bin monitor
```

## Arranque seguro

!!! note "Nota"
    En esta parte de la práctica, se sugiere que sigas trabajando con el mismo proyecto que elegiste en la primera (se sugiere `blink`). En todo caso, trabaja sobre un directorio nuevo para no perder el trabajo realizado en el apartado anterior, por ejemplo, `blink_secure`.

!!! note "Nota"
    Revisa las diapositivas de clase para recordar el background, funcionamiento y etapas de la verificación de bootloader e imágenes proporcionadas por el proceso de arranque seguro.

### ¿Cómo habilitar el arranque seguro?

!!! note "Tarea"
    Sigue los siguientes pasos para configurar tu proyecto para arranque en modo seguro, documentando todos los pasos y posibles observaciones en la memoria.

1. Selecciona como target ESP32.  Abre el menú de configuración del proyecto.  En `Security Features` habilita 
la opción *Enable hardware Secure Boot in Bootloader* para habilitar el 
arranque seguro. Deberás también especificar el esquema de firma RSA y la versión
2 de Secure Boot en las opciones 
*App signing scheme* y *Select Secure Boot version*. Si no te aparecen, 
pasa al punto 2 para habilitarlo y podrás seleccionarlas.

2. Para los ESP32, Secure Boot V2 solo está disponible para ESP32 revisión 
ECO3 en adelante. Para ver la opción "Secure Boot V2", la revisión del chip 
debe cambiarse a la revisión 3 (ESP32-ECO3). Para cambiar la revisión del chip, 
configura la opción *Minimum Supported ESP32 Revision* a Rev 3 en 
*Hardware Settings -> Chip revision -> Minimum Supported ESP32 revision*.

3. Especifica la ruta a la clave de firma de arranque seguro, 
relativa al directorio del proyecto. Puedes dejar su valor por defecto
(debería ser `secure_boot_signing_key.pem`) o usar otro.

4. **Importante**: selecciona también la opción para que el bootloader forme parte
del proceso de flasheo cuando se usa arranque seguro (estará deshabilitada por 
defecto). La opción se denomina *Flash bootloader along with other artifacts 
using the default flash command*.

5. Selecciona el modo de descarga de ROM UART deseado en 
"UART ROM download mode". 
De forma predeterminada, el modo de descarga de ROM UART se ha mantenido 
habilitado para evitar deshabilitarlo permanentemente en la fase de desarrollo; 
esta opción es potencialmente insegura. Se recomienda deshabilitar el modo de descarga UART para mayor seguridad.

6. Opcionalmente, puedes aumentar la información mostrada por el *bootloader* aumentando
el valor de la opción *Bootloader log verbosity* a Info o Verbose. Ten cuidado porque esto
aumentará el tamaño del bootloader, y probablemente debas aumentar entonces el *offset* 
de la tabla de particiones a un valor mucho mayor al establecido por defecto (opción
*Offset of partition table* a 0xe000 como máximo para dar cabida al bootloader.

Guarda la configuración (añade si lo deseas más opciones), y abandona la fase de configuración.

La primera vez que ejecute la construcción con `idf.py build`, 
si no se encuentra la clave de firma, 
se imprimirá un mensaje de error con un comando para generar una 
clave de firma a través del comando:

```sh
espsecure.py generate_signing_key
```

!!! danger "Importante"
    Una clave de firma generada de esta manera utilizará la mejor fuente de 
    números aleatorios disponible para el sistema operativo y tu instalación 
    de Python (`/dev/urandom` en OSX/Linux y `CryptGenRandom()` en Windows). 
    Si esta fuente de números aleatorios es débil, entonces la clave privada será débil.

!!! danger "Importante"
    Para entornos de producción, se recomienda generar el par de claves 
    mediante `openssl` u otro programa de cifrado estándar de la industria. 
    Tienes más información sobre cómo hacer esto [aquí](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/secure-boot-v2.html#generating-secure-boot-signing-key). De todos modos, dejamos esta parte como **opcional** en la práctica.

7. Tras la ejecución de la orden `idf.py build` de forma exitosa, se habrán creado las imágenes para el *bootloader*, tabla de particiones e imagen de aplicación individualmente en el directorio de construcción. Identifícalas.

8. La imagen final a flashear debería generarse fusionando el *bootloader*, la tabla
de particiones y la imagen de aplicación. Una forma sencilla, si se ha seguido el paso
4, es utilizar la herramienta `esptool.py` de la siguiente manera:

```sh
(cd build; esptool.py --chip esp32c3 merge_bin --fill-flash-size 4MB -o flash_image.bin @flash_args)
```

9. Como vamos a trabajar con QEMU, tenemos ya lista la imagen que se ejecutará en el emulador. Si trabajasemos con una placa real, pasaríamos en este punto por una fase de flasheado de la imagen.

10. Monitoriza la ejecución en QEMU siguiendo la forma de trabajar de ejemplos anteriores. Documenta lo que ves en la salida (en referencia al arranque seguro).

!!! note "Tarea"
    Fíjate en el proceso de verificación de bloques de firmas y de imágenes (incluyendo bootloader e imagen de aplicación). Comprueba que concuerdan con los explicados en clase y en la documentación de ESP-IDF enlazada al inicio de esta memoria. 

!!! note "Tarea"
    ¿Qué ocurriría si un compañero te pasa su imagen de aplicación o su *bootloader*, y tú lo integras en la imagen que emulas? ¿Y que ocurriría si usas tus imágenes con el fichero de firma de tu compañero/a? Intenta ver qué ocurre en el proceso de verificación de firmas en esos casos. De la misma forma, intenta ver qué ocurre si modificas un byte en la imagen firmada (no hagas esto con el *bootloader*).

7. Si has seguido el paso 4, Ejecutar para crear un cargador de arranque habilitado para arranque seguro. La salida de la compilación incluirá un mensaje para un comando de actualización, utilizando .idf.py bootloaderesptool.py write_flash

## Encriptación de FLASH

!!! note "Tarea"
    Sigue los siguientes pasos para configurar tu proyecto para integrar la encriptación de flash, documentando todos los pasos y posibles observaciones en la memoria. Como en anteriores ejercicios, utiliza el mismo proyecto (`blink`) pero trabaja en una nueva carpeta, por ejemplo `blink_encrypted`. Parte del proyecto en blanco, no del que integra el arranque seguro.

La encriptación de flash tiene como objetivo encriptar los contenidos de la memoria
flash del ESP32. Cuando se habilita, el firmware se flashea en claro, y se encripta al 
vuelo (*in-place*) en el primer arranque. Así, una simple lectura de flash deja de 
ser suficiente como para recuperar los datos almacenados.

Esta característica puede o no habilitarse junto al arranque seguro, aunque en esta práctica**no las combinaremos**.

Cuando se habilita, los siguientes tipos de partición se encriptan por defecto:

- *Second stage bootloader*.
- Tabla de particiones.
- NVS.
- OTADATA.
- Todas las particiones de tipo *app*.o

Tienes una referencia sobre los eFuses a activar en la [documentación oficial](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/flash-encryption.html#relevant-efuses).

### Cómo funciona la encriptación

Asumiendo que se ha activado la encriptación, ésta procede de la siguiente manera:

1. En el primer arranque, los datos en flash están desencriptados (almacenados en claro).
2. El bootloader lee el eFuse *FLASH_CRYPT_CNT*. Si su valor es 0, configura el bloque de encriptación (la activa).
3. El bootloader chequea si hay una clave válida presente en eFuse. Si es el caso, no se genera una nueva. Si no lo es se procede a la generación de una clave y se almacena en el eFuse *flash_encryption*.
4. El hardware de encriptación encripta los contenidos de la flash. Este proceso puede ser lento.
5. El bootloader fija la activación modificando el valor del eFuse FLASH_CRYPT_CNT.
6. El dispositivo se resetea.

### Cómo se activa la encriptación (modo *development*)

En el menú de configuración, simplemente busca y activa la opción *Enable Flash Encryption on Boot*.

!!! note "Tarea"
    Activa la encriptación de FLASH en tu proyecto `blink`. Observa y anota las fases por las que pasa el proceso de arranque (añade logs al bootloader) y comprueba si coinciden con las fases anteriormente mencionadas. ¿Notas alguna penalización de rendimiento evidente en alguna fase?

!!! note "Tarea"
    Tras observar el funcionamiento con el proyecto `blink`, sería conveniente observar el efecto de la encriptación en los datos almacenados en flash. Para ello, usaremos el ejemplo `flash_encryption` de ESP-IDF. Clónalo y chequea si está o no activa la encriptación. A continuación, flashea y ejecuta (en QEMU) el código. Observa la salida. Cambia la funcionalidad de encriptación y vuelve a observarla. ¿Qué ves en pantalla? Estudia el código e intenta entender qué está pasando, y por qué en un caso el contenido de la flash es "entendible", y en otro no. ¿Qué funciones usa el código para realizar la lectura con desencriptación y en crudo?

## Encriptación de NVS

De forma independiente a la encriptación de FLASH, es posible llevar a cabo un proceso de encriptación únicamente de los datos almacenados en NVS en forma clave-valor. Esta solución aporta cierta seguridad (en una parte de los datos), y a la vez no empeora el rendimiento como sí hace la encriptación total de flash.

!!! note "Tarea"
    El ejemplo `nvs_encryption_hmac` propone una forma muy sencilla de observar el efecto de la encriptación de NVS sobre el contenido que se almacena en la partición NVS de la flash. Compílalo y ejecútalo tal con NVS desactivado (busca la opción correspondiente en los menús de configuración). ¿Qué observas? A continuación, activa la encriptación NVS. ¿Qué observas? **IMPORTANTE: este proyecto debe configurarse para ESP32-C3, ya que solo él tiene soporte para HMAC, técnica usada para encriptar NVS. Además, la opción eFuse key ID storing the HMAC key debe fijarse a 3**. En todo caso, la ejecución se emulará vía QEMU. Recuerda: **no conectes tu placa**.
