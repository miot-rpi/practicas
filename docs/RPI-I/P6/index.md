# LAB6. BLE: cliente GATT

# Objetivos

* Diseccionar en detalle un *firmware* de un cliente GATT utilizando la API de ESP-IDF.
* Aprender a realizar un escaneo de dispositivos BLE.
* Conocer la información disponible en los anuncios BLE.
* Gestionar la conexión desde un cliente BLE a un servidor GATT BLE.

# Implementación de un cliente GATT para escaneo y conexión a servidor

En esta práctica se revisa el código de ejemplo para la construcción de un cliente GATT para el ESP32 utilizando ESP-IDF. El código implementa un cliente GATT que escanea servidores periféricos cercanos y se conecta a un servicio predefinido. El cliente busca características disponibles y se suscribe a una característica conocida para recibir indicaciones o notificaciones. El ejemplo puede registrar un perfil de aplicación e inicializa una secuencia de eventos que se pueden utilizar para configurar parámetros GAP y para manejar eventos como el escaneo, la conexión a periféricos y la lectura y escritura de características.

El desarrollo de esta práctica requiere el uso de dos placas: una ejecutando el servidor GATT básico (o modificado) que usaste en la práctica anterior, y otra ejecutando el código cliente.

# Descripción del código de ejemplo

El ejemplo que seguiremos y adaptaremos se encuentra en la carpeta de ejemplos de ESP-IDF en [bluetooth/bluedroid/ble/gatt_client/main](../main). El archivo [gattc_demo.c](../main/gattc_demo.c) contiene todas las funcionalidades que vamos a revisar. 

!!! note "Nota"
    Antes de comenzar, asegúrate de que la variable `remote_device_name` NO coincide con la de tu servidor GATT.

## Ficheros de cabecera

Observa los ficheros de cabecera incluidos en [gattc_demo.c](../main/gattc_demo.c) (similares a los de la práctica anterior):

```c
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "controller.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
```

Estos `includes` son necesarios para que funcionen los componentes del sistema subyacente y FreeRTOS, incluida la funcionalidad de registro y una biblioteca para almacenar datos en memoria flash no volátil. Estamos interesados en `"esp_bt.h"`, `"esp_bt_main.h"`, `"esp_gap_ble_api.h"` y `"esp_gattc_api.h"`, que exponen las APIs BLE necesarias para implementar este ejemplo:

* `esp_bt.h`: configura el controlador BT y VHCI desde el lado del host.
* `esp_bt_main.h`: inicializa y habilita la pila Bluedroid.
* `esp_gap_ble_api.h`: implementa la configuración GAP, por ejemplo los anuncios de dispositivos y los parámetros de conexión.
* `esp_gattc_api.h`: implementa la configuración del cliente GATT, como la conexión a periféricos y la búsqueda de servicios.

## Punto de entrada principal

La función de punto de entrada del programa es `app_main()`:

```c
void app_main()
{
    // Inicializar NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s inicialización del controlador fallida, código de error = %x", __func__, ret);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s habilitación del controlador fallida, código de error = %x", __func__, ret);
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s inicialización de Bluetooth fallida, código de error = %x", __func__, ret);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s habilitación de Bluetooth fallida, código de error = %x", __func__, ret);
        return;
    }

    // Registrar la función de devolución de llamada en el módulo GAP
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s registro de GAP fallido, código de error = %x", __func__, ret);
        return;
    }

    // Registrar la función de devolución de llamada en el módulo GATTC
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s registro de GATTC fallido, código de error = %x", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s registro de la aplicación GATTC fallido, código de error = %x", __func__, ret);
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "configuración del MTU local fallida, código de error = %x", local_mtu_ret);
    }

}
```

La función principal comienza inicializando la biblioteca de almacenamiento no volátil. Esta biblioteca permite guardar pares clave-valor en la memoria flash y se utiliza en algunos componentes, como la biblioteca Wi-Fi, para guardar el SSID y la contraseña:

```c
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);
```
## Inicialización del controlador Bluetooth y la pila BLE

