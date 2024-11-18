#
# Copyright (c) Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Authors: sudhir.gopal@intel.com
#
# Makefile for Yocto Build

TOP                     ?= $(shell pwd)
SHELL                    = /bin/bash
BLDDIR                  ?= $(TOP)/srp-build
BB_IMAGE_TYPE           ?= srp-image-run
BB_MACHINE_NAME         ?= srpeh
BB_NO_NETWORK           ?= 0
BB_NUMBER_THREADS       ?= "24"
DL_DIR                  ?=
INCLUDE_SIMICSFS        ?= true
LINUX_VERSION           ?= 6.6
PARALLEL_MAKE           := "-j $(BB_NUMBER_THREADS)"
META_SRP_REL            ?= hwc_del_2
SIMICS_FILE             ?= $(word 1,$(wildcard $(TOP)/simics*))

PREFERRED_PROVIDER_virtual/kernel   := "linux-yocto"

ifeq (,$(filter $(LINUX_VERSION),6.6))
  $(error LINUX_VERSION is not set correctly, run 'make help' for usage)
endif

META_SRP_DEPENDENCY                 ?= $(TOP)/meta-intel-srp/DEPENDENCIES
DISTRO                              := "intel-srp"
RUNTARGET                           := "qsp"

SRP_REPO_NAME                       := meta-intel-srp
UPDATES_REPO_NAME                   := meta-intel-srp-updates

# Define V=1 to echo everything
V ?= 1
ifneq ($(V),1)
        Q=@
endif

RM = $(Q)rm -f

META_SRP_URL     ?= https://github.com/axxia/meta-intel-srp.git
LAYERS           += $(TOP)/meta-intel-srp

POKY_URL          = https://git.yoctoproject.org/poky
LAYERS           += $(TOP)/poky

OE_URL            = https://github.com/openembedded/meta-openembedded.git
LAYERS           += $(TOP)/meta-openembedded

define source-bb-env
    source $(TOP)/meta-intel-srp/srp-env                         ; \
    source $(TOP)/poky/oe-init-build-env $(BLDDIR)
endef

define bitbake
	set -e ; \
	$(call source-bb-env) ; \
	echo BLDDIR=$(BLDDIR) ; \
	cd $(BLDDIR) ; \
	bitbake -c cleanall $(1) ; \
	bitbake $(1)
endef

define bitbake-task
	set -e ; \
	$(call source-bb-env) ; \
	cd $(BLDDIR) ; \
	bitbake $(1) -c $(2)
endef

define bitbake-cleansstate
	set -e ; \
	$(call source-bb-env) ; \
	cd $(BLDDIR) ; \
	CLEAN_TARGETS="virtual/kernel $(1)" ; \
	if [[ "$(INCLUDE_SIMICSFS)" = "true" ]]; then CLEAN_TARGETS="$$CLEAN_TARGETS simicsfs-client" ; fi ; \
	bitbake -c cleansstate $$CLEAN_TARGETS
endef

define populate
	echo "$(1)";
	set -e ; \
	if [ ! -d "$(1)" ]; then \
	    echo "Cloning Repo $(2)" ; \
	    git clone $(2) $(1); \
	else \
	    echo "Pull on Repo $(2)" ; \
	    cd $(1); \
            git clean -dfx; \
	    git fetch --all -f ; \
	fi;
endef

#
# Default to main if branch not available.
#
define checkout_rev_main
	set -e ; \
	(git -C $(1) show-branch origin/$(2) &>/dev/null) && \
		(git -C $(1) checkout $(2)) || \
		(git -C $(1) checkout main)
endef

define checkout_rev
	set -e ; \
	git -C $(1) checkout $(2)
endef

define check-file-exists
        echo "Checking for file: $(1)"; \
	if [ ! -f $(1) ]; then \
		echo "Error: $(1) is not present"; \
		exit 1; \
	fi
endef

define checkout_layer_from_file
	$(call check-file-exists, $(META_SRP_DEPENDENCY))
	set -e ; \
	BASE_LAYER=`basename $(1)`;\
	REV=`awk -v layer=$$BASE_LAYER '{\
	if($$0 ~ "^Dependencies" ) start_parse=1;\
	if($$0 ~ "^Back Ported" ) start_parse=0;\
	if(start_parse == 1 && $$0 ~ "URI" && $$0 ~ layer) parse_layer=1;\
	if(parse_layer == 1 && $$0 ~ "^revision") { print $$2; parse_layer=0; }\
	}' $(META_SRP_DEPENDENCY)`;\
	if [ -z "$$REV" ]; then echo "ERROR: Checkout REV of $$BASE_LAYER could not be read"; exit 1; fi; \
        echo "REPO: $$BASE_LAYER, REV: $$REV" ; \
        git -C $(1) checkout $$REV
