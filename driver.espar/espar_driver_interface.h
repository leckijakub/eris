#ifndef _ESPAR_DRIVER_INTERFACE_
#define _ESPAR_DRIVER_INTERFACE_

#if defined(NRF52840_XXAA)
#include "espar_driver_interface_nrf52.h"
#elif defined(JN5168)
#include "espar_driver_interface_jn5168.h"
#else
#error "No chip defined! (e.g. NRF52840_XXAA, JN5168)"
#endif

#if !defined(ESPAR_STANDARD_V1) && !defined(ESPAR_STANDARD_V2) && !defined(ESPAR_DUAL_PASSIVE)
#error "ESPAR type not defined! (e.g. ESPAR_STANDARD_V1, ESPAR_STANDARD_V2, ESPAR_DUAL_PASSIVE)"
#endif

#if (!ESPAR_STANDARD_V1) & (!ESPAR_STANDARD_V2) & (!ESPAR_DUAL_PASSIVE)
#error "ESPAR type not selected (e.g. #define ESPAR STANDARD 1)"
#endif

#if (!ESPAR_STANDARD_V1) ^ (!ESPAR_STANDARD_V2) ^ (!ESPAR_DUAL_PASSIVE)
#error "Too many ESPAR types selected!"
#endif

#endif /* _ESPAR_DRIVER_INTERFACE_ */