La función principal también inicializa el controlador BT al crear primero una estructura de configuración del controlador BT llamada `esp_bt_controller_config_t` con ajustes predeterminados generados por la macro `BT_CONTROLLER_INIT_CONFIG_DEFAULT()`. El controlador BT implementa la interfaz HCI en el lado del controlador, la capa de enlace (LL) y la capa física (PHY). El controlador BT es invisible para las aplicaciones de usuario y se encarga de las capas inferiores de la pila BLE. La configuración del controlador incluye el tamaño de la pila del controlador BT, la prioridad y la velocidad de baudios HCI para la transmisión. Con la configuración creada, se inicializa y habilita el controlador BT con la función `esp_bt_controller_init()`:

```c
esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
ret = esp_bt_controller_init(&bt_cfg);
```

A continuación, el controlador se habilita en modo BLE:

```c
ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
```

Hay cuatro modos de Bluetooth admitidos:

1. `ESP_BT_MODE_IDLE`: Bluetooth no se está ejecutando.
2. `ESP_BT_MODE_BLE`: modo BLE.
3. `ESP_BT_MODE_CLASSIC_BT`: modo BT clásico.
4. `ESP_BT_MODE_BTDM`: modo dual (BLE + BT clásico).

Después de la inicialización del controlador BT, se inicializa y habilita la pila Bluedroid, que incluye las definiciones y APIs comunes tanto para BT clásico como para BLE. Esto se realiza mediante:

```c
ret = esp_bluedroid_init();
ret = esp_bluedroid_enable();
```

La función principal finaliza registrando los controladores de eventos GAP y GATT así como el perfil de aplicación, y configurando el tamaño máximo admitido de MTU:

```c
// Registrar la función de devolución de llamada en el módulo GAP
ret = esp_ble_gap_register_callback(esp_gap_cb);

// Registrar la función de devolución de llamada en el módulo GATTC
ret = esp_ble_gattc_register_callback(esp_gattc_cb);

ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);

esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
if (local_mtu_ret){
    ESP_LOGE(GATTC_TAG, "configuración del MTU local fallida, código de error = %x", local_mtu_ret);
}
```

Los controladores de eventos GAP y GATT son las funciones utilizadas para capturar los eventos generados por la pila BLE y ejecutar funciones para configurar los parámetros de la aplicación. Además, los controladores de eventos también se utilizan para manejar eventos de lectura y escritura que provienen del dispositivo central. El controlador de eventos GAP se encarga del escaneo y la conexión a servidores, y el controlador GATT administra los eventos que ocurren después de que el cliente se haya conectado a un servidor, como la búsqueda de servicios y la escritura y lectura de datos.

## Perfiles de aplicación

Los perfiles de aplicación son una forma de agrupar funcionalidades diseñadas para una o más aplicaciones de servidor. Por ejemplo, puede tener un perfil de aplicación conectado a sensores de ritmo cardíaco y otro conectado a sensores de temperatura. Cada perfil de aplicación crea una interfaz GATT para conectarse a otros dispositivos. Las estructuras de perfiles de aplicación en el código son instancias de la estructura `gattc_profile_inst`, que se define como:

```c
struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};
```

La estructura de un perfil de aplicación contiene:

- `gattc_cb`: función de devolución de llamada del cliente GATT.
- `gattc_if`: número de interfaz del cliente GATT para este perfil.
- `app_id`: número de ID del perfil de aplicación.
- `conn_id`: ID de conexión.
- `service_start_handle`: handle de inicio del servicio.
- `service_end_handle`: handle de fin del servicio.
- `char_handle`: handle de característica.
- `remote_bda`: dirección del dispositivo remoto conectado a este cliente.

En este ejemplo, hay un perfil de aplicación y su ID se define como:

```c
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
```

Los perfiles de aplicación se almacenan en el array `gl_profile_tab`, que se inicializa de la siguiente manera:

```c
/* Un perfil basado en GATT, un app_id y un gattc_if, este arreglo almacenará el gattc_if devuelto por ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE, /* No se obtiene el gatt_if, por lo que se inicializa como ESP_GATT_IF_NONE */
    },
};
```

La inicialización del array de tablas de perfiles de aplicación incluye la definición de la función de devolución de llamada para el perfil (`gattc_profile_event_handler()`). Además, la interfaz GATT se inicializa con el valor predeterminado de `ESP_GATT_IF_NONE`. Más adelante cuando se registre el perfil de aplicación, la pila BLE devolverá una instancia de interfaz GATT para usar con ese perfil de aplicación.

