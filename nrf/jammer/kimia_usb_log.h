#ifndef KIMIA_USB_LOG__H__
#define KIMIA_USB_LOG__H__

#include "stdbool.h"
#include "string.h"


#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION false
#endif

#define READ_SIZE 1
#define WRITE_SIZE 1000

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1



/**
* To makro działa dokładnie jak printf, tyle że printuje na USB
* 
*/
#define KIMIA_USB_PRINT( ...)  \
    kimia_usb_log_write(kimia_usb_log_get_txbuf(),snprintf(kimia_usb_log_get_txbuf(),WRITE_SIZE, __VA_ARGS__) )




/**
* Inicjalizacja biblioteki
*/
void kimia_usb_log_init();

/**
* Funkcja zajmująca się obsługą procesu USB, pownna być dodana w funkcji idle_state_handle by wszystko działało
*
* @ret false jeśli nie ma żadnych eventów
*/
bool kimia_usb_log_process();

/**
* Zwraca statyczny bufor tx ukryty w pliku .c
*/
char* kimia_usb_log_get_txbuf();

/**
* Funkcja pomocnicza dla makra KIMIA_USB_PRINT
* Używanie jej poza makrem będzie raczej uciążliwe
*/
void kimia_usb_log_write(char* tx,size_t size);

/*
$(SDKDIR)/components/libraries/usbd
app_usbd.c
app_usbd_core.c
app_usbd_string_desc.c
app_usbd_serial_num.c

$(SDKDIR)/components/libraries/usbd/class/cdc/acm
app_usbd_cdc_acm.c


$(SDKDIR)/modules/nrfx/drivers/src
nrfx_usbd.c
nrfx_systick.c
nrfx_power.c

$(SDKDIR)/integration/nrfx/legacy
nrf_drv_power.c
*/

#endif