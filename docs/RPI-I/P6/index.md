# LAB6. BLE: cliente GATT

## Objetivos

* Analizar en detalle el *firmware* de un cliente GATT utilizando la API de ESP-IDF.
* Aprender a realizar un escaneo de dispositivos BLE.
* Extraer e interpretar la información que contienen los anuncios BLE.
* Gestionar la conexión desde un cliente BLE a un servidor GATT.

# Implementación de un cliente GATT

## Introducción

En esta práctica desplegaremos un cliente GATT utilizando la API de ESP-IDF. El código del ejemplo que vamos a analizar permite escanear dispositivos BLE y establecer una conexión con un servidor GATT que ofrece un servicio predefinido (el *Heart Rate Service*).

Una vez establecida la conexión, el cliente descubrirá los servicios y características disponibles en el servidor. El ejemplo registra un perfil de aplicación (modo cliente) e inicializa una secuencia de manejadores de eventos que permiten configurar los parámetros GAP y gestionar las diferentes operaciones del cliente: el escaneo de dispositivos, la conexión con servidores GATT, el descubrimiento de atributos, y la lectura, escritura y suscripción a características.

Para realizar esta práctica se necesitan dos placas ESP32, una ejecutando el servidor GATT modificado de la práctica anterior y otra ejecutando el código del cliente (ejemplo ***gatt_client*** del directorio *bluetooth/bluedroid/ble*).

!!! note "Nota"
    Antes de empezar, asegúrate de que la variable `remote_device_name` NO coincide con el nombre de tu servidor GATT (de lo contrario se establecería la conexión automáticamente).

## Inclusión de cabeceras

Observa los ficheros de cabecera necesarios:

```c
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
```

Al igual que en la implementación del servidor, estas cabeceras son necesarias para el correcto funcionamiento de FreeRTOS y de sus componentes. Resultan especialmente interesantes las cabeceras `esp_bt.h`, `esp_bt_main.h`, `esp_gap_ble_api.h` y `esp_gattc_api.h`, las cuales exponen las APIs BLE necesarias para la implementación del cliente:

* `esp_bt.h`: proporciona las funciones relacionadas con el controlador Bluetooth.
* `esp_bt_main.h`: declara las funciones de inicialización y activación de la pila Bluedroid.
* `esp_gap_ble_api.h`: proporciona la API GAP (para la configuración de los parámetros de anuncio y conexión).
* `esp_gattc_api.h`: proporciona la API del cliente GATT (para la conexión a servidores y el descubrimiento de atributos).

## Inicio de la aplicación

El punto de entrada de la aplicación se implementa mediante la función `app_main()`. Esta comienza inicializando la biblioteca de almacenamiento no volátil, que permite almacenar pares clave-valor en la memoria flash. Esta biblioteca es utilizada por diferentes componentes de ESP-IDF, como por el controlador Wi-Fi para almacenar el SSID y la contraseña de la red.

La función principal también inicializa el controlador Bluetooth. Para ello, primero crea una estructura de configuración de tipo `esp_bt_controller_config_t`, cuyos campos se inicializan con los valores por defecto proporcionados por la macro `BT_CONTROLLER_INIT_CONFIG_DEFAULT()`. Recuerda que el controlador Bluetooth implementa la interfaz *Host Controller Interface* (HCI), la capa de enlace y la capa física. Una vez creada la estructura de configuración, se inicializa el controlador mediante la función `esp_bt_controller_init()`:

```c
esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
ret = esp_bt_controller_init(&bt_cfg);
```

A continuación, se habilita el controlador en modo BLE (usando la opción `ESP_BT_MODE_BLE`):

```c
ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
```

Después de activar el controlador Bluetooth, se inicializa y habilita la pila Bluedroid (capas superiores), que incluye APIs tanto para BLE como para Bluetooth clásico:

```c
ret = esp_bluedroid_init_with_cfg(&cfg);    /* Configuración de seguridad para Bluetooth clásico */
ret = esp_bluedroid_enable();
```