El registro del perfil desencadena un evento `ESP_GATTC_REG_EVT`, que es manejado por el manejador de eventos `esp_gattc_cb()`. El manejador toma la interfaz GATT devuelta por el evento y la almacena en la tabla de perfiles:

```c
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d", event, gattc_if);

    /* Si el evento es un evento de registro, almacena el gattc_if para cada perfil */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "registro de aplicación fallido, app_id %04x, estado %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
…
```

Finalmente, la función de devolución de llamada invoca el manejador de eventos correspondiente para cada perfil de la tabla `gl_profile_tab`:

```c
…
    /* Si gattc_if es igual al perfil A, llamar al manejador de cb del perfil A,
    * por lo tanto, aquí llamar a la función cb de cada perfil */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, no especifica un cierto gatt_if, es necesario llamar a cada función cb del perfil */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}
```

## Configuración de parámetros de escaneo

El cliente GATT normalmente escanea servidores cercanos y trata de conectarse a estos si está interesado. Sin embargo, para realizar el escaneo, primero es necesario configurar los parámetros de configuración. Esto se hace después del registro de los perfiles de aplicación, porque una vez completado el registro, se desencadena un evento `ESP_GATTC_REG_EVT`. La primera vez que se desencadena este evento, el manejador de eventos GATT lo captura y asigna una interfaz GATT al perfil A. Luego, el evento se reenvía al manejador de eventos GATT del perfil A. En este manejador de eventos, el evento se utiliza para llamar a la función `esp_ble_gap_set_scan_params()`, que toma una instancia de estructura `ble_scan_params` como parámetro. Esta estructura se define como:

```c
// Parámetros de escaneo BLE
typedef struct {
    esp_ble_scan_type_t     scan_type;              /*!< Tipo de escaneo */
    esp_ble_addr_type_t     own_addr_type;          /*!< Tipo de dirección propia */
    esp_ble_scan_filter_t   scan_filter_policy;     /*!< Política de filtro de escaneo */
    uint16_t                scan_interval;          /*!< Intervalo de escaneo. Se define como el intervalo de tiempo desde que el controlador comenzó su último escaneo LE hasta que comienza el siguiente escaneo LE */
                                                    // Rango: 0x0004 a 0x4000
                                                    // Predeterminado: 0x0010 (10 ms)
                                                    // Tiempo = N * 0.625 ms
                                                    // Rango de tiempo: 2.5 ms a 10.24 segundos
    uint16_t                scan_window;            /*!< Ventana de escaneo. La duración del escaneo LE. LE_Scan_Window debe ser menor o igual que LE_Scan_Interval */
                                                    // Rango: 0x0004 a 0x4000
                                                    // Predeterminado: 0x0010 (10 ms)
                                                    // Tiempo = N * 0.625 ms
                                                    // Rango de tiempo: 2.5 ms a 10.24 segundos
} esp_ble_scan_params_t;
```

Y se inicializa de la siguiente manera:

```c
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30
};
```

Los parámetros de escaneo BLE se configuran de manera que el tipo de escaneo sea activo (incluye la lectura de la respuesta de escaneo), es de tipo público, permite leer cualquier dispositivo anunciado y tiene un intervalo de escaneo de 50 ms (0x50 * 0.625 ms) y una ventana de escaneo de 30 ms (0x30 * 0.625 ms).

Los valores de escaneo se establecen utilizando la función `esp_ble_gap_set_scan_params()`:

```c
case ESP_GATTC_REG_EVT:
    ESP_LOGI(GATTC_TAG, "REG_EVT");
    esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (scan_ret) {
        ESP_LOGE(GATTC_TAG, "error al configurar parámetros de escaneo, código de error = %x", scan_ret);
    }
    break;
```

!!! danger "Ejercicio 1"
    Configura los parámetros de escaneo para que éste se produzca con menos frecuencia (cada segundo).

Una vez que se establecen los parámetros de escaneo, se desencadena un evento `ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT`, que es manejado por el manejador de eventos GAP `esp_gap_cb()`. Este evento se utiliza para iniciar el escaneo de los servidores GATT cercanos:

```c
case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
    uint32_t duration = 30; // En segundos
    esp_ble_gap_start_scanning(duration);
    break;
}
```

