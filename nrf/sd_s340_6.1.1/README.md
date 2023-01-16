# Softdevice S340 6.6.1

## Description

Softdevice S340 contains implementation for both ANT and BLE stacks. We are
interested mostly in ANT stack as it allows to generate a continous wave that
can be ussed to jam given channel. We will be using it to jam advertisement
channel 37 (2402MHz)

The S340 firmware is not mainained directly by 'NRF' but by 'Thisisant' and must
be downloaded separately.


## Download

* Register on https://www.thisisant.com/ - account activation can take up do one
  business day.
* Download softdevice S340 in version **6.6.1** from https://www.thisisant.com/developer/components/nrf52832/
* Unpack it to this directory so it has a following structure
  ```
  nrf
  |-- client
  |-- common
  |-- jammer
  |-- master
  `-- sd_s340_6.1.1
      |-- ANT_s340_nrf52840_6.1.1.API
      |-- ANT_s340_nrf52840_6.1.1.hex
      |-- ANT_s340_nrf52840_6.1.1_readme.txt
      |-- ANT_s340_nrf52840_6.1.1_releasenotes.pdf
      |-- Note.txt
      |-- README.md
      `-- SoftDevSW\ Agrmt\ S332\ S212\ v3_1.pdf
  ```

After that this part is finished.
