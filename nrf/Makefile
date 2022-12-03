mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))
include $(current_dir)/common/utils.mk

# Agents definitions
master := master
client := client
jammer := jammer
agents := $(master) $(client) $(jammer)

# Remote boards IDs
master_board_id := 06
client_board_id := 08
jammer_board_id := 03

export sdk_path := $(current_dir)/../../
export remote_user := root
export remote_pass := root
export usb_path := /dev/ttyACM0

# Common actions definitions
actions := build clean flash log reset
agents_actions := $(foreach action_prefix, $(actions), $(addprefix $(action_prefix)_, $(agents)))
action = $(shell echo $@ | cut -d'_' -f 1)
action_target = $(shell echo $@ | cut -d'_' -f 2-)

# Generate global actions to run on all agents global actions depends on all agents
define main_action_target
$(1): $(addprefix $(1)_, $(agents))
endef
$(foreach main_action, $(actions), $(eval $(call main_action_target,$(main_action))))

# Actions for separate agents
$(agents_actions):
	$(call pretty-info,Running target $@)
	$(MAKE) $(action) -C $(action_target) board_id=$(value $(action_target)_board_id)

log:
	$(MAKE) log -C master board_id=$(master_board_id)
