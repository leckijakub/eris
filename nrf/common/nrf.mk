build:
	$(MAKE) -C pca10059/s140/armgcc

flash: build
	$(info $(board_id))
	sshpass -p $(remote_pass) scp -P 120$(board_id) pca10059/s140/armgcc/_build/nrf52840_xxaa.hex $(sdk_path)/components/softdevice/s140/hex/s140_nrf52_7.0.1_softdevice.hex $(remote_user)@153.19.49.102:/var/tmp
	sshpass -p $(remote_pass) ssh -p 120$(board_id) $(remote_user)@153.19.49.102 ' \
		   curl http://153.19.49.102:3001/resetNRF52v2/$(board_id) \
		&& nrfutil pkg generate --hw-version 52 --sd-req 0x00 --sd-id 0xCA --softdevice /var/tmp/s140_nrf52_7.0.1_softdevice.hex  --debug-mode --application /var/tmp/nrf52840_xxaa.hex /var/tmp/dfu.zip \
		&& nrfutil dfu usb-serial -pkg /var/tmp/dfu.zip -p $(usb_path) -b 115200 \
		&& rm /var/tmp/*.hex'

log:
	sshpass -p $(remote_pass) ssh -p 120$(board_id) $(remote_user)@153.19.49.102 -t 'minicom -D $(usb_path)'

reset:
	curl http://153.19.49.102:3001/resetNRF52v2/$(board_id)

clean:
	$(MAKE) clean -C pca10059/s140/armgcc
