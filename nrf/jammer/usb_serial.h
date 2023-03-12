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
#define USB_SER_PRINT( ...)  \
    usb_ser_write(usb_ser_log_get_txbuf(),snprintf(usb_ser_log_get_txbuf(),WRITE_SIZE, __VA_ARGS__) )




/**
* Inicjalizacja biblioteki
*/
void usb_ser_init();

void usb_ser_enable();

/**
* Funkcja zajmująca się obsługą procesu USB, pownna być dodana w funkcji idle_state_handle by wszystko działało
*
* @ret false jeśli nie ma żadnych eventów
*/
bool usb_ser_events_process();

/**
* Zwraca statyczny bufor tx ukryty w pliku .c
*/
char* usb_ser_log_get_txbuf();

/**
* Funkcja pomocnicza dla makra USB_SER_PRINT
* Używanie jej poza makrem będzie raczej uciążliwe
*/
void kimia_usb_log_write(char* tx,size_t size);
void usb_ser_write(char* buf, size_t size);


#endif
