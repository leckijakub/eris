master := master
client := client
jammer := jammer
agents := $(master) $(client) $(jammer)

master_board_id := 07
client_board_id := 08
jammer_board_id := 09

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))
export sdk_path := $(current_dir)/../../
export remote_user := root
export remote_pass := root
export usb_path := /dev/ttyACM0

clean_prefix := clean_
agents_clean := $(addprefix $(clean_prefix),$(agents))
clean_project = ${@:$(clean_prefix)%=%}

flash_prefix := flash_
agents_flash := $(addprefix $(flash_prefix),$(agents))
flash_project = ${@:$(flash_prefix)%=%}

log_prefix := log_
agents_log := $(addprefix $(log_prefix),$(agents))
log_project = ${@:$(log_prefix)%=%}


#### BUILD ####
.PHONY: all $(agents)
all: $(agents)

$(agents):
	$(MAKE) -C $@


#### FLASH ####
flash: $(agents_flash)

$(agents_flash):
	$(MAKE) flash -C $(flash_project) board_id=$(value $(flash_project)_board_id)


#### LOG ####
$(agents_log):
	$(MAKE) log -C $(log_project) board_id=$(value $(log_project)_board_id)


#### CLEAN ####
.PHONY: clean $(agents_clean)
clean: $(agents_clean)

$(agents_clean):
	$(MAKE) clean -C $(clean_project)
