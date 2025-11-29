# LAB9. 6LoWPAN y simulador Cooja

## Introducción y objetivos

Los routers de borde son enrutadores que pueden encontrarse en el borde de una
red, encaminando el tráfico de dicha red hacia una segunda red externa. Su
función, en definitiva, es conectar una red con otra.

En esta práctica, usaremos el simulador Cooja, del proyecto Contiki-NG, para
construir una red de nodos que se comuniquen por 6LoWPAN, usando RPL
(*Routing Protocol for Low-Power and Lossy Networks*) como
algoritmo de encaminamiento. Los nodos simulados usan el RTOS de Contiki-NG.
Veremos cómo un router de borde puede utilizarse para enrutar tráfico entre una
red RPL (una red de sensores simulada) y una red IPv4 externa, siguiendo el
siguiente diagrama:

![](img/diagram.png)

El objetivo de la práctica es ofrecer una visión general sobre cómo desplegar
tanto una red RPL con Contiki-NG en el simulador Cooja, así como conseguir
hacerla interactuar con una segunda red externa real utilizando la herramienta
`tunslip`.

## Instalación de Cooja 

Se ofrecen dos alternativas para instalar y usar el simulador Cooja:

- Instalación mediante contenedores Docker.
- Uso de una máquina virtual de VirtualBox proporcionada por el profesor.

### Alternativa 1: instalación con Docker

