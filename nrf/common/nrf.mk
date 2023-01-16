mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))

sd_version ?= s140
sd_hex_file ?= s140_nrf52_7.0.1_softdevice.hex
sd_req ?= 0x0
sd_id ?= 0xCA

sdk_path ?= $(current_dir)/../../..
remote_user ?= root
remote_pass ?= root
usb_path ?= /dev/ttyACM1

build:
	$(MAKE) -C pca10059/$(sd_version)/armgcc

flash: build
	$(info $(board_id))
	sshpass -p $(remote_pass) scp -P 120$(board_id) pca10059/$(sd_version)/armgcc/_build/nrf52840_xxaa.hex $(sdk_path)/components/softdevice/$(sd_version)/hex/$(sd_hex_file) $(remote_user)@153.19.49.102:/var/tmp
	sshpass -p $(remote_pass) ssh -p 120$(board_id) $(remote_user)@153.19.49.102 ' \
		   curl http://153.19.49.102:3001/resetNRF52v2/$(board_id) \
		&& nrfutil pkg generate --hw-version 52 --sd-req $(sd_req) --sd-id $(sd_id) --softdevice /var/tmp/$(sd_hex_file)  --debug-mode --application /var/tmp/nrf52840_xxaa.hex /var/tmp/dfu.zip \
		&& nrfutil dfu usb-serial -pkg /var/tmp/dfu.zip -p $(usb_path) -b 115200 \
		&& rm /var/tmp/*.hex'

log:
	sshpass -p $(remote_pass) ssh -p 120$(board_id) $(remote_user)@153.19.49.102 -t 'minicom -D $(usb_path)'

logs:

reset:
	curl http://153.19.49.102:3001/resetNRF52v2/$(board_id)

clean:
	$(MAKE) clean -C pca10059/$(sd_version)/armgcc
