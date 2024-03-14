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
# Makefile for AXXIA Yocto Build

TOP                     ?= $(shell pwd)
SHELL                    = /bin/bash
BLDDIR                  ?= $(TOP)/axxia
BB_IMAGE_TYPE           ?= axxia-image-dev
BB_MACHINE_NAME         ?= intel-axxia-snr
BB_NO_NETWORK           ?= 0
BB_NUMBER_THREADS       ?= "24"
DL_DIR                  ?=
IES_ENABLE_SHM          ?= true
INCLUDE_RDK             ?= true
INCLUDE_RDK_TOOLS       ?= true
INCLUDE_SIMICSFS        ?= false
LINUX_VERSION           ?= 5.10
PARALLEL_MAKE           := "-j $(BB_NUMBER_THREADS)"
META_AXXIA_REL          ?= snr_rdk_2310.02_h_510
RDK_KLM_VERSION         ?= snr_2310.02
RDK_TOOLS_VERSION       ?= snr_2310.02
RDK_LTTNG_ENABLE        ?= false
LTTNG_VERSION           ?= "2.13%"
INCLUDE_UPDATES         ?= true
RDK_KLM_ARCHIVE         ?= rdk_klm_src.txz
RDK_SRC_PATH            ?= $(TOP)
RDK_TOOLS_ARCHIVE       ?= rdk_user_src.txz
RDK_MODULES_STATIC      ?= false
SIMICS_FILE             ?= $(TOP)/simics*
SIMICS_VERSION          ?= 6.0.51
PREFERRED_VERSION_linux  = "PREFERRED_VERSION_linux-intel"
PREFERRED_PROVIDER_virtual/kernel := "linux-intel"

ifeq (,$(filter $(LINUX_VERSION),4.19 5.4 5.10 5.15))
  $(error LINUX_VERSION is not set correctly, run 'make help' for usage)
endif

META_AXXIA_DEPENDENCY               ?= $(TOP)/meta-intel-axxia/DEPENDENCIES.distro
DISTRO                              := "intel-axxia"
RUNTARGET                           := "vcn"
DISTRO_FEATURES_append_intree       := " rdk-userspace"

AXXIA_REPO_NAME                     := meta-intel-axxia/meta-intel-distro
RDK_REPO_NAME                       := meta-intel-axxia/meta-intel-rdk
UPDATES_REPO_NAME                   := meta-intel-axxia-updates

# Define V=1 to echo everything
V ?= 1
ifneq ($(V),1)
        Q=@
endif

RM = $(Q)rm -f

META_AXXIA_URL   ?= https://github.com/axxia/meta-intel-axxia.git
LAYERS           += $(TOP)/meta-intel-axxia

POKY_URL          = https://git.yoctoproject.org/git/poky
LAYERS           += $(TOP)/poky

OE_URL            = https://github.com/openembedded/meta-openembedded.git
LAYERS           += $(TOP)/meta-openembedded

VIRT_URL          = https://git.yoctoproject.org/git/meta-virtualization
LAYERS           += $(TOP)/meta-virtualization

INTEL_URL         = https://git.yoctoproject.org/git/meta-intel
LAYERS           += $(TOP)/meta-intel

SECURITY_URL      = https://git.yoctoproject.org/git/meta-security
LAYERS           += $(TOP)/meta-security

META_ROS          = https://github.com/bmwcarit/meta-ros.git
LAYERS           += $(TOP)/meta-ros

META_UPDATES_URL ?= https://github.com/axxia/meta-intel-axxia-updates.git
LAYERS           += $(TOP)/meta-intel-axxia-updates

