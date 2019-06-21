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
# Authors: behlul.sutarwala@intel.com
#
# Makefile for AXXIA Yocto Build

TOP 			?= $(shell pwd)
SHELL		 	 = /bin/bash
BLDDIR          	?= $(TOP)/axxia
BB_IMAGE_TYPE          	?= axxia-image-vcn
RDK_SRC_PATH		?= $(TOP)
SIMICS_FILE		?= $(TOP)/simics*
BB_NUMBER_THREADS 	?= "24"
PARALLEL_MAKE 		:= "-j $(BB_NUMBER_THREADS)"
INCLUDE_RDK		?= true
INCLUDE_SIMICSFS	?= true
META_AXXIA_REL          ?= snr_ase_rdk_1905
DL_DIR                  ?=
BB_NO_NETWORK           ?= 0
RDK_MODULES_STATIC      ?= false
AXXIA_REPO_NAME          = meta-intel-axxia

LINUX_VERSION		?= 4.19
ifeq ($(LINUX_VERSION),4.12)
  PREFERRED_VERSION_linux            = "PREFERRED_VERSION_linux-yocto"
  PREFERRED_PROVIDER_virtual/kernel := "linux-yocto"
else ifeq ($(LINUX_VERSION),4.14)
  PREFERRED_VERSION_linux            = "PREFERRED_VERSION_linux-intel"
  PREFERRED_PROVIDER_virtual/kernel := "linux-intel"
else ifeq ($(LINUX_VERSION),4.19)
  PREFERRED_VERSION_linux            = "PREFERRED_VERSION_linux-intel"
  PREFERRED_PROVIDER_virtual/kernel := "linux-intel"
  AXXIA_REPO_NAME                    = meta-intel-axxia/meta-intel-axxia
else
  $(error LINUX_VERSION is not set correctly, run 'make help' for usage)
endif

META_AXXIA_DEPENDENCY 	?= $(TOP)/meta-intel-axxia/DEPENDENCIES
DISTRO 			:= "intel-axxia"
RUNTARGET 		:= "snr"
DISTRO_FEATURES_append_intree 		:= " rdk-userspace"

# Define V=1 to echo everything
V ?= 1
ifneq ($(V),1)
	Q=@
endif

RM = $(Q)rm -f

META_AXXIA_URL   ?= https://github.com/axxia/meta-intel-axxia.git
LAYERS           += $(TOP)/meta-intel-axxia

ifeq ($(INCLUDE_RDK),true)
  META_AXXIA_RDK_URL ?= https://github.com/axxia/meta-intel-axxia-rdk.git
  LAYERS             += $(TOP)/meta-intel-axxia-rdk
  RDK_REPO_NAME       = meta-intel-axxia-rdk
  DISTRO             := "intel-axxia-indist"
endif

LAYERS       += $(TOP)

POKY_URL      = https://git.yoctoproject.org/git/poky
LAYERS       += $(TOP)/poky

OE_URL        = https://github.com/openembedded/meta-openembedded.git
LAYERS       += $(TOP)/meta-openembedded

VIRT_URL      = https://git.yoctoproject.org/git/meta-virtualization
LAYERS       += $(TOP)/meta-virtualization

INTEL_URL     = https://git.yoctoproject.org/git/meta-intel
LAYERS       += $(TOP)/meta-intel

SECURITY_URL  = https://git.yoctoproject.org/git/meta-security
LAYERS       += $(TOP)/meta-security

define bitbake
	set -e ; \
	cd $(TOP)/poky ; \
	source oe-init-build-env $(BLDDIR) ; \
	echo BLDDIR=$(BLDDIR) ; \
	cd $(BLDDIR) ; \
	bitbake $(1)
endef

define bitbake-task
	set -e ; \
	cd $(TOP)/poky ; \
	source oe-init-build-env $(BLDDIR) ; \
	cd $(BLDDIR) ; \
	bitbake $(1) -c $(2)
endef

define bitbake-cleansstate
	set -e ; \
        cd $(TOP)/poky ; \
        source oe-init-build-env $(BLDDIR) ; \
	cd $(BLDDIR) ; \
        bitbake virtual/kernel $(1) -c cleansstate ; \
	bitbake -c cleansstate simicsfs-client ; \
	bitbake -c cleansstate rdk-tools
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
            git checkout master; \
	    git pull; \
	fi;