El escaneo se inicia mediante la función `esp_ble_gap_start_scanning()`, que toma un parámetro que representa la duración del escaneo continuo (en segundos).

!!! danger "Ejercicio 2"
    Analiza el código y modifícalo para que el proceso de escaneo se produzca indefinidamente. Además, añade al código los parámetros de intervalo y ventana de escaneo a través de `menuconfig` (tomando valores en ms) para poder configurar la frecuencia de escaneo. Comprueba que efectivamente el tiempo en el que el dispositivo se encuentra en fase de escaneo es el seleccionado.

Los resultados del escaneo se muestran tan pronto como llegan con el evento `ESP_GAP_BLE_SCAN_RESULT_EVT`, que incluye los siguientes parámetros:

```c
/**
 * @brief ESP_GAP_BLE_SCAN_RESULT_EVT
 */
struct ble_scan_result_evt_param {
    esp_gap_search_evt_t search_evt;            /*!< Tipo de evento de búsqueda (subevento) */
    esp_bd_addr_t bda;                          /*!< Dirección del dispositivo Bluetooth que ha sido buscado */
    esp_bt_dev_type_t dev_type;                 /*!< Tipo de dispositivo */
    esp_ble_addr_type_t ble_addr_type;          /*!< Tipo de dirección del dispositivo BLE */
    esp_ble_evt_type_t ble_evt_type;            /*!< Tipo de evento de resultado de escaneo BLE */
    int rssi;                                   /*!< RSSI del dispositivo buscado (potencia recibida) */
    uint8_t ble_adv[ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX]; /*!< Datos de publicidad recibidos */
    int flag;                                   /*!< Bit de indicación de datos de publicidad */
    int num_resps;                              /*!< Número de resultados de escaneo */
    uint8_t adv_data_len;                       /*!< Longitud de datos de publicidad */
    uint8_t scan_rsp_len;                       /*!< Longitud de respuesta de escaneo */
} scan_rst;                                     /*!< Parámetro de evento de ESP_GAP_BLE_SCAN_RESULT_EVT */
```

Este evento también incluye una lista de subeventos:

```c
// Subevento de ESP_GAP_BLE_SCAN_RESULT_EVT
typedef enum {
    ESP_GAP_SEARCH_INQ_RES_EVT             = 0,      /*!< Resultado de la búsqueda para un dispositivo BLE */
    ESP_GAP_SEARCH_INQ_CMPL_EVT            = 1,      /*!< Escaneo finalizado */
    …
} esp_gap_search_evt_t;
```

Concretamente, nos interesa el evento `ESP_GAP_SEARCH_INQ_RES_EVT`, que se llama cada vez que se encuentra un nuevo dispositivo. También nos podría interesar el evento `ESP_GAP_SEARCH_INQ_CMPL_EVT`, que se desencadena cuando se completa el escaneo y se puede utilizar para reiniciar el procedimiento de escaneo:

```c
case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
            case ESP_GAP_SEARCH_INQ_RES_EVT:
                adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                            scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                            ESP_BLE_AD_TYPE_NAME_CMPL,
                                                            &adv_name_len);
                ESP_LOGI(GATTC_TAG, "Scan result, device " ESP_BD_ADDR_STR ", name len %u", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
                ESP_LOG_BUFFER_CHAR(GATTC_TAG, adv_name, adv_name_len);
                …
                if (adv_name != NULL) {
                    if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                        ESP_LOGI(GATTC_TAG, "Device found %s", remote_device_name);
                        if (connect == false) {
                            connect = true;
                            ESP_LOGI(GATTC_TAG, "Connect to the remote device");
                            esp_ble_gap_stop_scanning();
                            esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};
                            memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                            creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
                            creat_conn_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
                            creat_conn_params.is_direct = true;
                            creat_conn_params.is_aux = false;
                            creat_conn_params.phy_mask = 0x0;
                            esp_ble_gattc_enh_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, &creat_conn_params);
                        }
                    }
                }
                break;
            case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                …
                break;
```

En el código anterior se obtiene/resuelve el nombre del dispositivo remoto de los datos del anuncio recibido y se compara con el nombre del dispositivo del servidor GATT en el que estamos interesados (variable `remote_device_name`). Si coincide con el nombre del servidor GATT que estamos buscando, se detiene el escaneo.