define source-bb-env
    cd $(TOP)/meta-intel-axxia               ; \
    source meta-intel-distro/axxia-env       ; \
    cd $(TOP)/poky                           ; \
    source oe-init-build-env $(BLDDIR)
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
	if [[ "$(INCLUDE_RDK)" = "true" && "$(INCLUDE_RDK_TOOLS)" = "true" ]]; then \
		CLEAN_TARGETS="$$CLEAN_TARGETS rdk-tools linux-firmware" ; fi ; \
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
	$(call check-file-exists, $(META_AXXIA_DEPENDENCY))
	set -e ; \
	BASE_LAYER=`basename $(1)`;\
	REV=`awk -v layer=$$BASE_LAYER '{\
	if($$0 ~ "^Dependencies" ) start_parse=1;\
	if($$0 ~ "^Back Ported" ) start_parse=0;\
	if(start_parse == 1 && $$0 ~ "URI" && $$0 ~ layer) parse_layer=1;\
	if(parse_layer == 1 && $$0 ~ "^revision") { print $$2; parse_layer=0; }\
	}' $(META_AXXIA_DEPENDENCY)`;\
	if [ -z "$$REV" ]; then echo "ERROR: Checkout REV of $$BASE_LAYER could not be read"; exit 1; fi; \
        echo "REPO: $$BASE_LAYER, REV: $$REV" ; \
        git -C $(1) checkout $$REV
endef

help:
	@echo "USAGE:"; \
        echo  "------" ; \
	echo ; \
	echo "$$ make fs META_AXXIA_REL=<tag-name>"; \
	echo "    Pull and Build Yocto Linux with RDK In-Tree"; \
	echo "$$ make fs META_AXXIA_REL=<tag-name> INCLUDE_RDK=false"; \
	echo "    Pull and Build Yocto Linux" ; \
	echo "$$ make sdk install-sdk META_AXXIA_REL=<tag-name> INCLUDE_RDK=false SDK_INSTALL_DIR=<sdk-install-dir>"; \
	echo "    Pull and Build Yocto Linux and then build and install sdk"; \
	echo "$$ make sdk install-sdk META_AXXIA_REL=<tag-name> SDK_INSTALL_DIR=<sdk-install-dir>"; \
	echo "    Build and Install SDK with RDK in-tree" ; \
	echo "$$ make craff_gen META_AXXIA_REL=<tag-name>" ; \
	echo "    Create yocto.craff image from .wic after building sdk\n"; \
	echo ; \
        echo "Parameters" ; \
        echo "----------" ; \
	echo "META_AXXIA_REL=<tag-name>: Optional:" ; \
        echo "    Tag-name in META-AXXIA repo" ; \
	echo "    Defaults to current release"; \
	echo "META_AXXIA_URL=<URL-for-meta-intel-axxia>: Optional:"; \
	echo "    Defaults to https://github.com/axxia/meta-intel-axxia.git"; \
	echo "META_AXXIA_DEPENDENCY=<File which contains commit ids of meta layers>: Optional:"; \
	echo "    Defaults to <current-dir>/meta-intel-axxia/DEPENDENCIES.distro"; \
	echo "INCLUDE_RDK: Optional: Default = true:" ; \
	echo "    When set to 'true', builds RDK in-tree,"; \
	echo "    Set to 'false' to only build Linux"; \
	echo "INCLUDE_RDK_TOOLS: Optional: Default = true:" ; \
	echo "    Depends on INCLUDE_RDK = true,"; \
	echo "    When set to 'true', builds RDK userspace tools,"; \
	echo "    Set to 'false' build RDK in-tree without rdk-tools"; \
	echo "INCLUDE_SIMICSFS: Optional: Default = false:" ; \
	echo "    When set to 'true', builds with simics,"; \
	echo "    Set to 'false' builds Linux without simics"; \
	echo "RDK_SRC_PATH=<Location-of-RDK-Archives>"; \
	echo "    Required: when INCLUDE_RDK='true'"; \
	echo "    Default:  <current-dir>"; \
	echo "RDK_KLM_ARCHIVE=<Name-of-RDK-KLM-Archive>"; \
	echo "    Required: when INCLUDE_RDK='true'"; \
	echo "    Default:  <current-dir>/rdk_klm_src.txz"; \
	echo "RDK_TOOLS_ARCHIVE=<Name-of-RDK-USERSPACE-TOOLS-Archive>"; \
	echo "    Required: when INCLUDE_RDK='true' and INCLUDE_RDK_TOOLS='true'"; \
	echo "    Default:  <current-dir>/rdk_user_src.txz"; \
	echo "RDK_MODULES_STATIC: Indicates if the RDK modules should"; \
	echo "                    be statically linked in the linux kernel"; \
	echo "    Optional: Applies only when INCLUDE_RDK='true'" ; \
	echo "    Default:  false"; \
	echo "IES_ENABLE_SHM=[true|false]:"; \
	echo "    Enable building for RDK with IES in shared-memory mode" ; \
	echo "    Defaults to 'true'" ; \
	echo "    When set to false, will build IES in RPC mode" ; \
	echo "RDK_LTTNG_ENABLE=[true|false]:"; \
	echo "    Enable building RDK with LTTNG enabled in user space" ; \
	echo "    Defaults to 'false'" ; \
	echo "    When set to true, will build RDK with LTTNG enabled" ; \
	echo "LTTNG_VERSION=<lttng_version>:"; \
	echo "    The version of LTTNG to build with" ; \
	echo "    Defaults to '2.13*'" ; \
	echo "SIMICS_FILE=<Location-of-Simics-archive>: Required"; \
	echo "    Default: <current-dir>/simics-*" ; \
	echo "BLDDIR=<build dir>: Optional" ; \
	echo "    Location of the build"; \
	echo "    Defaults to <current-dir>/axxia" ; \
	echo "BB_IMAGE_TYPE=<image name>: Optional"; \
	echo "    The type of image to build"; \
	echo "    Default: axxia-image-dev" ; \
	echo "    Other choices are: axxia-image-run, axxia-image-small"; \
	echo "BB_MACHINE_NAME=<machine-name>: Optional"; \
	echo "    Yocto MACHINE: intel-axxia-snr or intel-axxia-grr"; \
	echo "    Default: intel-axxia-grr"; \
	echo "SDK_INSTALL_DIR=<sdk-install-location>"; \
	echo "    Required when 'install-sdk' is called"; \
	echo "LINUX_VERSION=<version>: Optional:"; \
	echo "    Default: 5.10" ; \
	echo "    Supported versions are: 4.19, 5.4, 5.10, 5.15"; \
	echo "SIMICS_VERSION=<version>" ; \
	echo "    Optional: when INCLUDE_SIMICSFS='false'" ; \
	echo "    Default: 6.0.51" ; \
	echo "INCLUDE_UPDATES: Optional: Default = true" ; \
	echo "    When set to 'true', will include meta-intel-axxia-updates Yocto"; \
	echo "    layer with minimal changes to fix older Intel Axxia relases"; \
	echo "META_UPDATES_URL=<URL-for-meta-intel-axxia-updates>: Optional:"; \
	echo "    Defaults to https://github.com/axxia/meta-intel-axxia-updates.git";\
	echo "    Required: when INCLUDE_UPDATES='true'"; \
	echo "BB_NUMBER_THREADS=<num_threads>: Optional:"; \
	echo "    Number of parallel threads to run"; \
	echo "    Defaults to '24'" ; \
	echo "DL_DIR=<download-directory>: Optional:"; \
	echo "    A pre-populated shared download directory used for"; \
	echo "    build time/space saving"; \
	echo "    Defaults to BLDDIR/downloads" ; \