La función principal finaliza registrando dos funciones de *callback* para gestionar los eventos BLE, una para GAP y otra para GATT (cliente), así como registrando el perfil de aplicación (modo cliente) y configurando el tamaño máximo de MTU admitido:

```c
// Register the callback function to the gap module
ret = esp_ble_gap_register_callback(esp_gap_cb);

// Register the callback function to the gattc module
ret = esp_ble_gattc_register_callback(esp_gattc_cb);

ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);

esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
```

El manejador de eventos GAP (`esp_gap_cb()`) se encarga de gestionar el escaneo y la conexión con servidores GATT. Por su parte, el manejador GATT (`esp_gattc_cb()`) gestionará las operaciones propias del cliente GATT, como el descubrimiento de atributos y la lectura y escritura de datos.

## Perfiles de aplicación

Las estructuras que representan los perfiles de aplicación desde el punto de vista del cliente GATT son instancias del tipo personalizado `gattc_profile_inst`:

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

* `gattc_cb`: función de *callback* del perfil de aplicación.
* `gattc_if`: ID de la interfaz lógica del cliente asociada al perfil.
* `app_id`: ID asignado al perfil de aplicación.
* `conn_id`: ID de la conexión GATT.
* `service_start_handle`: handle de inicio del servicio descubierto.
* `service_end_handle`: handle de fin del servicio descubierto.
* `char_handle`: handle de la característica objetivo (*Heart Rate Measurement*).
* `remote_bda`: dirección Bluetooth del dispositivo remoto al que está conectado el cliente.

En el ejemplo se utiliza un único perfil de aplicación, asociado al servicio *Heart Rate*, cuyo identificador/índice es 0:

```c
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
```

El perfil de aplicación se almacena en el array `gl_profile_tab`:

```c
/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};
```

En la inicialización del array `gl_profile_tab` se configura el manejador del perfil de aplicación (`gattc_profile_event_handler()`). Además, `gattc_if` se inicializa con el valor `ESP_GATT_IF_NONE` (sin interfaz).

Cuando se registra el perfil mediante `esp_ble_gattc_app_register()`, se desencadena el evento `ESP_GATTC_REG_EVT`, que es gestionado por el manejador de eventos GATT `esp_gattc_cb()`. Este toma el ID de la interfaz GATT proporcionado en el evento y lo almacena en la tabla de perfiles:

```c
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }
    ...
```

Finalmente, la función delega el manejo de eventos en el *callback* asociado al perfil de aplicación (`gattc_profile_event_handler()`):

```c
    ...
    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}
```

## Parámetros GAP (escaneo)

El cliente GATT normalmente escanea servidores GATT y trata de conectarse a estos si está interesado. Sin embargo, antes de realizar el escaneo, es necesario configurar sus parámetros. Esta configuración se lleva a cabo durante la gestión del evento `ESP_GATTC_REG_EVT` (manejador `gattc_profile_event_handler()`). En este, el evento se utiliza para llamar a la función `esp_ble_gap_set_scan_params()`, que recibe como argumento una estructura de tipo `esp_ble_scan_params_t` con los parámetros de escaneo:

```c
typedef struct {
    esp_ble_scan_type_t     scan_type;              /*!< Tipo de escaneo */
    esp_ble_addr_type_t     own_addr_type;          /*!< Tipo de dirección propia */
    esp_ble_scan_filter_t   scan_filter_policy;     /*!< Política de filtrado del escaneo */
    uint16_t                scan_interval;          /*!< Intervalo de escaneo. Se define como el intervalo de tiempo desde que el controlador comenzó su último escaneo LE hasta que comienza el siguiente */
                                                    // Rango: 0x0004 a 0x4000
                                                    // Predeterminado: 0x0010 (10 ms)
                                                    // Tiempo = N * 0.625 ms
                                                    // Rango de tiempo: 2.5 ms a 10.24 segundos
    uint16_t                scan_window;            /*!< Ventana de escaneo. La duración del escaneo LE. LE_Scan_Window debe ser menor o igual que LE_Scan_Interval */
                                                    // Rango: 0x0004 a 0x4000
                                                    // Predeterminado: 0x0010 (10 ms)
                                                    // Tiempo = N * 0.625 ms
                                                    // Rango de tiempo: 2.5 ms a 10.24 segundos
    esp_ble_scan_duplicate_t  scan_duplicate;       /*!< Indica si se deben filtrar los anuncios duplicados */
} esp_ble_scan_params_t;
```