!!! note "Nota"
    Recuerda que al inicio de la práctica cambiamos la variable `remote_device_name` para que no coincidiera con la de tu servidor GATT. Lo dejamos así por ahora.

Para extraer el nombre del dispositivo del paquete publicitario, utilizamos la función `esp_ble_resolve_adv_data_by_type()`, que toma los datos anunciados almacenados en `scan_result->scan_rst.ble_adv`, la longitud de los datos y su tipo. Una vez obtenido, se imprime el nombre del dispositivo.

!!! danger "Ejercicio 3"
    Muestra también por pantalla el valor de RSSI de cada dispositivo BLE encontrado.

!!! danger "Ejercicio 4"
    Añade a través de `menuconfig` un campo que indique la dirección BLE de un dispositivo (conocido) que esté en fase de anuncio. Modifica la fase de escaneado para que únicamente se muestre la información del dispositivo cuya dirección BLE se haya configurado.

!!! danger "Ejercicio 5"
    Modifica el *firmware* para que, en función del valor de RSSI obtenido para el dispositivo de interés, el cliente reporte por pantalla un valor proporcional a la distancia aproximada con dicho dispositivo (por ejemplo, un número de puntos `.` concreto según la distancia). Para ello, establece un mínimo y un máximo de RSSI y aplica la siguiente fórmula: 
    `(RSSI - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)`. Prueba a configurar un intervalo de escaneo pequeño y a alejar físicamente los dispositivos entre sí.

!!! note "Nota"
    A partir de este punto puedes volver a fijar el nombre del dispositivo remoto de interés en función del nombre otorgado en el servidor GATT.

Finalmente, si el nombre del dispositivo remoto es el mismo que hemos definido anteriormente, el dispositivo local detiene el escaneo y trata de abrir una conexión con el dispositivo remoto utilizando la función `esp_ble_gattc_enh_open()`. Esta función toma parámetros como la interfaz GATT del perfil de aplicación y la dirección del servidor remoto, entre otros. Ten en cuenta que el cliente abre una conexión virtual con el servidor y que la conexión virtual devuelve un ID de conexión (`conn_id`). La conexión virtual es la conexión entre el perfil de aplicación y el servidor remoto. Dado que muchos perfiles de aplicación pueden ejecutarse en un ESP32, podría haber muchas conexiones virtuales abiertas al mismo servidor remoto. También está la conexión física, que es el enlace BLE real entre el cliente y el servidor. Por lo tanto, si la conexión física se desconecta con la función `esp_ble_gap_disconnect()`, se cierran todas las demás conexiones virtuales.

En este ejemplo, cada perfil de aplicación crea una conexión virtual al mismo servidor con la función `esp_ble_gattc_enh_open()`, por lo que si se llamase a la función de cierre (`esp_ble_gattc_close()`), sólo se cerraría esa conexión del perfil de aplicación, mientras que si se llamase a la función de desconexión GAP, se cerrarían ambas conexiones. Además, los eventos de conexión se propagan a todos los perfiles porque se relacionan con la conexión física, mientras que los eventos de apertura se propagan sólo al perfil que crea la conexión virtual.

## Configuración del tamaño de MTU

El MTU se define como el tamaño máximo de cualquier paquete enviado entre un cliente y un servidor. Cuando el cliente se conecta al servidor, informa al servidor qué tamaño de MTU usar intercambiando PDUs de solicitud y respuesta de MTU. Esto se hace después de abrir la conexión.

Cabe destacar que después de abrir una conexión se desencadena el evento `ESP_GATTC_CONNECT_EVT`:

```c
case ESP_GATTC_CONNECT_EVT:
{
    ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote " ESP_BD_ADDR_STR "", p_data->connect.conn_id, ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
    gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
    memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
    esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
    if (mtu_ret) {
        ESP_LOGE(GATTC_TAG, "Config MTU error, error code = %x", mtu_ret);
    }
    break;
}
```

Como se puede observar, el ID de conexión (`conn_id`) y la dirección del dispositivo remoto (`remote_bda`) se almacenan en la tabla de perfiles de aplicación y se imprimen por consola.