all: fs

.PHONY: $(LAYERS)

$(TOP)/meta-intel-axxia:
	$(call  populate,$@,$(META_AXXIA_URL))
	$(call  checkout_rev,$@,$(META_AXXIA_REL))

$(TOP)/meta-openembedded: $(TOP)/meta-intel-axxia
	$(call  populate,$@,$(OE_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/meta-virtualization: $(TOP)/meta-intel-axxia
	$(call  populate,$@,$(VIRT_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/meta-intel: $(TOP)/meta-intel-axxia
	$(call  populate,$@,$(INTEL_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/poky: $(TOP)/meta-intel-axxia
	$(call  populate,$@,$(POKY_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/meta-security: $(TOP)/meta-intel-axxia
	$(call  populate,$@,$(SECURITY_URL))
	$(call  checkout_layer_from_file,$@)

$(TOP)/meta-ros: $(TOP)/meta-intel-axxia
ifeq ($(LINUX_VERSION),4.19)
	$(call  populate,$@,$(META_ROS))
	$(call  checkout_layer_from_file,$@)
endif

$(TOP)/meta-intel-axxia-updates: $(TOP)/meta-intel-axxia-updates
ifeq ($(INCLUDE_UPDATES),true)
	$(call  populate,$@,$(META_UPDATES_URL))
	$(call  checkout_rev_main,$@,$(META_AXXIA_REL))
endif

# create bitbake build
.PHONY: build
build: get-rdk get-simics $(LAYERS)
		set -e ; \
        echo "Generating local.conf in $(BLDDIR)" ; \
		cd $(TOP); \
		rm -rf $(BLDDIR)/conf; \
		$(call source-bb-env) ; \
		echo "PWD = $$PWD" ; \
		sed -i s/^MACHINE[[:space:]]*=.*/MACHINE\ =\ \"$(BB_MACHINE_NAME)\"/g conf/local.conf ; \
		sed -i s/^DISTRO[[:space:]]*=.*/DISTRO\ =\ \"$(DISTRO)\"/g conf/local.conf ; \
		sed -i s/^RUNTARGET.*/RUNTARGET\ =\ \"$(RUNTARGET)\"/g conf/local.conf ; \
		sed -i s/^BB_NUMBER_THREADS.*/BB_NUMBER_THREADS\ =\ \"$(BB_NUMBER_THREADS)\"/g conf/local.conf ; \
		sed -i s/^PARALLEL_MAKE.*/PARALLEL_MAKE\ =\ \"$(PARALLEL_MAKE)\"/g conf/local.conf ; \
		sed -i '/^PREFERRED_PROVIDER_virtual\/kernel/ d' conf/local.conf ; \
		sed -i '/^PREFERRED_VERSION_linux/ d' conf/local.conf ; \
		echo "PREFERRED_PROVIDER_virtual/kernel = \"$(PREFERRED_PROVIDER_virtual/kernel)\"" >> conf/local.conf ; \
		echo "$(PREFERRED_VERSION_linux) = \"$(LINUX_VERSION)%\"" >> conf/local.conf ; \
		echo "RELEASE_VERSION = \"$(META_AXXIA_REL)\"" >> conf/local.conf ; \
		echo "BB_NO_NETWORK = \"$(BB_NO_NETWORK)\"" >> conf/local.conf ; \
		if [[ "$(INCLUDE_RDK)" == "true" ]]; then \
			bitbake-layers add-layer -F $(TOP)/$(RDK_REPO_NAME) ; \
			echo "DISTRO_FEATURES:append = \"$(DISTRO_FEATURES_append_intree)\"" >> conf/local.conf ; \
			sed -i '/^RDK_.*._ARCHIVE/ d' conf/local.conf ; \
			sed -i '/^RDK_.*._VERSION/ d' conf/local.conf ; \
			RC=$(grep IES_ENABLE_SHM conf/local.conf) ; \
			if [[ $$RC == 0 ]]; then \
				echo "s/IES_ENABLE_SHM.*$$/IES_ENABLE_SHM = \"$(IES_ENABLE_SHM)\"/g" > sed.tmp ; \
				sed -i -f sed.tmp conf/local.conf ; \
				rm -f sed.tmp ; \
			else \
				echo "IES_ENABLE_SHM = \"$(IES_ENABLE_SHM)\"" >> conf/local.conf ; \
			fi ; \
			RC=$(grep RDK_LTTNG_ENABLE conf/local.conf) ; \
			if [[ $$RC == 0 ]]; then \
				echo "s/RDK_LTTNG_ENABLE.*$$/RDK_LTTNG_ENABLE = \"$(RDK_LTTNG_ENABLE)\"/g" > sed.tmp ; \
				sed -i -f sed.tmp conf/local.conf ; \
				rm -f sed.tmp; \
			else \
				echo "RDK_LTTNG_ENABLE = \"$(RDK_LTTNG_ENABLE)\"" >> conf/local.conf ; \
			fi ; \
			echo "LTTNG_VERSION = \"$(LTTNG_VERSION)\"" >> conf/local.conf ; \
			echo "RDK_KLM_ARCHIVE = \"file://$(RDK_SRC_PATH)/$(RDK_KLM_ARCHIVE)\"" >> conf/local.conf ; \
			echo "RDK_KLM_VERSION = \"$(RDK_KLM_VERSION)\"" >> conf/local.conf ; \
			if [[ "$(INCLUDE_RDK_TOOLS)" == "true" ]]; then \
				echo "RDK_TOOLS_ARCHIVE = \"file://$(RDK_SRC_PATH)/$(RDK_TOOLS_ARCHIVE)\"" >> conf/local.conf ; \
				echo "RDK_TOOLS_VERSION = \"$(RDK_TOOLS_VERSION)\"" >> conf/local.conf ; \
			fi ; \
		fi ; \
		if [[ "$(INCLUDE_SIMICSFS)" = "true" ]]; then \
                        echo "DISTRO_FEATURES:append = \" simics\"" >> conf/local.conf ;\
                        echo "SIMICS_VERSION = \"$(SIMICS_VERSION)\"" >> conf/local.conf ;\
                else \
                        sed -i /DISTRO_FEATURES:append.*simics/s/^/#/g conf/local.conf; \
		fi; \
		if [[ "$(DL_DIR)" != "" ]]; then \
			echo "DL_DIR = \"$(DL_DIR)\"" >> conf/local.conf; \
		fi ; \
		if [[ "$(INCLUDE_UPDATES)" == "true" ]]; then \
			bitbake-layers add-layer -F $(TOP)/$(UPDATES_REPO_NAME); \
		fi

.PHONY: get-rdk
get-rdk: $(LAYERS)
ifeq ($(INCLUDE_RDK),true)
	$(call check-file-exists, $(RDK_SRC_PATH)/$(RDK_KLM_ARCHIVE))
ifeq ($(INCLUDE_RDK_TOOLS),true)
	$(call check-file-exists, $(RDK_SRC_PATH)/$(RDK_TOOLS_ARCHIVE))
endif
ifeq ($(RDK_MODULES_STATIC),true)
	(RDK_CFG=$(TOP)/$(RDK_REPO_NAME)/recipes-kernel/linux/frags/rdk-modules.cfg ; \
	sed -i 's/=m$$/=y/' $$RDK_CFG ; \
	cd `dirname $$RDK_CFG` ; \
	git commit -a -m"Set modules to static for $(META_AXXIA_REL)")
endif
endif

.PHONY: get-simics
get-simics: $(LAYERS)
ifeq ($(INCLUDE_SIMICSFS),true)
	$(call check-file-exists, $(SIMICS_FILE))
	mkdir -p $(TOP)/$(AXXIA_REPO_NAME)/downloads/
	cp --verbose $(SIMICS_FILE) $(TOP)/$(AXXIA_REPO_NAME)/downloads/.
endif

layer-list:
	echo $(LAYERS)

bbs: build
	$(Q)cd $(BLDDIR) ; \
	bash

fs: build
	$(call bitbake, $(BB_IMAGE_TYPE))

sdk: fs
	$(call bitbake-task, $(BB_IMAGE_TYPE), populate_sdk)

.PHONY: install-sdk
install-sdk:
ifdef SDK_INSTALL_DIR
	set -e ; \
	cd $(BLDDIR)/tmp/deploy/sdk ; \
	mkdir -p $(SDK_INSTALL_DIR) ; \
	rm -rf $(SDK_INSTALL_DIR)/* ;\
	./$(DISTRO)-glibc-x86_64-$(BB_IMAGE_TYPE)*toolchain-$(META_AXXIA_REL).sh -y -d $(SDK_INSTALL_DIR) ; \
	cd $(SDK_INSTALL_DIR) ; \
	unset LD_LIBRARY_PATH ; \
	source environment-setup*; \
	./external-modules-setup.sh ;
else
	$(error SDK_INSTALL_DIR is undefined, run 'make help' for usage)
endif

.PHONY: craff_gen
craff_gen:
ifeq ($(INCLUDE_SIMICSFS),true)
	set -e ; \
	cd $(BLDDIR) ; \
	CRAFF_BIN=$$(find tmp/work/*/simicsfs-client -name craff | grep linux64 | grep bin | head -n 1) ; \
	cd $(BLDDIR)/tmp/deploy/images/$(BB_MACHINE_NAME) ; \
	$(BLDDIR)/$$CRAFF_BIN -o $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME).craff $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME).wic
endif

clobber:
	$(RM) -r $(BLDDIR)
	$(RM) -r $(TOP)/poky
	$(RM) -r $(TOP)/meta-*

clean:
	$(RM) -r $(BLDDIR)

cleansstate: build
	$(call bitbake-cleansstate, $(BB_IMAGE_TYPE))