La estructura se inicializa de la siguiente manera:

```c
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = ESP_BLE_GAP_SCAN_ITVL_MS(50),
    .scan_window            = ESP_BLE_GAP_SCAN_WIN_MS(30),
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};
```

Los parámetros de escaneo se configuran para realizar un escaneo activo (incluye el envío de *scan requests*), utilizar una dirección Bluetooth pública y aceptar anuncios de cualquier dispositivo. Además, se establece un intervalo de escaneo de 50 ms (0x50 * 0.625 ms) y una ventana de escaneo de 30 ms (0x30 * 0.625 ms).

!!! danger "Ejercicio 1"
    Modifica los parámetros de escaneo para que este se realice con menor frecuencia. Añade estos parámetros a través de *menuconfig*. ¿Qué diferencia hay entre el intervalo de escaneo y la ventana de escaneo? Muestra cómo cambia el comportamiento del escaneo al modificar ambos parámetros.

Al establecer los parámetros de escaneo con `esp_ble_gap_set_scan_params()` se desencadena el evento `ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT`, que es gestionado por el manejador de eventos GAP `esp_gap_cb()`. Este evento se utiliza para iniciar el escaneo con la función `esp_ble_gap_start_scanning()`.

!!! danger "Ejercicio 2"
    ¿Por qué llega un momento en el que el escaneo se detiene? Analiza el código y modifícalo para que el proceso de escaneo se realice indefinidamente. Comprueba que el tiempo durante el cual el dispositivo permanece en fase de escaneo corresponde al valor seleccionado.

Los resultados del escaneo se muestran por consola a medida que se reciben mediante el evento `ESP_GAP_BLE_SCAN_RESULT_EVT`. Cada resultado se representa con una estructura de tipo `ble_scan_result_evt_param`:

```c
struct ble_scan_result_evt_param {
    esp_gap_search_evt_t search_evt;            /*!< Tipo de subevento */
    esp_bd_addr_t bda;                          /*!< Dirección Bluetooth del dispositivo detectado */
    esp_bt_dev_type_t dev_type;                 /*!< Tipo de dispositivo */
    esp_ble_addr_type_t ble_addr_type;          /*!< Tipo de dirección BLE del dispositivo */
    esp_ble_evt_type_t ble_evt_type;            /*!< Tipo de anuncio BLE recibido */
    int rssi;                                   /*!< RSSI del dispositivo detectado (potencia recibida) */
    uint8_t ble_adv[ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX]; /*!< Datos del anuncio recibido */
    int flag;                                   /*!< Flag de los datos del anuncio */
    int num_resps;                              /*!< Número de resultados de escaneo asociados al dispositivo */
    uint8_t adv_data_len;                       /*!< Longitud del anuncio recibido (en bytes) */
    uint8_t scan_rsp_len;                       /*!< Longitud del scan response recibido (en bytes) */
    uint32_t num_dis;                           /*!< Número de resultados de escaneo descartados */
} scan_rst;
```

Dentro del evento `ESP_GAP_BLE_SCAN_RESULT_EVT`, los resultados del escaneo se gestionan según el valor del campo `search_evt`, que permite identificar el tipo de subevento recibido. Entre sus posibles valores se encuentran:

```c
typedef enum {
    ESP_GAP_SEARCH_INQ_RES_EVT             = 0,      /*!< Resultado de búsqueda de un dispositivo */
    ESP_GAP_SEARCH_INQ_CMPL_EVT            = 1,      /*!< Búsqueda finalizada */
    ...
} esp_gap_search_evt_t;
```