endef

define checkout_rev
	set -e ; \
	git -C $(1) checkout $(2)
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

define check-file-exists
        echo "file=$(1)"; \
	if [ ! -f $(1) ]; then \
		echo "error $(1) is not present"; \
		exit 1; \
	fi
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
	echo "    Create yocto.craff image from hddimg after building sdk\n"; \
	echo ; \
        echo "Parameters" ; \
        echo "----------" ; \
	echo "META_AXXIA_REL=<tag-name>: Optional:" ; \
        echo "    Tag-name in META-AXXIA repo" ; \
	echo "    Defaults to current release"; \
	echo "META_AXXIA_URL=<URL-for-meta-intel-axxia>: Optional:"; \
	echo "    Defaults to https://github.com/axxia/meta-intel-axxia.git"; \
	echo "META_AXXIA_DEPENDENCY=<File which contains commit ids of meta layers>: Optional:"; \
	echo "    Defaults to <current-dir>/meta-intel-axxia/DEPENDENCIES"; \
	echo "INCLUDE_RDK: Optional: Default = true:" ; \
	echo "    When set to 'true', builds RDK in-tree,"; \
	echo "    Set to 'false' to only build Linux"; \
	echo "INCLUDE_SIMICSFS: Optional: Default = true:" ; \
	echo "    When set to 'true', builds with simicsfs,"; \
	echo "    Set to 'false' builds Linux without simicsfs"; \
	echo "META_AXXIA_RDK_URL=<URL for meta-intel-axxia-rdk>: Optional:"; \
	echo "    Only used if INCLUDE_RDK is set to 'true'"; \
	echo "    Default: https://github.com/axxia/meta-intel-axxia-rdk.git"; \
	echo "RDK_SRC_PATH=<Location-of-RDK-Archives>"; \
	echo "    Required: when INCLUDE_RDK='true'"; \
	echo "    Default:  <current-dir>"; \
	echo "    NOTE: Archives need to be in the format"; \
	echo "          rdk_klm_src.txz and rdk_user_src.txz" ; \
	echo "RDK_MODULES_STATIC: Indicates if the RDK modules should"; \
	echo "                    be statically linked in the linux kernel"; \
	echo "    Optional: Applies only when INCLUDE_RDK='true'" ; \
	echo "    Default:  false"; \
	echo "SIMICS_FILE=<Location-of-Simics-archive>: Required"; \
	echo "    Default: <current-dir>/simics-*" ; \
	echo "BLDDIR=<build dir>: Optional" ; \
	echo "    Location of the build"; \
	echo "    Defaults to <current-dir>/axxia" ; \
	echo "BB_IMAGE_TYPE=<image name>: Optional"; \
	echo "    The type of image to build"; \
	echo "    Default: axxia-image-vcn" ; \
	echo "    Other choices are: axxia-image-sim, axxia-image-small"; \
	echo "SDK_INSTALL_DIR=<sdk-install-location>"; \
	echo "    Required when 'install-sdk' is called"; \
	echo "LINUX_VERSION=<version>: Optional:"; \
	echo "    Default: 4.19" ; \
	echo "    Other choices are: 4.14, 4.12 "; \
	echo "BB_NUMBER_THREADS=<num_threads>: Optional:"; \
	echo "    Number of parallel threads to run"; \
	echo "    Defaults to '12'" ; \

all: fs

.PHONY: $(LAYERS)

$(TOP)/meta-intel-axxia:
	$(call  populate,$@,$(META_AXXIA_URL))
	$(call  checkout_rev,$@,$(META_AXXIA_REL))

$(TOP)/meta-intel-axxia-rdk:
ifeq ($(INCLUDE_RDK),true)
	$(call  populate,$@,$(META_AXXIA_RDK_URL))
	$(call  checkout_rev,$@,$(META_AXXIA_REL))
ifeq ($(RDK_MODULES_STATIC),true)
	RDK_CFG=$(TOP)/meta-intel-axxia-rdk/recipes-kernel/linux/frags/rdk-modules.cfg ; \
	sed -i 's/=m$$/=y/' $$RDK_CFG
endif
endif

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