El tamaño típico del MTU para una conexión Bluetooth 4.0 es de 23 bytes. Un cliente puede cambiar el tamaño del MTU utilizando la función `esp_ble_gattc_send_mtu_req()`, que toma la interfaz GATT y el ID de conexión. El tamaño del MTU solicitado se define localmente mediante `esp_ble_gatt_set_local_mtu()`. Luego, el servidor puede aceptar o rechazar la solicitud. El ESP32 admite un tamaño de MTU de hasta 517 bytes. En este ejemplo, el tamaño del MTU se establece en 500 bytes.

La apertura de la conexión también desencadena un evento `ESP_GATTC_OPEN_EVT`, que se utiliza para comprobar si la apertura de la conexión se realizó con éxito; de lo contrario, se imprime un error y se sale del programa:

```c
if (param->open.status != ESP_GATT_OK) {
    ESP_LOGE(GATTC_TAG, "Open failed, status %d", p_data->open.status);
    break;
}
ESP_LOGI(GATTC_TAG, "Open successfully, MTU %u", p_data->open.mtu);
break;
```

Cuando se intercambia el MTU, se desencadena el evento `ESP_GATTC_CFG_MTU_EVT`, que en este ejemplo se utiliza simplemente para imprimir el nuevo tamaño del MTU:

```c
case ESP_GATTC_CFG_MTU_EVT:
    ESP_LOGI(GATTC_TAG, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
    break;
```

El evento `ESP_GATTC_CONNECT_EVT` también se utiliza como punto de partida para descubrir los servicios disponibles en el servidor al que se ha conectado el cliente: el stack BLE ejecuta internamente un descubrimiento de servicios inicial, generando el evento `ESP_GATTC_DIS_SRVC_CMPL_EVT`.
Una vez en este evento, para descubrir un servicio concreto se utiliza la función `esp_ble_gattc_search_service()`. Los parámetros de la función son la interfaz GATT, el ID de conexión y el UUID del servicio que le interesa al cliente. El servicio que estamos buscando se define de la siguiente manera:

```c
static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = REMOTE_SERVICE_UUID,
    },
};
```

Donde,

```c
#define REMOTE_SERVICE_UUID 0x00FF
```

Nota: si el UUID del servicio que te interesa es de 128 bits, hay que tener en cuenta que el ESP32 usa "little-endian" para almacenar los datos. En el modo de almacenamiento en "little-endian" puedes definir directamente el UUID del servicio en el orden normal si es un UUID de 16 bits o 32 bits. Para UUIDs de servicio de 128 bits, el orden de los bytes debe invertirse. Por ejemplo, si el UUID del servicio es 12345678-a1b2-c3d4-e5f6-9fafd205e457, `REMOTE_SERVICE_UUID` debería definirse como {0x57,0xE4,0x05,0xD2,0xAF,0x9F,0xF6,0xE5,0xD4,0xC3,0xB2,0xA1,0x78,0x56,0x34,0x12}.

Como ya se ha comentado, los servicios se descubren con la función `esp_ble_gattc_search_service()`:

```c
esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &remote_filter_service_uuid);
```

Para cada servicio encontrado (si los hay), se desencadena un evento `ESP_GATTC_SEARCH_RES_EVT`:

```c
case ESP_GATTC_SEARCH_RES_EVT:
{
    ESP_LOGI(GATTC_TAG, "Service search result, conn_id = %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
    ESP_LOGI(GATTC_TAG, "start handle %d, end handle %d, current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
    if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID)
    {
        ESP_LOGI(GATTC_TAG, "Service found");
        get_server = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
    }
    break;
}
```

En caso de que el cliente encuentre el servicio que busca, la bandera `get_server` se establece a true y se guardan los handles de inicio y fin que se utilizarán posteriormente para obtener todas las características de este servicio. Una vez que se han devuelto todos los resultados de los servicios, se completa la búsqueda y se desencadena un evento `ESP_GATTC_SEARCH_CMPL_EVT`.

## Obteniendo características

Este ejemplo implementa la obtención de datos de características de un servicio predefinido. El servicio del cual queremos obtener características tiene un UUID de 0x00FF, y la característica de interés tiene un UUID de 0xFF01:

```c
#define REMOTE_NOTIFY_CHAR_UUID 0xFF01
```