Concretamente, nos interesa el evento `ESP_GAP_SEARCH_INQ_RES_EVT`, que se desencadena cada vez que se detecta un dispositivo. También puede resultar útil el evento `ESP_GAP_SEARCH_INQ_CMPL_EVT`, que se emite cuando finaliza el escaneo y podría utilizarse para reiniciarlo:

```c
case ESP_GAP_BLE_SCAN_RESULT_EVT: {
    esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
    switch (scan_result->scan_rst.search_evt) {
    case ESP_GAP_SEARCH_INQ_RES_EVT:
        adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                    scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                    ESP_BLE_AD_TYPE_NAME_CMPL,
                                                    &adv_name_len);
        ESP_LOGI(GATTC_TAG, "Scan result, device "ESP_BD_ADDR_STR", name len %u", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
        ESP_LOG_BUFFER_CHAR(GATTC_TAG, adv_name, adv_name_len);
        ...
        if (adv_name != NULL) {
            if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                // Note: If there are multiple devices with the same device name, the device may connect to an unintended one.
                // It is recommended to change the default device name to ensure it is unique.
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
        break;
    default:
        break;
    }
    break;
}
```

En el código anterior se obtiene/resuelve el nombre del dispositivo remoto a partir de los datos del anuncio recibido (función `esp_ble_resolve_adv_data_by_type()`) y se compara con el nombre del servidor GATT que nos interesa (variable `remote_device_name`). Si ambos nombres coinciden, se detiene el escaneo con `esp_ble_gap_stop_scanning()`. Recuerda que al principio de la práctica modificamos la variable `remote_device_name` para que no coincidiera con el nombre de nuestro servidor GATT.

!!! danger "Ejercicio 3"
    Muestra también por pantalla el valor del RSSI de cada dispositivo BLE detectado.

!!! danger "Ejercicio 4"
    Añade a *menuconfig* un campo que permita configurar la dirección Bluetooth de un dispositivo conocido. Modifica el código para que únicamente se muestre la información del dispositivo cuya dirección se haya configurado.

!!! danger "Ejercicio 5"
    Modifica el código para que, en función del valor de RSSI obtenido para el dispositivo de interés, el cliente muestre por pantalla un indicador proporcional a la distancia aproximada a dicho dispositivo (por ejemplo, un número concreto de puntos según la distancia). Para ello, establece un valor mínimo y otro máximo de RSSI y aplica la siguiente fórmula: `(RSSI - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)`. Prueba a alejar físicamente los dispositivos entre sí.

## Establecimiento de la conexión

!!! note "Nota"
    A partir de aquí puedes volver a configurar la variable `remote_device_name` con el nombre asignado a tu servidor GATT.

Si el valor de `remote_device_name` coincide con el nombre del dispositivo remoto, el cliente detiene el escaneo e intenta establecer una conexión con este utilizando la función `esp_ble_gattc_enh_open()`. Esta función recibe, entre otros parámetros, la interfaz GATT del perfil de aplicación y la dirección Bluetooth del servidor remoto.

Ten en cuenta que, además de la conexión física, el cliente establece una conexión virtual con el servidor, a la que se le asigna un ID (`conn_id`). Dado que pueden ejecutarse varios perfiles de aplicación en un mismo ESP32, es posible establecer varias conexiones virtuales con un mismo servidor remoto.

En el cliente, cada perfil de aplicación registrado crea una conexión virtual con el servidor mediante la función `esp_ble_gattc_enh_open()`. Por tanto, si posteriormente se llamase a la función `esp_ble_gattc_close()`, solo se cerraría la conexión virtual correspondiente a ese perfil de aplicación. En cambio, si se llama a la función de desconexión GAP (`esp_ble_gap_disconnect()`), se cerrarán ambas conexiones (física y todas las virtuales).

Una vez establecida la conexión, se desencadena el evento `ESP_GATTC_CONNECT_EVT`:

```c
case ESP_GATTC_CONNECT_EVT:{
    ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"", p_data->connect.conn_id, ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
    gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
    memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
    ...
}
```

Como se observa en el código, el ID de conexión (`conn_id`) y la dirección del dispositivo remoto (`remote_bda`) se almacenan en la tabla de perfiles de aplicación y se imprimen por consola.





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
    ...
    if (get_server) {
        uint16_t count = 0;
        esp_gatt_status_t status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                p_data->search_cmpl.conn_id,
                                                                ESP_GATT_DB_CHARACTERISTIC,
                                                                gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                INVALID_HANDLE,
                                                                &count);
        ...
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
                ...
            }
        }
        else {
            ESP_LOGE(GATTC_TAG, "no char found");
        }
    }
    break;
```

`esp_ble_gattc_get_attr_count()` obtiene el número de atributos de tipo característica en un rango concreto. Los parámetros de esta función son: la interfaz GATT, el ID de conexión, el tipo de atributo que se busca (`ESP_GATT_DB_CHARACTERISTIC`), handles de inicio y final, el handle de la característica (este parámetro solo es válido cuando el tipo se establece en `ESP_GATT_DB_DESCRIPTOR`) y la salida del número de atributos encontrados.

Después, se reserva memoria (variable `char_elem_result`) para guardar la característica que encuentre después la función `esp_ble_gattc_get_char_by_uuid()`. Esta función busca la característica con el UUID de característica `0xFF01` (usando `remote_filter_char_uuid`). Cabe destacar que en un servidor puede haber más de una característica con el mismo UUID. Sin embargo, en nuestro ejemplo de servidor GATT, cada característica tiene un UUID único y es por eso que solo usamos la primera característica de `char_elem_result` (index 0).

## Registro para notificaciones

El cliente puede registrarse para recibir notificaciones del servidor cada vez que cambia el valor de la característica. En este ejemplo, queremos registrarnos para recibir notificaciones de la característica identificada con el UUID `0xFF01`.

Después de obtener la característica, verificamos sus propiedades (lectura, escritura, notificaciones...) y utilizamos la función `esp_ble_gattc_register_for_notify()` para registrarnos localmente y recibir notificaciones. Los argumentos de la función son: la interfaz GATT, la dirección del servidor GATT remoto y el handle del que queremos recibir notificaciones.

```c
if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
{
    gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
    esp_ble_gattc_register_for_notify(gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
}
```

Este procedimiento registra notificaciones en la pila BLE y desencadena un evento `ESP_GATTC_REG_FOR_NOTIFY_EVT`. Este evento se utiliza para escribir en el descriptor CCC (Client Characteristic Configuration) del servidor con el fin de habilitar las notificaciones para dicho cliente:

```c
case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
    ...
    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
        ret_status = esp_ble_gattc_write_char_descr(gattc_if,
                                                    gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                    descr_elem_result[0].handle,
                                                    sizeof(notify_en),
                                                    (uint8_t *)&notify_en,
                                                    ESP_GATT_WRITE_TYPE_RSP,
                                                    ESP_GATT_AUTH_REQ_NONE);
    }
    ...
}
```

Donde `ESP_GATT_UUID_CHAR_CLIENT_CONFIG` se define con el UUID estándar que identifica a un descriptor CCC:

```c
#define ESP_GATT_UUID_CHAR_CLIENT_CONFIG 0x2902
```

En el evento se utiliza la función `esp_ble_gattc_write_char_descr()` para escribir en el descriptor CCC. El valor a escribir es "1" (variable `notify_en`) para habilitar las notificaciones. También pasamos `ESP_GATT_WRITE_TYPE_RSP` para solicitar que el servidor responda a la solicitud de habilitar las notificaciones y `ESP_GATT_AUTH_REQ_NONE` para indicar que la solicitud de escritura no requiere autenticación.

!!! danger "Ejercicio 6"
    Conecta el cliente GATT con el servidor GATT modificado del último ejercicio de la práctica anterior para que las notificaciones sean recibidas y mostradas por consola.