# create bitbake build
.PHONY: build
build: get-rdk get-simics $(LAYERS)
		set -e ; \
                echo "Generating local.conf in $(BLDDIR)" ; \
		cd $(TOP); \
		rm -rf $(BLDDIR)/conf; \
		source $(AXXIA_REPO_NAME)/axxia-env ; \
		source poky/oe-init-build-env $(BLDDIR) ; \
		echo "PWD = $$PWD" ; \
		sed -i s/^DISTRO[[:space:]]*=.*/DISTRO\ =\ \"$(DISTRO)\"/g conf/local.conf ; \
		sed -i s/^RUNTARGET.*/RUNTARGET\ =\ \"$(RUNTARGET)\"/g conf/local.conf ; \
		sed -i sX^PREFERRED_PROVIDER_virtual/kernel.*XPREFERRED_PROVIDER_virtual/kernel\ =\ \"$(PREFERRED_PROVIDER_virtual/kernel)\"Xg conf/local.conf ; \
		sed -i s/^PREFERRED_VERSION_linux.*/$(PREFERRED_VERSION_linux)\ =\ \"$(LINUX_VERSION)%\"/g conf/local.conf ; \
		echo "RELEASE_VERSION = \"$(META_AXXIA_REL)\"" >> conf/local.conf ; \
		echo "BB_NO_NETWORK = \"$(BB_NO_NETWORK)\"" >> conf/local.conf ; \
		if [[ "$(INCLUDE_RDK)" == "true" ]]; then echo "DISTRO_FEATURES_append = \"$(DISTRO_FEATURES_append_intree)\"" >> conf/local.conf ;\
		if [[ "$(DL_DIR)" != "" ]]; then \
			echo "DL_DIR = \"$(DL_DIR)\"" >> conf/local.conf; \
		fi ; \
                echo "BBLAYERS += \"$(TOP)/$(RDK_REPO_NAME)\"" >> conf/bblayers.conf; fi ; \
		if [[ "$(INCLUDE_SIMICSFS)" != "true" ]]; then sed -i /DISTRO_FEATURES_append.*simicsfs/s/^/#/g conf/local.conf; fi;

.PHONY: get-rdk
get-rdk: $(LAYERS)
ifeq ($(INCLUDE_RDK),true)
	$(call check-file-exists, $(RDK_SRC_PATH)/rdk_klm_src.txz)
	$(call check-file-exists, $(RDK_SRC_PATH)/rdk_user_src.txz)
	set -e ; mkdir -p $(TOP)/$(RDK_REPO_NAME)/downloads/
	set -e ; cp --verbose $(RDK_SRC_PATH)/rdk_klm_src.txz $(TOP)/$(RDK_REPO_NAME)/downloads/rdk_klm_src.tar.xz
	set -e ; cp --verbose $(RDK_SRC_PATH)/rdk_user_src.txz $(TOP)/$(RDK_REPO_NAME)/downloads/rdk_user_src.tar.xz
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
	$(DISTRO)-glibc-x86_64-$(BB_IMAGE_TYPE)*toolchain-$(META_AXXIA_REL).sh -y -d $(SDK_INSTALL_DIR) ; \
	cd $(SDK_INSTALL_DIR) ; \
	unset LD_LIBRARY_PATH ; \
	source environment-setup*; \
	cd $(SDK_INSTALL_DIR)/sysroots/*-intelaxxia-linux/usr/src/kernel ; \
	make clean oldconfig scripts tools/objtool ;
else
	$(error SDK_INSTALL_DIR is undefined, run 'make help' for usage)
endif

.PHONY: craff_gen
craff_gen:
ifeq ($(INCLUDE_SIMICSFS),true)
	set -e ; \
	cd $(BLDDIR) ; \
	CRAFF_BIN=$$(find tmp/work/*/simicsfs-client -name craff | grep linux64 | head -n 1) ; \
	cd $(BLDDIR)/tmp/deploy/images/axxiax86-64 ; \
	$(BLDDIR)/$$CRAFF_BIN -o $(BB_IMAGE_TYPE)-axxiax86-64.craff $(BB_IMAGE_TYPE)-axxiax86-64.hddimg
endif

clobber:
	$(RM) -r $(BLDDIR)
	$(RM) -r $(TOP)/poky
	$(RM) -r $(TOP)/meta-*

clean:
	$(RM) -r $(BLDDIR)

cleansstate: build
	$(call bitbake-cleansstate, $(BB_IMAGE_TYPE))
