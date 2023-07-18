# ERIS - ESPAR Reactive Isolation System

ERIS is a PoC system used to research ESPAR (Electronically Steerable Parasitic
Array Radiator) capabilities in field of isolating the medium between a
transmitter and a receiver communicating on 2.4 GHz frequencies.

## Table of Contents

- [ERIS - ESPAR Reactive Isolation System](#eris---espar-reactive-isolation-system)
  - [Table of Contents](#table-of-contents)
  - [Architecture](#architecture)
    - [Components](#components)
    - [General architecture](#general-architecture)
    - [Beacon connection architecture](#beacon-connection-architecture)
  - [Software](#software)
    - [Building](#building)
      - [Open project in devcontainer - suggested for development](#open-project-in-devcontainer---suggested-for-development)
      - [Run VScode task - suggested for fast testing](#run-vscode-task---suggested-for-fast-testing)
    - [Flashing](#flashing)
      - [Beacon](#beacon)
      - [ESPAR board](#espar-board)
    - [Control](#control)

## Architecture

### Components

1. ESPAR - Receiver
2. Beacons - Transmitter/Jammers
3. RPi4 - System control unit

### General architecture

![eris-arch](assets/eris-arch_simple.drawio.svg)

### Beacon connection architecture

![eris-beacon-arch](assets/eris-arch_beacon-connection.drawio.svg)

## Software

ERIS software is compatible with two platforms:

1. NRF52840 dongle - used as beacon
2. Custom NRF52840 board - designed for ESPAR integration

ERIS device can act in one of four modes:

| Mode | Description                                                         | Agent                       |
| ---- | ------------------------------------------------------------------- | --------------------------- |
| RX   | listen for packets                                                  | ESPAR platform (master)     |
| TX   | send packets with incrementing 32bit value                          | transmitter beacon (client) |
| JAM  | emit a continuous wave on a given channel to jam transmited packets | jamming beacon (jammer)     |
| Idle | standby mode                                                        |                             |

### Building

Project can be built in one of two ways:

#### Open project in devcontainer - suggested for development

1. Use vscode option to reopen the project in devcontainer
2. Build espar software

   ```bash
   make build_espar
   ```

   ESPAR platform software can be found in
   `pca10056/armgcc/_build/nrf52840_xxaa.hex`

3. Build beacon software

   ```bash
   make build_dongle
   ```

    NRF52840 dongle software can be found in `pca10059/armgcc/_build/dfu.zip`.

#### Run VScode task - suggested for fast testing

- Run vscode configured build task (`ctrl+shift+B`)

### Flashing

#### Beacon

```bash
./scripts/dut_ctrl.py /dev/ttyACM1 reset && ~/.nrfutil/bin/nrfutil dfu usb-serial -pkg pca10059/armgcc/_build/dfu.zip -p /dev/ttyACM1 -b 115200
```

#### ESPAR board

```bash
nrfjprog --family NRF52 --recover && nrfjprog --family NRF52 --program pca10056/armgcc/_build/nrf52840_xxaa.hex && nrfjprog --family NRF52 --reset
```

### Control

ERIS boards can be controlled through USB Serial (`/dev/ACM<id>`) connection using
`scripts/dut_ctrl.py` script.

For usage instructions run:

```bash
./scripts/dut_ctrl.py --help
```