endef

help:
	@echo "USAGE:"; \
        echo  "------" ; \
	echo ; \
	echo "$$ make fs META_SRP_REL=<tag-name>"; \
	echo "    Pull and Build Yocto Linux"; \
	echo "$$ make sdk install-sdk META_SRP_REL=<tag-name> SDK_INSTALL_DIR=<sdk-install-dir>"; \
	echo "    Pull and Build Yocto Linux and then build and install sdk"; \
	echo "$$ make craff_gen META_SRP_REL=<tag-name>" ; \
	echo "    Create yocto.craff image from .wic after building sdk\n"; \
	echo ; \
        echo "Parameters" ; \
        echo "----------" ; \
	echo "META_SRP_REL=<tag-name>: Optional:" ; \
        echo "    Tag-name in META-SRP repo" ; \
	echo "    Defaults to current release"; \
	echo "META_SRP_URL=<URL-for-meta-intel-srp>: Optional:"; \
	echo "    Defaults to https://github.com/axxia/meta-intel-srp.git"; \
	echo "META_SRP_DEPEDENCY=<File which contains commit ids of meta layers>: Optional:"; \
	echo "    Defaults to <current-dir>/meta-intel-srp/DEPENDENCIES.distro"; \
	echo "INCLUDE_SIMICSFS: Optional: Default = true:" ; \
	echo "    When set to 'true', builds with simics,"; \
	echo "    Set to 'false' builds Linux without simics"; \
	echo "SIMICS_FILE=<Location-of-Simics-archive>: Required"; \
	echo "    Default: <current-dir>/simics-*" ; \
	echo "BLDDIR=<build dir>: Optional" ; \
	echo "    Location of the build"; \
	echo "    Defaults to <current-dir>/srp-build" ; \
	echo "BB_IMAGE_TYPE=<image name>: Optional"; \
	echo "    The type of image to build"; \
	echo "    Default: srp-image-run" ; \
	echo "SDK_INSTALL_DIR=<sdk-install-location>"; \
	echo "    Required when 'install-sdk' is called"; \
	echo "LINUX_VERSION=<version>: Optional:"; \
	echo "    Default: 6.6" ; \
	echo "    Supported versions : 6.6"; \
	echo "INCLUDE_UPDATES: Optional: Default = true" ; \
	echo "    When set to 'true', will include meta-intel-srp-updates Yocto"; \
	echo "    layer with minimal changes to fix older Intel Axxia relases"; \
	echo "BB_NUMBER_THREADS=<num_threads>: Optional:"; \
	echo "    Number of parallel threads to run"; \
	echo "    Defaults to '24'" ; \
	echo "DL_DIR=<download-directory>: Optional:"; \
	echo "    A pre-populated shared download directory used for"; \
	echo "    build time/space saving"; \
	echo "    Defaults to $(BLDDIR)/downloads" ; \

all: fs

.PHONY: $(LAYERS)

$(TOP)/meta-intel-srp:
	$(call  populate,$@,$(META_SRP_URL))
	$(call  checkout_rev,$@,$(META_SRP_REL))

