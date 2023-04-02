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