A continuación, indicamos los pasos a seguir para realizar la instalación del
software necesario en un sistema GNU/Linux. Los detalles de instalación para
Windows y macOS están en la [*getting started guide* de
Contiki-NG](https://docs.contiki-ng.org/en/develop/doc/getting-started/Docker.html).

Comenzaremos por instalar Docker si no lo tenemos instalado ya, haciendo:

```sh
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

Si nuestro usuario no pertenece al grupo Docker, lo añadimos:

```sh
sudo usermod -aG docker <your-user>
```

Después, será necesario reiniciar el sistema para completar la instalación
y actualizar la lista de grupos del usuario.

A continuación, descargaremos la imagen Docker de Contiki-NG:

```sh
docker pull contiker/contiki-ng
```

Una vez descargada la imagen, clonaremos el repositorio git de Contiki-NG (por
ejemplo, en nuestro directorio home):

```sh 
git clone https://github.com/contiki-ng/contiki-ng.git
cd contiki-ng
git submodule update --init --recursive
```

A continuación, crearemos el script `$HOME/.local/bin/contiker` con permisos de ejecución, el cual
usaremos en el futuro para lanzar el contenedor Docker. El contenido de este script será:

```bash
#!/bin/bash

export CNG_PATH=$HOME/contiki-ng
xhost +SI:localuser:$(id -un)
docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 \
  --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng \
  -e DISPLAY=$DISPLAY -e LOCAL_UID=$(id -u $USER) -e LOCAL_GID=$(id -g $USER) \
  -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb \
  -ti contiker/contiki-ng
xhost -SI:localuser:$(id -un)
```

En este momento, si añadimos la ruta `$HOME/.local/bin` a la variable
*PATH*, podemos ejecutar el contenedor de Contiki-NG ejecutando el script
`contiker` y, una vez dentro del contenedor, ejecutar el simulador Cooja:

```sh 
$ contiker
localuser:user being added to access control list
To run a command as administrator (user "root"), use "sudo <command>".
See "man sudo_root" for details.

user@e2d84745c836:~/contiki-ng$ cooja
```

La primera vez que lo ejecutemos tardará un poco porque se descargará una serie
de archivos Java necesarios para el simulador. Es posible que se produzca un
error en la resolución de nombres (DNS). En ese caso, debemos configurar el DNS
de Docker:

```sh 
dockerd --dns 8.8.8.8
```

### Alternativa 2: uso de máquina virtual

Si no tenemos VirtualBox instalado en nuestro equipo, lo primero será instalarlo
descargando el instalador desde la página oficial de [Oracle VirtualBox](https://www.virtualbox.org/wiki/Downloads).

A continuación, descargaremos la máquina virtual Debian con Contiki-NG instalado
desde [este enlace de Google
Drive](https://drive.google.com/file/d/1RMv7yfqvhENRwD1GXS_5Qw5suOLt_98c/view?usp=sharing).
Se trata de un archivo .ova que tendremos que importar en VirtualBox.

Una vez importado, podemos arrancar la máquina virtual (el usuario es *user* y la
contraseña *contiki*).
Para arrancar el simulador, bastará con abrir una terminal y ejecutar el comando *cooja*.
El repositorio de Contiki-NG se encuentra en un directorio con el mismo nombre dentro del home del usuario *user*.

## Código Contiki-NG

En el desarrollo de la práctica utilizaremos algunos de los ejemplos incluidos en la
instalación de Contiki-NG (ubicados en el directorio `contiki-ng/examples`):

* `rpl-border-router/border_router.c`: contiene la lógica de enrutamiento
  del router de borde, que será la raíz del DODAG (*Destination-Oriented Directed Acyclic Graph*).
* `hello-world/hello-world.c`: será ejecutado por el resto de nodos de la red RPL.

Los nodos que ejecuten el código `hello-world.c` formarán un DAG con el
router de borde configurado como raíz. Este router recibirá el prefijo de
red a través de una conexión SLIP (*Serial Line Interface Protocol*) y lo comunicará al
resto de nodos de la red RPL para que configuren sus respectivas direcciones IPv6
globales. Una vez recibido el prefijo, el router de borde se configura como
raíz del DODAG y envía el prefijo al resto de nodos de la red.

## Simulación en Cooja

Para crear una simulación completa en Cooja, arrancamos el simulador usando el
siguiente comando:

```sh
To run a command as administrator (user "root"), use "sudo <command>".
See "man sudo_root" for details.

user@e2d84745c836:~/contiki-ng$ cooja
```

Si estamos utilizando la máquina virtual, basta con ejecutar Cooja desde un terminal.
De aquí en adelante se pueden ignorar todos los detalles relativos a Docker si
hemos optado por usar la máquina virtual.

Si todo ha ido correctamente, debería aparecer la ventana principal del simulador:

![](img/Cooja_Window.png)

A partir de ahora, sigue los pasos indicados para crear una simulación en Cooja.

!!! danger "Ejercicio 1"
    Documenta con capturas de pantalla tanto la configuración que vas a realizar a continuación
    como el resultado final que muestre el simulador.

Primero, selecciona la opción `File -> New simulation`.
Selecciona `UDGM` como modelo de radio para simular las comunicaciones inalámbricas
e introduce el nombre de la simulación. Presiona `Create` y se abrirá la ventana de simulación:

![](img/Cooja_New_Sim.png)

En el menú `Motes` (mote = nodo de sensor simulado), selecciona `Add motes -> Create new mote type` y seleccona el
tipo de mota `Cooja`.
Luego selecciona como código fuente el archivo del ejemplo para el router de borde: `examples/rpl-border-router/rpl-border-router.c`:

![](img/Cooja_New_Mote.png)

Pulsa en `Compile`, luego en `Create` y añade **una única** mota de este tipo.

Repite los pasos anteriores para crear 4 motas de tipo `Cooja` que ejecuten el ejemplo `hello-world.c`.
Distribúyelas por la simulación, asegurándote de que no todas estén al alcance directo del router de borde
pero que puedan llegar a este pasando a través de otros nodos que sí estén dentro de su rango:

![](img/RPL_Red_Ejemplo.png)

A continuación, crearemos una conexión SLIP entre la red RPL simulada en Cooja y
una máquina externa (ya sea un contenedor Docker o nuestra máquina virtual). Para ello, pulsa
en el menú `Tools -> Serial Socket (SERVER)` y selecciona la mota correspondiente
al router de borde (identifícala por su número o tipo):

![](img/Cooja_Serial_Socket.png)

Aparecerá un mensaje como el de la siguiente figura (observa que indica "**Listen port: 60001**").
Pulsa `Start` para activar la conexión SLIP (en este caso, iniciar el socket TCP):

![](img/Cooja_Serial_Listening.png)

Finalmente, inicia la simulación pulsando `Start/Pause` en la ventana principal del simulador.
Revisa la ventana `Network` y la salida de las motas en la ventana `Mote output`.

## Asignando el prefijo de red

Como ya se ha comentado, un router de borde actúa como enlace para conectar una red con
otra. En este ejemplo, el router de borde se utiliza para establecer una ruta de datos
entre la red RPL y una máquina externa (ya sea un contenedor Docker o nuestra máquina virtual).
Para ello, utilizaremos la herramienta *tunslip6* proporcionada por Contiki-NG
en el directorio `tools/serial-io`, que se puede compilar de la siguiente forma:

```sh
make tunslip6
```

Una vez compilado, ejecutamos el binario resultante para establecer una conexión
entre la red RPL y la máquina externa:

```sh
sudo ./tunslip6 -a 127.0.0.1 -p 60001 aaaa::1/64
```

Si la ejecución se ha realizado correctamente, aparecerá una salida similar a la siguiente:

```sh
slip connected to ``127.0.0.1:60001''
opened tun device ``/dev/tun0''
ifconfig tun0 inet `hostname` mtu 1500 up
ifconfig tun0 add aaaa::1/64
ifconfig tun0 add fe80::0:0:0:1/64
ifconfig tun0

tun0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 127.0.1.1  netmask 255.255.255.255  destination 127.0.1.1
        inet6 fe80::1  prefixlen 64  scopeid 0x20<link>
        inet6 aaaa::1  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::93d:daef:6b56:52a7  prefixlen 64  scopeid 0x20<link>
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

...

*** Address:aaaa::1 => aaaa:0000:0000:0000
[INFO: BR        ] Waiting for prefix
[INFO: BR        ] Server IPv6 addresses:
[INFO: BR        ]   aaaa::201:1:1:1
[INFO: BR        ]   fe80::201:1:1:1
```

La herramienta *tunslip6* ha creado una interfaz puente `tun0` con IPv4 127.0.1.1 y ha
enviado, a través de la conexión serie, un mensaje de configuración al router de borde indicándole el
prefijo IPv6 deseado para todos los nodos de la red RPL (`aaaa::/64`).
Las dos últimas líneas de la salida anterior indican cuáles son las direcciones
IPv6 del router de borde tras recibir el prefijo.

Vuelve al simulador Cooja y observa el mensaje que ha aparecido en la ventana `Serial Socket (SERVER)`
(apartado *status*).

## Verificación de resultados

Es posible verificar la dirección IPv6 del router de borde realizando un ping
desde tu contenedor o máquina virtual:

```sh
user@e2d84745c836:~/contiki-ng$ ping6 aaaa::201:1:1:1
PING aaaa::201:1:1:1(aaaa::201:1:1:1) 56 data bytes
64 bytes from aaaa::201:1:1:1: icmp_seq=1 ttl=64 time=17.7 ms
64 bytes from aaaa::201:1:1:1: icmp_seq=2 ttl=64 time=44.8 ms
64 bytes from aaaa::201:1:1:1: icmp_seq=3 ttl=64 time=10.4 ms
^C
```

Así como la IPv6 de cualquier otro nodo de la red. Por ejemplo, para el nodo 5:

```sh
user@e2d84745c836:~/contiki-ng$ ping6 aaaa::205:5:5:5
PING aaaa::205:5:5:5(aaaa::205:5:5:5) 56 data bytes
64 bytes from aaaa::205:5:5:5: icmp_seq=1 ttl=61 time=6.56 ms
64 bytes from aaaa::205:5:5:5: icmp_seq=2 ttl=61 time=17.2 ms
64 bytes from aaaa::205:5:5:5: icmp_seq=3 ttl=61 time=6.38 ms
^C
```

La dirección IPv6 de cada nodo puede obtenerse filtrando en la ventana `Mote output`
según el ID del nodo (mota).

!!! danger "Ejercicio 2"
	  Mientras haces ping a uno de los nodos, vuelve al simulador Cooja y explica lo que
	  ocurre en la ventana `Network`. Ajusta la velocidad de simulación a 1X para poder
	  seguir la transmisión en tiempo real.

## Captura de paquetes para análisis

En el simulador Cooja, desde el menú `Tools` podemos abrir la ventana de `Radio Messages`.
Esta ventana permite capturar todos los paquetes de la simulación y generar un archivo
`.pcap` para su posterior análisis con Wireshark.
Para ello, en el menú `Analyzer` de la ventana `Radio Messages` seleccionamos la opción
`6LoWPAN Analyzer with PCAP`.

A continuación, reiniciamos la simulación anterior pulsando el botón `Reload` en la ventana
principal de Cooja. Esto interrumpirá la comunicación con la máquina externa, por lo que
será necesario volver a ejecutar *tunslip6*.

Ahora estamos listos para volver a simular la red capturando todos los paquetes
enviados entre los nodos.
Dejamos que la simulación se ejecute durante un tiempo y luego la paramos.
Cooja habrá generado un archivo `radiolog-<n>.pcap`, donde `<n>` será un número aleatorio,
en el directorio desde el que se haya lanzado Cooja.

En Wireshark podemos filtrar los paquetes relacionados con el protocolo RPL y
buscar los mensajes `Destination Advertisement Object (DAO)` que los nodos
envían hacia la raíz indicando el nodo que han elegido como padre.
Por ejemplo, en la siguiente captura podemos observar que el nodo 3 envía su DAO indicando que
escoge al nodo 4 como padre:

![](img/Wireshark_RPL_DAO.png)

Esto es razonable para la topología escogida en la simulación en la que el nodo
3 no tiene al nodo 1 dentro del alcance de radio, pero sí al nodo 4, que está a un salto
del nodo raíz:

![](img/RPL_Red_Ejemplo.png)

!!! danger "Ejercicio 3"
	  Crea una red RPL con un router de borde y 10 motas de tipo `Cooja`.
	  Conéctala a tu red local mediante *tunslip6*.
    Asegúrate de que no todos los nodos estén al alcance del router de borde (implementa 3 niveles).
    Comprueba la conectividad con todas las motas y documenta el proceso.

!!! danger "Ejercicio 4"
    Con una ejecución de ping activa sobre una mota al alcance directo del
    router de borde, cambia la posición de dicha mota para que necesite al menos un
    salto intermedio para llegar a la raíz.
    Registra el tiempo que tarda RPL en volver a converger el DODAG.
    Documenta el proceso y tus observaciones.

!!! danger "Ejercicio 5"
    Captura los mensajes enviados por los nodos de la nueva red en un archivo `.pcap`.
    Analiza y reporta el tráfico RPL generado durante el proceso de construcción del DAG.
    A partir de esta información, deduce la topología de la red, identificando el padre
    preferente de cada nodo.
