/* A simple BLE scanner example - prints out raw data from scanned advertisements through USB CDC ACM */

#include <stdio.h>

#include "ble_hci.h"
#include "nrf.h"
#include "nrf_ble_scan.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_usbd.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

//#include "app_uart.h"
#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_core.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"
#include "app_util.h"

#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3

#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3

// A macro for defining a nrf_ble_scan instance, with m_scan as the name of the instance
NRF_BLE_SCAN_DEF(m_scan);

// USB DEFINES START
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst,
    app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE 0
#define CDC_ACM_COMM_EPIN NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE 1
#define CDC_ACM_DATA_EPIN NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT NRF_DRV_USBD_EPOUT1

// TODO real length??
#define CDC_DATA_ARRAY_LEN ((255 * 2) + 1)
static char m_cdc_data_array[CDC_DATA_ARRAY_LEN];

/** @brief CDC_ACM class instance */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
    cdc_acm_user_ev_handler,
    CDC_ACM_COMM_INTERFACE,
    CDC_ACM_DATA_INTERFACE,
    CDC_ACM_COMM_EPIN,
    CDC_ACM_DATA_EPIN,
    CDC_ACM_DATA_EPOUT,
    APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

// END OF USB DEFINES

void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
  app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

static void scan_start(void) {

  /* nrf_ble_scan_start: part of the BLE library Scanning Module
** takes  a pointer to a scanning module instance
** can take a pointer to GAP scanning parameters, possibly NULL
*/
  ret_code_t err_code = nrf_ble_scan_start(&m_scan);
  APP_ERROR_CHECK(err_code);
  NRF_LOG_DEBUG("scan_start SUCCESSFUL");
}

static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
  ret_code_t err_code;
  char *p = &m_cdc_data_array[0];

  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_ADV_REPORT: {
    //NRF_LOG_RAW_HEXDUMP_INFO(m_scan.scan_buffer.p_data, m_scan.scan_buffer.len);
    //NRF_LOG_INFO("%d # # # # # # # # # # # # # # # \r\n", m_scan.scan_buffer.len);

    for (int i = 0; i < m_scan.scan_buffer.len && i < CDC_DATA_ARRAY_LEN; i++) {
      p += sprintf(p, "%02x", m_scan.scan_buffer.p_data[i]);
    }
    //NRF_LOG_INFO(":%s:", m_cdc_data_array);

    err_code = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_cdc_data_array, CDC_DATA_ARRAY_LEN);
    if (err_code != NRF_SUCCESS) {
      NRF_LOG_INFO("CDC ACM unavailable, data received: %s", m_cdc_data_array);
    }
  } break;
  default:
    break;
  }
}

static void ble_stack_init(void) {
  ret_code_t err_code;

  /* nrf_sdh_enable_request: part of SDK Common Libraries, SoftDevice Handler
** Function to request enabling of SoftDevice by issuing an NRF_SDH_EVT_ENABLE_REQUEST
** to all observers registered using the NRF_SDH_REQUEST_OBSERVER macro.  If all observers
** acknowledge the request, the SoftDevice is enabled
*/
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  /* nrf_sdh_ble_default_cfg_set: BLET Support in SoftDevice Handler
** Sets the default BLE stack configuration, configuring 
** number of peripheral links
** number of central links
** ATT MTU size
** Vendor specific UUID count
** GATTS Attribute table size
** Service changed
*/
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  /* nrf_sdh_ble_enable: function for retrieving the address of the start of the application's RAM */
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);

  /* NRF_SDH_BLE_OBSERVER: BLE Support for SoftDevice
** Macro for registering a nrf_sdh_ble_evet_observer_t modules to be notified about SoC events.
** arguments are  observer name, priority of the observer event handler, BLE event handler, and parameter to handler
*/
  NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
  NRF_LOG_DEBUG("ble_stack_init SUCCESSFUL");
}

static void log_init(void) {

  /* NRF_LOG_INIT: Logger Module
** Macro for initializing the logs.
** first parameter is the timestamp function (nrf_log_timestamp_func_t
** second parameter is optional timestamp frequency in Hz; otherwise default is used
*/
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  /* NRF_LOG_DEFAULT_BACKENDS_INIT - Logger Module
** Macro for initializingg default backends
*/
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  NRF_LOG_DEBUG("log_init SUCCESSFUL");
}

static void scan_init(void) {
  ret_code_t err_code;

  /* nrf_ble_scan_init: Scanning Module function to initiliaze the scanning module
** takes pointer to nrf_scan_ble_t used to identify instance
** point of parameters to initialize module, may be null
** handler for scanning events - can be NULL if no handling is implemented
*/
  err_code = nrf_ble_scan_init(&m_scan, NULL, NULL);
  APP_ERROR_CHECK(err_code);
  NRF_LOG_DEBUG("scan_init SUCCESSFUL");
}

static void idle_state_handle(void) {

  /* NRF_LOG_FLUSH - Macro for flushing log data before reset */
  NRF_LOG_FLUSH();
  /* nrf_pwr_mgmt_run: Function for running power management; should be run in main loop */
  nrf_pwr_mgmt_run();
}

// USB CODE START
static bool m_usb_connected = false;

/** @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst,
    app_usbd_cdc_acm_user_event_t event) {
  app_usbd_cdc_acm_t const *p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

  switch (event) {
  case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    break;
  case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
    break;
  case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    break;
  case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    break;
  default:
    break;
  }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event) {
  switch (event) {
  case APP_USBD_EVT_DRV_SUSPEND:
    break;

  case APP_USBD_EVT_DRV_RESUME:
    break;

  case APP_USBD_EVT_STARTED:
    break;

  case APP_USBD_EVT_STOPPED:
    app_usbd_disable();
    break;

  case APP_USBD_EVT_POWER_DETECTED:
    NRF_LOG_DEBUG("USB power detected");

    if (!nrf_drv_usbd_is_enabled()) {
      app_usbd_enable();
    }
    break;

  case APP_USBD_EVT_POWER_REMOVED: {
    NRF_LOG_DEBUG("USB power removed");
    m_usb_connected = false;
    app_usbd_stop();
  } break;

  case APP_USBD_EVT_POWER_READY: {
    NRF_LOG_DEBUG("USB ready");
    m_usb_connected = true;
    app_usbd_start();
  } break;

  default:
    break;
  }
}

int main(void) {

  ret_code_t ret;
  static const app_usbd_config_t usbd_config = {
      .ev_state_proc = usbd_user_ev_handler};

  log_init();
  NRF_LOG_INFO("USBD Central Example started.");
  //power_management_init();

  app_usbd_serial_num_generate();
  NRF_LOG_DEBUG("app_usbd_serial_num_generate");

  ret = nrf_drv_clock_init();
  APP_ERROR_CHECK(ret);
  NRF_LOG_DEBUG("nrf_drv_clock_init");

  ret = app_usbd_init(&usbd_config);
  APP_ERROR_CHECK(ret);
  NRF_LOG_DEBUG("app_usbd_init");

  app_usbd_class_inst_t const *class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
  ret = app_usbd_class_append(class_cdc_acm);
  APP_ERROR_CHECK(ret);
  NRF_LOG_DEBUG("app_usbd_class_append");

  ble_stack_init();
  NRF_LOG_INFO("ble_stack_init");

  scan_init();
  NRF_LOG_INFO("scan_init");

  scan_start();

  ret = app_usbd_power_events_enable();
  APP_ERROR_CHECK(ret);

  NRF_LOG_FLUSH();
  for (;;) {
    while (app_usbd_event_queue_process()) {
    }
    idle_state_handle();
  }
}