$(TOP)/meta-openembedded: $(TOP)/meta-intel-srp
	$(call  populate,$@,$(OE_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/poky: $(TOP)/meta-intel-srp
	$(call  populate,$@,$(POKY_URL))
	$(call  checkout_layer_from_file,$@)


# create bitbake build
.PHONY: build
build: get-simics $(LAYERS)
		set -e ; \
                echo "Generating local.conf in $(BLDDIR)" ; \
		cd $(TOP); \
		rm -rf $(BLDDIR)/conf ; \
		$(call source-bb-env) ; \
		echo "PWD = $$PWD" ; \
		sed -i s/^MACHINE[[:space:]]*=.*/MACHINE\ =\ \"$(BB_MACHINE_NAME)\"/g conf/local.conf ; \
		sed -i s/^DISTRO[[:space:]]*=.*/DISTRO\ =\ \"$(DISTRO)\"/g conf/local.conf ; \
		sed -i s/^MACHINE.*/MACHINE\ =\ \"$(BB_MACHINE_NAME)\"/g conf/local.conf; \
		sed -i s/^RUNTARGET.*/RUNTARGET\ =\ \"$(RUNTARGET)\"/g conf/local.conf ; \
		sed -i s/^BB_NUMBER_THREADS.*/BB_NUMBER_THREADS\ =\ \"$(BB_NUMBER_THREADS)\"/g conf/local.conf ; \
		sed -i s/^PARALLEL_MAKE.*/PARALLEL_MAKE\ =\ \"$(PARALLEL_MAKE)\"/g conf/local.conf ; \
		sed -i '/^PREFERRED_PROVIDER_virtual\/kernel/ d' conf/local.conf ; \
		sed -i '/^PREFERRED_VERSION_linux/ d' conf/local.conf ; \
		echo "PREFERRED_PROVIDER_virtual/kernel = \"$(PREFERRED_PROVIDER_virtual/kernel)\"" >> conf/local.conf ; \
		echo "PREFERRED_VERSION_linux-yocto = \"$(LINUX_VERSION)%\"" >> conf/local.conf ; \
		echo "RELEASE_VERSION = \"$(META_SRP_REL)\"" >> conf/local.conf ; \
		echo "BB_NO_NETWORK = \"$(BB_NO_NETWORK)\"" >> conf/local.conf ; \
		echo "DISTRO_FEATURES:append = \" multilib\"" >> conf/local.conf ; \
		if [[ "$(INCLUDE_SIMICSFS)" = "true" ]]; then \
                        echo "DISTRO_FEATURES:append = \" simics\"" >> conf/local.conf ;\
			SIM_VER=`basename $(TOP)/$(SRP_REPO_NAME)/downloads/simics-*.tgz | sed 's/simics-//g' | sed 's/.tgz//g'` ; \
                        echo "SIMICS_VERSION = \"$$SIM_VER\"" >> conf/local.conf ;\
                else \
                        sed -i /DISTRO_FEATURES:append.*simics/s/^/#/g conf/local.conf; \
		fi; \
		if [[ "$(DL_DIR)" != "" ]]; then \
			echo "DL_DIR = \"$(DL_DIR)\"" >> conf/local.conf; \
		fi ; \
		echo "MACHINE = \"$(BB_MACHINE_NAME)\"" >> conf/local.conf

.PHONY: get-simics
get-simics: $(LAYERS)
ifeq ($(INCLUDE_SIMICSFS),true)
	$(call check-file-exists, $(SIMICS_FILE))
	(set -xe                                    ; \
	echo "Extracting Simicsfs client"           ; \
	mkdir -p $(TOP)/$(SRP_REPO_NAME)/downloads/ ; \
	cd $(TOP)/$(SRP_REPO_NAME)/downloads        ; \
	tar xz --wildcards -f $(SIMICS_FILE) ase/simics/simics*/src/misc ase/simics/simics*/linux64/bin ; \
	cd ase/simics                               ; \
	rm -fr simics-qsp*                          ; \
	SIMICS_VER=`ls -d simics*`                  ; \
	tar cfz $(TOP)/$(SRP_REPO_NAME)/downloads/$$SIMICS_VER.tgz $$SIMICS_VER ; \
	cd $(TOP)/$(SRP_REPO_NAME)/downloads        ; \
	echo "Created Simicsfs client $(TOP)/$(SRP_REPO_NAME)/downloads/$$SIMICS_VER.tgz")
endif

layer-list:
	echo $(LAYERS)

bbs: build
	$(Q)cd $(BLDDIR) ; \
	bash

fs: build
	$(call bitbake, $(BB_IMAGE_TYPE))

sdk: fs
	$(call bitbake-task, srp-image-dev, populate_sdk)

.PHONY: install-sdk
install-sdk:
ifdef SDK_INSTALL_DIR
	set -e ; \
	IMAGE_TYPE=srp-image-dev ; \
	cd $(BLDDIR)/tmp/deploy/sdk ; \
	mkdir -p $(SDK_INSTALL_DIR) ; \
	rm -rf $(SDK_INSTALL_DIR)/* ; \
	./$(DISTRO)-glibc-x86_64-$${IMAGE_TYPE}*toolchain-$(META_SRP_REL).sh -y -d $(SDK_INSTALL_DIR) ; \
	cd $(SDK_INSTALL_DIR) ; \
	unset LD_LIBRARY_PATH ; \
	source environment-setup*64*; \
	./external-modules-setup.sh ;
else
	$(error SDK_INSTALL_DIR is undefined, run 'make help' for usage)
endif

.PHONY: craff_gen
craff_gen:
ifeq ($(INCLUDE_SIMICSFS),true)
	set -e ;          \
	cd $(BLDDIR) ;    \
	CRAFF_BIN=$$(find tmp/work/*/simicsfs-client -name craff | grep linux64 | grep bin | head -n 1) ; \
	cd $(BLDDIR)/tmp/deploy/images/$(BB_MACHINE_NAME) ; \
	IMAGE_SUFFIX=".rootfs.hddimg"; \
	$(BLDDIR)/$$CRAFF_BIN -o $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME).craff $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME)$${IMAGE_SUFFIX}
endif

clobber:
	$(RM) -r $(BLDDIR)
	$(RM) -r $(TOP)/poky
	$(RM) -r $(TOP)/meta-*

clean:
	$(RM) -r $(BLDDIR)

cleansstate: build
	$(call bitbake-cleansstate, $(BB_IMAGE_TYPE))

