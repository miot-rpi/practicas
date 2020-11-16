# Práctica 4. Bluetooth Low Energy (BLE)

# Objetivos

* Bla
* Bla
* Bla

# Implementación de un servidor GATT basado en tablas

## Introducción

This document presents a walkthrough of the GATT Server Service Table example code for the ESP32. This example implements a Bluetooth Low Energy (BLE) Generic Attribute (GATT) Server using a table-like data structure to define the server services and characteristics such as the one shown in the figure below Therefore, it demonstrates a practical way to define the server functionality in one place instead of adding services and characteristics one by one. 

This example implements the *Heart Rate Profile* as defined by the [Traditional Profile Specifications](https://www.bluetooth.com/specifications/profiles-overview).

<div align="center"><img src="image/Heart_Rate_Service.png" width = "450" alt="Table-like data structure representing the Heart Rate Service" align=center /> </div>

## Inclusión de encabezados

Los siguientes ficheros de cabecera son necesarios para dotar de funcionalidad
BLE a nuestro *firmware*:

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bt.h"
#include "bta_api.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"
#include “gatts_table_creat_demo.h"
```

Estos encabezados son necesarios para un correcto funcionamiento de *FreeRTOS*
y de sus componentes, incluyendo funcionalidad relativa a *logging* y 
almacenamiento no volátil. 
Son especialmente interesantes los ficheros ``bt.h``, ``esp_bt_main.h``, 
``esp_gap_ble_api.h`` y ``esp_gatts_api.h``, ya que exponen la API BLE necesaria
para la implementación del *firmware*:

* ``bt.h``: implementa el controlador Bluetooth.
* ``esp_bt_main.h``: implementa las rutinas de inicialización y activación de la pila Bluedroid.
* ``esp_gap_ble_api.h``: implements GAP configuration such as advertising and connection parameters.
* ``esp_gap_ble_api.h``: implementa la configuración GAP (parámetros de anuncios y conexión).
* ``esp_gatts_api.h``: immplementa la configuración del servidor GATT (por ejemplo, la creación de servicios y características).

## La tabla de servicios

El fichero de encabezado [gatts_table_creat_demo.h](../main/gatts_table_creat_demo.h) 
contiene una enumeración de los servicios y características deseadas:

```c
enum
{
    HRS_IDX_SVC,

    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_NTF_CFG,

    HRS_IDX_BOBY_SENSOR_LOC_CHAR,
    HRS_IDX_BOBY_SENSOR_LOC_VAL,

    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,

    HRS_IDX_NB,
};
```

Los elementos de la anterior estructura se han incluido en el mismo orden
que los atributos del *Heart Rate Profile*, comenzando con el servicio, seguido
por las características del mismo. Además, la característica *Heart Rate Measurement*
dispone de configuración propia (*Client Characteristic Configuration*,
o CCC), un descriptor que **describe si la característica tiene las notificaciones
activas**. Todos estos índices pueden utilizarse para identificar a cada elemento 
a la hora de crear la tabla de atributos:

* ``HRS_IDX_SVC``: índice del servicio Heart Rate.
* ``HRS_IDX_HR_MEAS_CHAR``: índice de la característica Heart Rate Measurement.
* ``HRS_IDX_HR_MEAS_VAL``: índice del valor Heart Rate Measurement. 
* ``HRS_IDX_HR_MEAS_NTF_CFG``: índice de la configuración de notificaciones (CCC) Heart Rate Measurement.
* ``HRS_IDX_BOBY_SENSOR_LOC_CHAR``: índice de la característica Heart Rate Body Sensor Location.
* ``HRS_IDX_BOBY_SENSOR_LOC_VAL``: índice del valor Heart Rate Body Sensor Location.
* ``HRS_IDX_HR_CTNL_PT_CHAR``: índice de la característica Heart Rate Control Point.
* ``HRS_IDX_HR_CTNL_PT_VAL``: índice del valor Heart Rate Control Point.
* ``HRS_IDX_NB``: número de elementos d ela tabla.

## Punto de entrada

El punto de entrada de la aplicación (``app_main()``) se implementa como 
sigue:

```c
void app_main()
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ESP_LOGI(GATTS_TABLE_TAG, "%s init bluetooth\n", __func__);
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed\n", __func__);
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_HEART_RATE_APP_ID);
    return;
}
```

La función principal procede incializando el almacenamiento no volátil, para 
almacenar los parámetros necesarios en memoria *flash*:

```c
ret = nvs_flash_init();
```

## Inicialización del controlador y de la pila Bluetooth

La función principal inicializa también el controlador Bluetooth, creando
en primer lugar una estructura de configuración para tal fin de tipo 
`esp_bt_controller_config_t` con valores por defecto dictados por la macro               `BT_CONTROLLER_INIT_CONFIG_DEFAULT()`. 

El controlador Bluetooth implementa el *Host Controller Interface* (HCI), la
capa de enlace y la capa física BLE; es, por tanto, transparente para el programador. 
La configuración incluye el tamaño de pila reservado al controlador, prioridad 
y baudios para la transmisión. Con estas configuraciones, el controlador
puede ser inicializado y activado con la función `esp_bt_controller_init()`:

```c
esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
ret = esp_bt_controller_init(&bt_cfg);
```

A continuación, el controlador activa el modo BLE:

```c
ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
```

Existen cuatro modos de funcioinamiento Bluetooth:

1. `ESP_BT_MODE_IDLE`: Bluetooth no funcional
2. `ESP_BT_MODE_BLE`: Modo BLE
3. `ESP_BT_MODE_CLASSIC_BT`: Modo BT Clásico
4. `ESP_BT_MODE_BTDM`: Modo Dual (BLE + BT Clásico)

Tras la incialización del controlador Bluetooth, la pila Bluedroid (que 
incluye APIs tanto para BLE como para Bluetooth Clásico) debe ser inicializada
y activada:

```c
ret = esp_bluedroid_init();
ret = esp_bluedroid_enable();
```

La pila Bluetooth está, a partir de este punto, lista para funcionar, pero todavía
no se ha implementado ninguna lógica de aplicación. Dicha funcionalidad
se define con el clásico mecanismo basado en eventos, que pueden ser emitidos,
por ejemplo, cuando otro dispositivo intenta leer o escribir parámetros, o
establecer una conexión. 

Existen dos gestores de eventos relacionados con BLE: los manejadores 
(*handlers*) GAP y GATT. La aplicación necesita registrar una función de 
*callback* para cada manejador, para permitir a la aplicación conocer qué 
funciones se invocarán eventos de tipo GAP y GATT:

```c
esp_ble_gatts_register_callback(gatts_event_handler);
esp_ble_gap_register_callback(gap_event_handler);
```

Las funciones `gatts_event_handler()` y `gap_event_handler()` 
manejan todos los eventos emitidos por la pila BLE hacia la plicación.

## Perfiles de aplicación (*Application profiles*)

Como se ha dicho, el objetivo es implementar un Perfil de Aplicación 
para el servicio *Heart Rate*. Un Perfil de Aplicación es un mecanismo que
permite agrupar funcionalidad diseñada para ser utilizada por un cliente
de la aplicación, por ejemplo, una aplicación móvil. En este sentido, 
diferentes tipos de perfiles pueden acomodarse en un mismo servidor.

El Identifificador de Perfil de Aplicación (*Application Profile ID*) es un valor
seleccionable por el usuario para identificar cada perfil; su uso se recude al
registro del perfil en la pila Bluetooth. En el ejemplo, el ID es `0x55`.

```c
#define HEART_PROFILE_NUM                       1
#define HEART_PROFILE_APP_IDX                   0
#define ESP_HEART_RATE_APP_ID                   0x55
```

Los perfiles se almacenan en el array ``heart_rate_profile_tab``. 
Al haber un único perfil en el ejemplo, sólo se almacena un elemento en el 
array, con índice 0 (tal y como se define en ``HEART_PROFILE_APP_IDX``). 
Además, es necesario inicializar la función de *callback* manejadora de los
eventos del perfil. Cada aplicación en el servidor GATT utiliza una interfaz
diferenciada, representada por el parámetro `gats_if`. Para la incialización,
este parámetro se iguala a ``ESP_GATT_IF_NONE``; 
cuando la aplicación se registre, más adelante, el parámetro `gatts_if` se 
actualizará con la interfaz generada automáticamente por la pila Bluetooth.

```c
/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[HEART_PROFILE_NUM] = {
    [HEART_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },

};
```


El registro de la aplicación tiene lugar en la función ``app_main()``,
utilizando la función ``esp_ble_gatts_app_register()``:

```c
esp_ble_gatts_app_register(ESP_HEART_RATE_APP_ID);
```

## Parámetros GAP

El evento de registro de aplicación es el primero que se invoca durante
la vida de un programa. Este ejemplo utiliza este evento para configurar 
parámetros GAP (de anuncio). Las funciones asociadas son:

* ``esp_ble_gap_set_device_name()``: utilizada para establecer el nombre del dispositivo anunciado.
* ``esp_ble_gap_config_adv_data()``: usada para configurar datos estándar de anuncio.

La función utilizada para configurar los parámetros estándar 
(``esp_ble_gap_config_adv_data()``) toma un puntero a una estructura de tipo ``esp_ble_adv_data_t``. La estructura ``esp_ble_adv_data_t`` dispone de los siguientes campos:

```c
typedef struct {
    bool set_scan_rsp;    /*!< Set this advertising data as scan response or not*/
    bool include_name;    /*!< Advertising data include device name or not */
    bool include_txpower; /*!< Advertising data include TX power */
    int min_interval;     /*!< Advertising data show slave preferred connection min interval */
    int max_interval;     /*!< Advertising data show slave preferred connection max interval */
    int appearance;       /*!< External appearance of device */
    uint16_t manufacturer_len; /*!< Manufacturer data length */
    uint8_t *p_manufacturer_data; /*!< Manufacturer data point */
    uint16_t service_data_len;    /*!< Service data length */
    uint8_t *p_service_data;      /*!< Service data point */
    uint16_t service_uuid_len;    /*!< Service uuid length */
    uint8_t *p_service_uuid;      /*!< Service uuid array point */
    uint8_t flag;         /*!< Advertising flag of discovery mode, see BLE_ADV_DATA_FLAG detail */
} esp_ble_adv_data_t;
```

En el ejemplo, la estructura se incializará como sigue:

```c
static esp_ble_adv_data_t heart_rate_adv_config = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(heart_rate_service_uuid),
    .p_service_uuid = heart_rate_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
