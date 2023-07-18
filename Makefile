build_espar:
	$(MAKE) -C pca10056/armgcc

build_dongle:
	$(MAKE) -C pca10059/armgcc
	nrfutil pkg generate --hw-version 52 \
		--sd-req 0x0 \
		--debug-mode \
		--application pca10059/armgcc/_build/nrf52840_xxaa.hex \
		pca10059/armgcc/_build/dfu.zip

clean_espar:
	$(MAKE) clean -C pca10056/armgcc

clean_dongle:
	$(MAKE) clean -C pca10059/armgcc

clean: clean_espar clean_dongle
