# ESPAR Driver
## Introduction
This repository provides a driver for ESPAR antenna.

Two variants of ESPAR antenna are covered:
* Standard 2.4 GHz ESPAR
* Standard 2.4 GHz ESPAR version 2
* ESPAR with two rows of passive elements

## Supported MCUs
So far, the driver has interface for Nordic nRF52 chip.

## Usage
### Adding git submodule to project
To add this driver to your repository, all that needs to be done is:
```shell
git submodule add https://gitlab.mw.eti.pg.gda.pl/wsn/driver.espar
```

### Adding driver files in Segger Embedded Studio
After adding the submodule, it is neccessary to point Segger Embedded Studio the driver files. It can be done by editing the *.emProject file and adding:

```
<folder Name="driver.espar"
        exclude="espar_driver_interface_jn5168.h"
        filter="*.c;*.h"
        path="../../../driver.espar"
        recurse="Yes" />
```

### Choosing ESPAR type
**_Note:_** Remember to define espar type by **defining (only!) one of three**:
```C
#define ESPAR_STANDARD_V1 1
#define ESPAR_STANDARD_V2 0
#define ESPAR_DUAL_PASSIVE 0
```
This can also be done as preprocessor macro.

If user will not define ESPAR type, ESPAR_STANDARD will be assumed.

### Choosing characteristics (3, 5 or 8 directors)
**_Note:_** Remember to choose which characteristics to useby **defining one, two or all three**:
```C
#define USE_3DIR_CHARACTERISTICS 0
#define USE_5DIR_CHARACTERISTICS 1
#define USE_8DIR_CHARACTERISTICS 0
```
This can also be done as preprocessor macro.


### Using driver in code

```C
#include "espar_driver.h"
```


## Adding chip support
It is possible to add new chip support, simply by creating an interface files (.c and .h) and filling required functions. It may be helpful to base new interface file on nrf52 interface file.
