bold := \033[1m
normal := \033[0m
blue := \033[34m

define pretty-info
@printf '$(bold)$(blue)$1$(normal)\n'
endef