```
Los intervalos mínimos y máximos de conexión se establecen en unidades de
1.25 ms. En el ejemplo, el intervalo de conexión mínimo preferido se establece, 
por tanto, en 7.5 ms y el máximo en 20 ms.

El *payload* del anuncio puede almacenar hasta 31 bytes de datos. 
Es posible que algunos parámetros los superen, pero en dicho caso el stack
BLE cortará el mensaje y eliminará aquellos que superen el tamaño máximo.
Por último, para establecer el nombre del dispositivo se puede utilizar la 
función ``esp_ble_gap_set_device_name()``. 

Para regitrar el manejador de eventos, procedemos de la siguiente forma:

```c
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, 
esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGE(GATTS_TABLE_TAG, "event = %x\n",event);
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
            esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
            esp_ble_gap_config_adv_data(&heart_rate_adv_config);
            ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
…
```

## El manejador de eventos GAP

Una vez establecidos los datos de anuncio, se emite un evento de tipo 
``ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT``, que será manejado por el manejador
 GAP configurado. Además, se emite también un evento de tipo
 ``ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT`` si se ha configurado una respuesta
 al escaneado.
Así, el manejador puede utilizar cualquiera de estos dos eventos para comenzar
con el proceso de anuncio, utilizando la función
``esp_ble_gap_start_advertising()``:

```c
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{   
    ESP_LOGE(GATTS_TABLE_TAG, "GAP_EVT, event %d\n", event);
    
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&heart_rate_adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed\n");
        }
        break;
    default:
        break;
    }
}
```

La función de inicio de anuncios toma una estructura de tipo 
``esp_ble_adv_params_t`` con los parámetros de anuncio requeridos:

```c
/// Advertising parameters
typedef struct {
    uint16_t adv_int_min; /*!< Minimum advertising interval for undirected and low duty cycle directed advertising.
    Range: 0x0020 to 0x4000
    Default: N = 0x0800 (1.28 second)
    Time = N * 0.625 msec
    Time Range: 20 ms to 10.24 sec */
    uint16_t adv_int_max; /*!< Maximum advertising interval for undirected and low duty cycle directed advertising.
    Range: 0x0020 to 0x4000
    Default: N = 0x0800 (1.28 second)
    Time = N * 0.625 msec
    Time Range: 20 ms to 10.24 sec */
    esp_ble_adv_type_t adv_type;            /*!< Advertising type */
    esp_ble_addr_type_t own_addr_type;      /*!< Owner bluetooth device address type */
    esp_bd_addr_t peer_addr;                /*!< Peer device bluetooth device address */
    esp_ble_addr_type_t peer_addr_type;     /*!< Peer device bluetooth device address type */
    esp_ble_adv_channel_t channel_map;      /*!< Advertising channel map */
    esp_ble_adv_filter_t adv_filter_policy; /*!< Advertising filter policy */
} esp_ble_adv_params_t;
```

Nótese como ``esp_ble_gap_config_adv_data()`` configura los datos que son
aunciados al cliente y toma una estructura de tipo ``esp_ble_adv_data_t structure``, 
mientras que ``esp_ble_gap_start_advertising()`` hace que el servidor realmente
comience a anunciar, tomando una estructura de tipo ``esp_ble_adv_params_t``. 
Los datos de anuncio son aquellos que realmente se envían al cliente, mientras
que los parámetros de anuncio son la configuración requerida por la pila BLE
para actuar correctamente.

Para este ejemplo, los parámetros de anuncio se inicializarán como sigue:

```c
static esp_ble_adv_params_t heart_rate_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
```

Estos parámetros configuran el intervalo de anuncio entre 20 ms y 40 ms. 
El anuncio es de tipo `ADV_TYPE_IND` (tipo genérico), destinados a ningún dispositivo
central en particular, y anuncia que el servidor GATT es conectable. El tipo 
de dirección es público, utiliza todos los canales y permite peticiones de 
escaneo y conexión por parte de cualquier dispositivo central.

Si el proceso de anuncio se inició correctamente, se emitirá un evento de tipo
``ESP_GAP_BLE_ADV_START_COMPLETE_EVT``, que en este ejemplo se utiliza para comprobar
si el estado de anuncio es realmente *anunciando* u otro, en cuyo caso se 
emitirá un mensaje de error:

```c
...
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed\n");
        }
        break;
