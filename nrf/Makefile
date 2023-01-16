mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))
include $(current_dir)/common/utils.mk

# Agents definitions
master := master
client := client
jammer := jammer
agents := $(master) $(client) $(jammer)

# Remote boards IDs
master_board_id := 02
client_board_id := 04
jammer_board_id := 03

# Common actions definitions
actions := build clean flash log reset
no_global_actions := log
global_actions := $(filter-out $(no_global_actions), $(actions))
agents_actions := $(foreach action_prefix, $(actions), $(addprefix $(action_prefix)_, $(agents)))
action = $(shell echo $@ | cut -d'_' -f 1)
action_target = $(shell echo $@ | cut -d'_' -f 2-)

# Generate global actions to run on all agents global actions depends on all agents
define global_action_target
$(1): $(addprefix $(1)_, $(agents))
endef
$(foreach global_action, $(global_actions), $(eval $(call global_action_target,$(global_action))))

# Actions for separate agents
$(agents_actions):
	$(call pretty-info,Running target $@)
	$(MAKE) $(action) -C $(action_target) board_id=$(value $(action_target)_board_id)

# Mainly we are interested in Master logs, thus make it default for target `log`
log:
	$(MAKE) log -C master board_id=$(master_board_id)
