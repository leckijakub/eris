# ESPAR DUT

DUT - Device Under Testing

ESPAR DUT device can act in one of four modes:

* RX    - scan for advertising packets and report RSSI.
* TX    - send adv packets.
* JAM   - emits a continuous wave on a given channel to jam adv packets.
* Idle  - no action.

## Building

To build the project two options can be followed:

### Open project in devcontainer - suggested for development

* Use vscode option to reopen the project in devcontainer
* In the devcontainer run:

```bash
make build
```

* The `dfu.zip` will be created in the workspace root dir.

### Run VScode task - suggested for fast testing

* Run vscode configured build task (`ctrl+shift+B`)
* The `dfu.zip` will be created in the workspace root dir.

## Flashing

To flash the device run

```bash
make flash
```

## Control

To control DUT use `dut_ctrl.py`

For usage instructions run:

```bash
./dut_ctrl.py --help
```


## Examples

resetting and flashing a dongle
```
./dut_ctrl.py /dev/ttyACM1 reset && ~/.nrfutil/bin/nrfutil dfu usb-serial -pkg pca10059/armgcc/_build/dfu.zip -p /dev/ttyACM1 -b 115200
```



ESPAR board programming:
```
nrfjprog --family NRF52 --recover
nrfjprog --family NRF52 --program pca10056/armgcc/_build/nrf52840_xxaa.hex
nrfjprog --family NRF52 --reset
```