...
```

## Manejadores de eventos GATT

Al registrar un Pefil de Aplicación, se emite un evento de tipo
``ESP_GATTS_REG_EVT``. 
Los parámetros asociados al evento son:

```c
esp_gatt_status_t status;    /*!< Operation status */
uint16_t app_id;             /*!< Application id which input in register API */
```

Además de los anteriores parámetros, el evento también contiene la interfaz
GATT asignada por la pila BLE, a utilizar a partir de ahora. El evento es capturado
por el manejador ``gatts_event_handler()``, que almacena la interfaz generada
en la tabla de perfiles, y la reenvía al manejador de eventos correspondiente 
al perfil:

```c
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "EVT %d, gatts if %d\n", event, gatts_if);

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            heart_rate_profile_tab[HEART_PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TABLE_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < HEART_PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            gatts_if == heart_rate_profile_tab[idx].gatts_if) {
                if (heart_rate_profile_tab[idx].gatts_cb) {
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}
```

## Creación de Servicios y Características con una Tabla de Atributos

Aprovecharemos el evento de tipo Registro para crear una tabla de atributos
de perfil utilizando la función ``esp_ble_gatts_create_attr_tab()``.
Esta función toma como argumento una estructura de tipo ``esp_gatts_attr_db_t``,
que corresponde a una tabla de *lookup* 
indexada por los valores de la enumeración definidos en el fichero de cabecera.
La estructura ``esp_gatts_attr_db_t`` tiene dos miembros:

```c
esp_attr_control_t    attr_control;       /*!< The attribute control type*/
esp_attr_desc_t       att_desc;           /*!< The attribute type*/
```

* `attr_control` es el parámetro de autorespuesta, típicamente fijado a 
``ESP_GATT_AUTO_RSP`` para permitir que la pila BLE reponda automáticamente a los
mensajes de lectura o escritura cuando dichos eventos son recibidos. 
Una opción alternativa es ``ESP_GATT_RSP_BY_APP`` que permite respuestas 
manuales utilizando la función ``esp_ble_gatts_send_response()``.

* `att_desc` es la descripción del atributo, formada por:

```c
uint16_t uuid_length;      /*!< UUID length */  
uint8_t  *uuid_p;          /*!< UUID value */  
uint16_t perm;             /*!< Attribute permission */        
uint16_t max_length;       /*!< Maximum length of the element*/    
uint16_t length;           /*!< Current length of the element*/    
uint8_t  *value;           /*!< Element value array*/ 
```

Por ejemplo, el primer elemento de la tabla en el ejemplo es el atributo de servicio:

```c
[HRS_IDX_SVC]                       =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(heart_rate_svc), (uint8_t *)&heart_rate_svc}},
```

Los valores de inicialización son:

* ``[HRS_IDX_SVC]``: Inicializador en la tabla.
* ``ESP_GATT_AUTO_RSP``: configuración de respuesta automática, fijada en este
    caso a respuesta automática por parte de la pila BLE.
* ``ESP_UUID_LEN_16``: longitudo del UUID fijada a 16 bits.
* ``(uint8_t *)&primary_service_uuid``: UUID para identificar al servicio como primario (0x2800).
* ``ESP_GATT_PERM_READ``: Permisos de lectura para el servicio.
* ``sizeof(uint16_t)``: Longitud máxima del UUID del servicio (16 bits).
* ``sizeof(heart_rate_svc)``: Longitud del servicio, en este caso 16 bits (fijada por el tamaño de la variable *heart_rate_svc*).
* ``(uint8_t *)&heart_rate_svc``: Valor del atributo servicio fijada a la 
 variable the variable *heart_rate_svc*, que contiene el UUID del *Heart Rate Service* (0x180D).

El resto de atributos se inicializan de forma similar. Algunos atributos también
tienen activa la propiedad *NOTIFY*, que se establece vía 
``&char_prop_notify``. La tabla completa se inicializa como sigue:

```c
/// Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t heart_rate_gatt_db[HRS_IDX_NB] =
{
    // Heart Rate Service Declaration
    [HRS_IDX_SVC]                       =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(heart_rate_svc), (uint8_t *)&heart_rate_svc}},

    // Heart Rate Measurement Characteristic Declaration
    [HRS_IDX_HR_MEAS_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_notify}},

    // Heart Rate Measurement Characteristic Value
    [HRS_IDX_HR_MEAS_VAL]               =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&heart_rate_meas_uuid, ESP_GATT_PERM_READ,
      HRPS_HT_MEAS_MAX_LEN,0, NULL}},

    // Heart Rate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HRS_IDX_HR_MEAS_NTF_CFG]           =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),sizeof(heart_measurement_ccc), (uint8_t *)heart_measurement_ccc}},

    // Body Sensor Location Characteristic Declaration
    [HRS_IDX_BOBY_SENSOR_LOC_CHAR]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    // Body Sensor Location Characteristic Value
    [HRS_IDX_BOBY_SENSOR_LOC_VAL]   =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&body_sensor_location_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(body_sensor_loc_val), (uint8_t *)body_sensor_loc_val}},

    // Heart Rate Control Point Characteristic Declaration
    [HRS_IDX_HR_CTNL_PT_CHAR]          =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    // Heart Rate Control Point Characteristic Value
    [HRS_IDX_HR_CTNL_PT_VAL]             =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&heart_rate_ctrl_point, ESP_GATT_PERM_WRITE|ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(heart_ctrl_point), (uint8_t *)heart_ctrl_point}},
};
```

## Inicialización del servicio

Cuando la tabla se crea, se emite un evento de tipo ``ESP_GATTS_CREAT_ATTR_TAB_EVT``. 
Este evento tiene los siguientes parámetros asociados:

```c
esp_gatt_status_t status;    /*!< Operation status */
esp_bt_uuid_t svc_uuid;      /*!< Service uuid type */
uint16_t num_handle;         /*!< The number of the attribute handle to be added to the gatts database */
uint16_t *handles;           /*!< The number to the handles */
```

Este ejemplo utiliza este evento para mostrar información y comprobar que el 
tamaño de la tabla creada es igual al número de elementos en la enumeración
`HRS_IDX_NB`. Si la tabla se creó correctamente, los manejadores de atributos se copian
en la tabla de manejadores `heart_rate_handle_table` y el servicio se inicicaliza
utilizando la función ``esp_ble_gatts_start_service()``:

```c
case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
        ESP_LOGI(GATTS_TABLE_TAG, "The number handle =%x\n",param->add_attr_tab.num_handle);
        if (param->add_attr_tab.status != ESP_GATT_OK){
            ESP_LOGE(GATTS_TABLE_TAG, "Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
            ESP_LOGE(GATTS_TABLE_TAG, "Create attribute table abnormally, num_handle (%d) \
                    doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
        }
        else {
            memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
            esp_ble_gatts_start_service(heart_rate_handle_table[HRS_IDX_SVC]);
        }
        break;
```

Los manejadores almacenados son números que identifican cada atributo. Estos manejadores
pueden usarse para determinar qué característica está siendo leída o escrita,
y por tanto pueden ser proporcionados a otros puntos de la aplicación para manejar
distintas acciones.

Finalmente, la tabla `heart_rate_handle_table` contiene el Perfil de Aplicación
en forma de estructura con información sobre los parámetros de los atributos y 
la interfaz GATT, ID de conexión, permisos e ID de aplicación. La estructura
presenta los siguientes campos (no todos se usan en el ejemplo):

```c
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};
```