Podemos obtener las características de ese servicio utilizando la función `esp_ble_gattc_get_char_by_uuid()`, la cual se llama en el evento `ESP_GATTC_SEARCH_CMPL_EVT` después de que se haya completado la búsqueda de servicios y el cliente haya encontrado el servicio que estaba buscando:

```c
case ESP_GATTC_SEARCH_CMPL_EVT:
    …
    if (get_server) {
        uint16_t count = 0;
        esp_gatt_status_t status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                p_data->search_cmpl.conn_id,
                                                                ESP_GATT_DB_CHARACTERISTIC,
                                                                gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                INVALID_HANDLE,
                                                                &count);
        …
        if (count > 0) {
            char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
            if (!char_elem_result) {
                ESP_LOGE(GATTC_TAG, "gattc no mem");
                break;
            }
            else {
                status = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                        remote_filter_char_uuid,
                                                        char_elem_result,
                                                        &count);
                …
            }
        }
        else {
            ESP_LOGE(GATTC_TAG, "no char found");
        }
    }
    break;
```

`esp_ble_gattc_get_attr_count()` obtiene el número de atributos de tipo característica en un rango concreto. Los parámetros de esta función son: la interfaz GATT, el ID de conexión, el tipo de atributo que se busca (`ESP_GATT_DB_CHARACTERISTIC`), handles de inicio y final, el handle de la característica (este parámetro sólo es válido cuando el tipo se establece en `ESP_GATT_DB_DESCRIPTOR`) y la salida del número de atributos encontrados.

Después, se reserva memoria (variable `char_elem_result`) para guardar la característica que encuentre después la función `esp_ble_gattc_get_char_by_uuid()`. Esta función busca la característica con el UUID de característica `0xFF01` (usando `remote_filter_char_uuid`). Cabe destacar que en un servidor puede haber más de una característica con el mismo UUID. Sin embargo, en nuestro ejemplo de servidor GATT, cada característica tiene un UUID único y es por eso que sólo usamos la primera característica de `char_elem_result` (index 0).

## Registro para notificaciones

El cliente puede registrarse para recibir notificaciones del servidor cada vez que cambia el valor de la característica. En este ejemplo, queremos registrarnos para recibir notificaciones de la característica identificada con el UUID `0xFF01`.

Después de obtener la característica, verificamos sus propiedades (lectura, escritura, notificaciones...) y utilizamos la función `esp_ble_gattc_register_for_notify()` para registrarnos localmente y recibir notificaciones. Los argumentos de la función son: la interfaz GATT, la dirección del servidor GATT remoto y el handle del que queremos recibir notificaciones.

```c
…
if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
{
    gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
    esp_ble_gattc_register_for_notify(gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
}
…
```

Este procedimiento registra notificaciones en la pila BLE y desencadena un evento `ESP_GATTC_REG_FOR_NOTIFY_EVT`. Este evento se utiliza para escribir en el descriptor CCC (Client Characteristic Configuration) del servidor con el fin de habilitar las notificaciones para dicho cliente:

```c
case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
    …
    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
        ret_status = esp_ble_gattc_write_char_descr(gattc_if,
                                                    gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                    descr_elem_result[0].handle,
                                                    sizeof(notify_en),
                                                    (uint8_t *)&notify_en,
                                                    ESP_GATT_WRITE_TYPE_RSP,
                                                    ESP_GATT_AUTH_REQ_NONE);
    }
    …
}
```

Donde `ESP_GATT_UUID_CHAR_CLIENT_CONFIG` se define con el UUID estándar que identifica a un descriptor CCC:

```c
#define ESP_GATT_UUID_CHAR_CLIENT_CONFIG 0x2902
```

En el evento se utiliza la función `esp_ble_gattc_write_char_descr()` para escribir en el descriptor CCC. El valor a escribir es "1" (variable `notify_en`) para habilitar las notificaciones. También pasamos `ESP_GATT_WRITE_TYPE_RSP` para solicitar que el servidor responda a la solicitud de habilitar las notificaciones y `ESP_GATT_AUTH_REQ_NONE` para indicar que la solicitud de escritura no requiere autenticación.

!!! danger "Ejercicio 6"
    Conecta el cliente GATT con el servidor GATT modificado del último ejercicio de la práctica anterior para que las notificaciones sean recibidas y mostradas por consola.
