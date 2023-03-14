ISGIT := $(shell git ls-files 1>/dev/null 2>&1 ; echo $$?)

ifeq ($(ISGIT),0)
	VER="$(shell git describe --abbrev --dirty --always --tags)"
else
	VER="UNKNOWN"
endif

%.sh: %.sh.in
	sed -e "s/__VERSION__/\"${VER}\"/" $^ >$@
	chmod 755 $@

%.sh.d: %.sh.in
	@echo "$(shell echo $@ | sed -e 's/.d//') : $^" >$@

SCRIPTS_IN := $(shell find . -name '*.sh.in')
TARGETS = $(SCRIPTS_IN:.sh.in=.sh)
deps = $(SCRIPTS_IN:.sh.in=.sh.d)

all: $(TARGETS)

clean:
	@rm -f *~ $(TARGETS) *.d *.key *.pub *.csr *.crt *.dgst

-include $(deps)
