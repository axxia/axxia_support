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
BLDDIR                  ?= $(TOP)/axxia
BB_IMAGE_TYPE           ?= axxia-image-run
BB_IMAGE_TYPE_SDK       ?= axxia-image-dev
BB_MACHINE_NAME         ?= intel-axxia-pmr
BB_NO_NETWORK           ?= 0
BB_NUMBER_THREADS       ?= "24"
DL_DIR                  ?=
INCLUDE_RDK             ?= true
INCLUDE_RDK_TOOLS       ?= true
INCLUDE_SIMICSFS        ?= true
INCLUDE_UPDATES         ?= false
LINUX_VERSION           ?= v6.16
PARALLEL_MAKE           := "-j $(BB_NUMBER_THREADS)"
META_AXXIA_REL          ?= pmr_ase_del_9B
RDK_TOOLS_VERSION       ?= pmr_del9B
RDK_LTTNG_ENABLE        ?= false
RDK_MODULES_STATIC      ?= false
RDK_SRC_PATH            ?= $(TOP)
RDK_TOOLS_ARCHIVE       ?= pmr_rdk_user_src.tgz
USE_RDK_REPO            ?= false
RDK_REPO                ?= https://github.com/intel-collab/networking.wireless.ran.bts-rdk-releases
SIMICS_FILE             ?= $(word 1,$(wildcard $(TOP)/pmr_ase_svl_vp*))
ROOTFS_EXTRA_SPACE      ?= "1M"
INCLUDE_I3C_TOOLS       ?= true
I3C_TOOLS_REPO          ?= public
#
# Ethtool specific info. Required while upstream code is in-flight
#
IPROUTE2_VERSION        ?= "6.16%"
ETHTOOL_VERSION         := "git"
ETHTOOL_GIT_URL         ?= "https://kernel.googlesource.com/pub/scm/network/ethtool/ethtool"
ETHTOOL_EXTRA_PATH      ?= $(TOP)/pmr_ethtool_patches
ETHTOOL_GIT_BRANCH      ?= "master"
ETHTOOL_GIT_SRCREV      ?= $(shell cat $(ETHTOOL_EXTRA_PATH)/ethtool_base_commit.txt)

# Allow for this to be overridden
RELEASE_VERSION         ?= $(META_AXXIA_REL)

#
# Specify the kernel source tree and version
#
KERNEL_SOURCE                       ?= "linux-korg-netnext"
KORG_NETNEXT_EXTRA_PATH             ?= $(TOP)/pmr_rdk_klm_src
KORG_NETNEXT_REVISION               ?= $(file < $(KORG_NETNEXT_EXTRA_PATH)/netdev_commit_id.txt)
PREFERRED_PROVIDER_virtual/kernel   ?= "$(KERNEL_SOURCE)"
KORG_VERSION                        ?= $(LINUX_VERSION)
KBRANCH_INTEL_PRIVATE               ?= $(META_AXXIA_REL)

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
META_UPDATES_URL ?= https://github.com/axxia/meta-intel-axxia-updates.git
LAYERS           += $(TOP)/meta-intel-axxia
LAYERS           += $(TOP)/meta-intel-axxia-updates

# get_layers: clone or update every layer listed in DEPENDENCIES.distro.
# Runs at recipe time so no parse-time access to the file is needed.
# Uses the revision when set; falls back to the branch name.
define get_layers
	set -e                                                       ; \
	echo "--- Fetching layers from $(META_AXXIA_DEPENDENCY) ---" ; \
	_GL_TMP=$$(mktemp)                                           ; \
	awk '/^[[:alpha:]][[:alnum:]_-]*[[:space:]]*$$/ &&             \
	    !/^Dependencies/ && !/^Back/ {                             \
	        if (layer != "" && uri != "")                          \
		    printf "%s %s %s %s\n", layer, uri, branch, rev;   \
	        layer=$$1; uri=""; branch=""; rev=""                   \
	    }                                                          \
	    /^URI:/      { uri=$$2 }                                   \
	    /^branch:/   { branch=$$2 }                                \
	    /^revision:/ { rev=$$2 }                                   \
	    END { if (layer != "" && uri != "") printf "%s %s %s %s\n", layer, uri, branch, rev } \
	' $(META_AXXIA_DEPENDENCY) > "$$_GL_TMP"                     ; \
	while IFS=' ' read -r _gl_name _gl_uri _gl_branch _gl_rev; do  \
	    _gl_ref="$${_gl_rev:-$$_gl_branch}"                      ; \
	    echo "  $$_gl_name  =>  $$_gl_ref"                       ; \
	    if [ ! -d "$(TOP)/$$_gl_name" ]; then                      \
	        git clone "$$_gl_uri" "$(TOP)/$$_gl_name"            ; \
	    else                                                       \
	        git -C "$(TOP)/$$_gl_name" fetch --all -f            ; \
	    fi                                                       ; \
	    git -C "$(TOP)/$$_gl_name" checkout "$$_gl_ref"          ; \
	done < "$$_GL_TMP"                                           ; \
	rm -f "$$_GL_TMP"                                            ; \
	echo "INFO: Meta-Layers fetched"
endef

# verify_layers: confirm every layer listed in DEPENDENCIES.distro is present.
define verify_layers
	set -e                                                       ; \
	_VL_TMP=$$(mktemp)                                           ; \
	awk '/^[[:alpha:]][[:alnum:]_-]*[[:space:]]*$$/ &&             \
	     !/^Dependencies/ && !/^Back/ {                            \
	        if (layer != "") print layer; layer=$$1                \
	    }                                                          \
	    END { if (layer != "") print layer }                       \
	' $(META_AXXIA_DEPENDENCY) > "$$_VL_TMP"                     ; \
	while IFS=' ' read -r _vl_name; do                             \
	    [ -d "$(TOP)/$$_vl_name" ] ||                              \
	        { echo "ERROR: layer '$$_vl_name' missing at $(TOP)/$$_vl_name — run 'make get_layers'"; \
	          rm -f "$$_VL_TMP"; exit 1; }                       ; \
	done < "$$_VL_TMP"                                           ; \
	rm -f "$$_VL_TMP"                                            ; \
	echo "INFO: All layers verified."
endef

# source-bb-env: determine the OE init layer by parsing DEPENDENCIES.distro.
# 'poky' (used from v6.16+) takes precedence; falls back to 'openembedded-core'.
define source-bb-env
	set -e ; \
	_OE_LAYER=$$(awk '\
	    /^poky[[:space:]]*$$/         { print "poky";        exit } \
	    /^openembedded-core[[:space:]]*$$/ {print "openembedded-core"; exit} \
	' $(META_AXXIA_DEPENDENCY))                                   ; \
	[ -n "$$_OE_LAYER" ] ||                                         \
	    { echo "ERROR: 'poky' or 'openembedded-core' not found "; exit 1; } ; \
	_OE_DIR="$(TOP)/$$_OE_LAYER"                                  ; \
	cd $(TOP)/meta-intel-axxia                                    ; \
	source meta-intel-distro/axxia-env                            ; \
	cd "$$_OE_DIR"                                                ; \
	source oe-init-build-env $(BLDDIR)
endef

define bitbake
	set -e                                                        ; \
	$(call source-bb-env)                                         ; \
	echo BLDDIR=$(BLDDIR)                                         ; \
	cd $(BLDDIR)                                                  ; \
	bitbake -c cleanall $(1)                                      ; \
	bitbake $(1)
endef

define bitbake-task
	set -e                                                        ; \
	$(call source-bb-env)                                         ; \
	cd $(BLDDIR)                                                  ; \
	bitbake $(1) -c $(2)
endef

define bitbake-cleansstate
	set -e                                                        ; \
	$(call source-bb-env)                                         ; \
	cd $(BLDDIR)                                                  ; \
	CLEAN_TARGETS="virtual/kernel $(1)"                           ; \
	if [[ "$(INCLUDE_SIMICSFS)" = "true" ]]; then CLEAN_TARGETS="$$CLEAN_TARGETS simicsfs-client" ; fi ; \
	if [[ "$(INCLUDE_RDK)" = "true" && "$(INCLUDE_RDK_TOOLS)" = "true" ]]; then \
		CLEAN_TARGETS="$$CLEAN_TARGETS rdk-tools linux-firmware";\
	fi                                                             ; \
	bitbake -c cleansstate $$CLEAN_TARGETS
endef

define populate
	echo "$(1)"                                                    ; \
	set -e                                                         ; \
	if [ ! -d "$(1)" ]; then                                         \
	    echo "Cloning Repo $(2)"                                   ; \
	    git clone $(2) $(1)                                        ; \
	else                                                             \
	    echo "Pull on Repo $(2)"                                   ; \
	    cd $(1)                                                    ; \
            git clean -dfx                                             ; \
	    git fetch --all -f                                         ; \
	fi
endef

#
# Default to main if branch not available.
#
define checkout_rev_main
	set -e                                                         ; \
	(git -C $(1) show-branch origin/$(2) &>/dev/null) &&             \
		(git -C $(1) checkout $(2)) ||                           \
		(git -C $(1) checkout main)
endef

define checkout_rev
	set -e                                                         ; \
	git -C $(1) checkout $(2)
endef

define check-file-exists
        echo "Checking for file: $(1)"                                 ; \
	if [ ! -f $(1) ]; then                                           \
		echo "Error: $(1) is not present"                      ; \
		exit 1                                                 ; \
	fi
endef

define setup_conf
	set -e ; \
	echo "Generating local.conf in $(BLDDIR)" ; \
	cd $(TOP); \
	rm -rf $(BLDDIR)/conf ; \
	$(call source-bb-env) ; \
	echo "PWD = $$PWD" ; \
	sed -i s/^MACHINE[[:space:]]*=.*/MACHINE\ =\ \"$(BB_MACHINE_NAME)\"/g conf/local.conf ; \
	sed -i s/^DISTRO[[:space:]]*=.*/DISTRO\ =\ \"$(DISTRO)\"/g conf/local.conf ; \
	sed -i s/^MACHINE.*/MACHINE\ =\ \"$(BB_MACHINE_NAME)\"/g conf/local.conf; \
	sed -i s/^BB_NUMBER_THREADS.*/BB_NUMBER_THREADS\ =\ \"$(BB_NUMBER_THREADS)\"/g conf/local.conf ; \
	sed -i s/^PARALLEL_MAKE.*/PARALLEL_MAKE\ =\ \"$(PARALLEL_MAKE)\"/g conf/local.conf ; \
	echo "BB_NO_NETWORK = \"$(BB_NO_NETWORK)\"" >> conf/local.conf ; \
	echo "RELEASE_VERSION = \"$(RELEASE_VERSION)\"" >> conf/local.conf ; \
	echo "WIC_ROOTFS_EXTRA_SPACE = \"$(ROOTFS_EXTRA_SPACE)\"" >> conf/local.conf
endef

define setup_kernel
	cp -pf $(KORG_NETNEXT_EXTRA_PATH)/pmr_defconfig $(KORG_NETNEXT_EXTRA_PATH)/defconfig ; \
	if [[ "$(KERNEL_SOURCE)" == "linux-korg-netnext" ]]; then \
                echo "KORG_VERSION = \"$(KORG_VERSION)\"" >> conf/local.conf ; \
                echo "KORG_NETNEXT_EXTRA_PATH = \"$(KORG_NETNEXT_EXTRA_PATH)\"" >> conf/local.conf ; \
	        echo "KORG_NETNEXT_REVISION = \"$(KORG_NETNEXT_REVISION)\"" >> conf/local.conf ; \
                echo "SRCREV:pn-linux-korg-netnext = \"$(KORG_NETNEXT_REVISION)\"" >> conf/local.conf ; \
                echo "SRCREV:pn-linux-korg-netnext-debug = \"$(KORG_NETNEXT_REVISION)\"" >> conf/local.conf ; \
	elif [[ "$(KERNEL_SOURCE)" == "linux-local" ]]; then \
                echo "LOCAL_KERNEL_PATH = \"$(LINUX_LOCAL_PATH)\"" >> conf/local.conf ; \
                echo "LOCAL_KERNEL_BRANCH = \"$(LINUX_LOCAL_BRANCH)\"" >> conf/local.conf ; \
                echo "LOCAL_DEFCONFIG = \"$(LINUX_LOCAL_DEFCONFIG)\"" >> conf/local.conf ; \
	else  \
                echo "KBRANCH_INTEL_PRIVATE = \"$(KBRANCH_INTEL_PRIVATE)\"" >> conf/local.conf ; \
	fi
endef

define setup_rdk
	if [[ "$(RDK_LTTNG_ENABLE)" == "false" ]]; then \
	    echo "LTTNG_SUPPORT = \"\"" >> conf/local.conf ; \
	fi ; \
	if [[ "$(INCLUDE_RDK)" == "true" ]]; then \
		bitbake-layers add-layer -F $(TOP)/$(RDK_REPO_NAME) ; \
		echo "DISTRO_FEATURES:append = \"$(DISTRO_FEATURES_append_intree)\"" >> conf/local.conf ; \
		sed -i '/^RDK_.*._ARCHIVE/ d' conf/local.conf ; \
		sed -i '/^RDK_.*._VERSION/ d' conf/local.conf ; \
		RC=$(grep RDK_LTTNG_ENABLE conf/local.conf) ; \
		if [[ $$RC == 0 ]]; then \
			echo "s/RDK_LTTNG_ENABLE.*$$/RDK_LTTNG_ENABLE = \"$(RDK_LTTNG_ENABLE)\"/g" > sed.tmp ; \
			sed -i -f sed.tmp conf/local.conf ; \
			rm -f sed.tmp; \
		else \
			echo "RDK_LTTNG_ENABLE = \"$(RDK_LTTNG_ENABLE)\"" >> conf/local.conf ; \
		fi ; \
		if [[ "$(USE_RDK_REPO)" == "true" ]]; then \
			echo "USE_RDK_REPO = \"$(USE_RDK_REPO)\"" >> conf/local.conf ; \
			echo "RDK_REPO = \"$(RDK_REPO)\"" >> conf/local.conf ; \
			if [[ "x$(RDK_REPO_REV)" != "x" ]]; then \
				echo "RDK_REPO_REV = \"$(RDK_REPO_REV)\"" >> conf/local.conf ; \
			else \
				echo "Error: RDK_REPO_REV not set. When USE_RDK_REPO is 'true', need to set RDK_REPO_REV to either a commit SHA or tag."; \
				exit 1; \
			fi ; \
		elif [[ "$(INCLUDE_RDK_TOOLS)" == "true" ]]; then \
			echo "RDK_TOOLS_ARCHIVE = \"file://$(RDK_SRC_PATH)/$(RDK_TOOLS_ARCHIVE)\"" >> conf/local.conf ; \
			echo "RDK_TOOLS_VERSION = \"$(RDK_TOOLS_VERSION)\"" >> conf/local.conf ; \
		fi ; \
	fi
endef

define setup_simicsfs
	if [[ "$(INCLUDE_SIMICSFS)" = "true" ]]; then \
            SIM_VER=`basename $(TOP)/$(AXXIA_REPO_NAME)/downloads/simics-*.tgz | sed 's/simics-//g' | sed 's/.tgz//g'` ; \
            echo "SIMICS_VERSION = \"$$SIM_VER\"" >> conf/local.conf       ; \
            echo "DISTRO_FEATURES:append = \" simics\"" >> conf/local.conf ; \
        else                                                                 \
            sed -i /DISTRO_FEATURES:append.*simics/s/^/#/g conf/local.conf ; \
	fi
endef

define setup_i3ctools
	if [[ "$(INCLUDE_I3C_TOOLS)" == "true" ]]; then \
	    echo "IMAGE_INSTALL:append = \" i3c-tools\"" >> conf/local.conf  ; \
	    echo "I3C_TOOLS_REPO = \"$(I3C_TOOLS_REPO)\"" >> conf/local.conf ; \
	fi
endef

define setup_ethtool
	echo "IPROUTE2_VERSION = \"$(IPROUTE2_VERSION)\"" >> conf/local.conf ; \
	echo "ETHTOOL_VERSION = \"$(ETHTOOL_VERSION)\""   >> conf/local.conf ; \
	echo "ETHTOOL_GIT_URL = \"$(ETHTOOL_GIT_URL)\""   >> conf/local.conf ; \
	echo "ETHTOOL_GIT_BRANCH = \"$(ETHTOOL_GIT_BRANCH)\"" >> conf/local.conf ; \
	echo "ETHTOOL_GIT_SRCREV = \"$(ETHTOOL_GIT_SRCREV)\"" >> conf/local.conf ; \
	echo "ETHTOOL_EXTRA_PATH = \"$(ETHTOOL_EXTRA_PATH)\"" >> conf/local.conf
endef

all: fs

.PHONY: $(LAYERS)

$(TOP)/meta-intel-axxia:
	$(call  populate,$@,$(META_AXXIA_URL))
	$(call  checkout_rev,$@,$(META_AXXIA_REL))

$(TOP)/meta-intel-axxia-updates: $(TOP)/meta-intel-axxia
ifeq ($(INCLUDE_UPDATES),true)
	$(call  populate,$@,$(META_UPDATES_URL))
	$(call  checkout_rev_main,$@,$(META_AXXIA_REL))
endif

.PHONY: qsp
qsp: BB_IMAGE_TYPE=axxia-image-qsp
qsp: get-simics $(TOP)/meta-intel-axxia
	$(call get_layers)    ; \
	$(call verify_layers) ; \
	$(call setup_conf)    ; \
	echo "DISTRO_FEATURES:append = \" qsp\"" >> conf/local.conf ; \
	$(call bitbake, $(BB_IMAGE_TYPE))

# create bitbake build
.PHONY: build
build: get-rdk get-simics $(TOP)/meta-intel-axxia $(TOP)/meta-intel-axxia-updates
	$(call get_layers)    ; \
	$(call verify_layers) ; \
	$(call setup_conf)    ; \
	sed -i s/^RUNTARGET.*/RUNTARGET\ =\ \"$(RUNTARGET)\"/g conf/local.conf ; \
	sed -i '/^PREFERRED_PROVIDER_virtual\/kernel/ d' conf/local.conf ; \
	sed -i '/^PREFERRED_VERSION_linux/ d' conf/local.conf ; \
	echo "PREFERRED_PROVIDER_virtual/kernel = \"$(PREFERRED_PROVIDER_virtual/kernel)\"" >> conf/local.conf ; \
	echo "DISTRO_FEATURES:append = \" multilib\"" >> conf/local.conf ; \
	if [[ "$(DL_DIR)" != "" ]]; then \
	    echo "DL_DIR = \"$(DL_DIR)\"" >> conf/local.conf; \
	fi ; \
	if [[ "$(INCLUDE_UPDATES)" == "true" ]]; then \
	    bitbake-layers add-layer -F $(TOP)/$(UPDATES_REPO_NAME); \
	fi ; \
	$(call setup_kernel)   ; \
	$(call setup_ethtool)  ; \
	$(call setup_rdk)      ; \
	$(call setup_simicsfs) ; \
	$(call setup_i3ctools) ; \
	echo "MACHINE = \"$(BB_MACHINE_NAME)\"" >> conf/local.conf

.PHONY: get-rdk
get-rdk: $(TOP)/meta-intel-axxia
ifeq ($(INCLUDE_RDK),true)
ifeq ($(USE_RDK_REPO),false)
ifeq ($(INCLUDE_RDK_TOOLS),true)
	$(call check-file-exists, $(RDK_SRC_PATH)/$(RDK_TOOLS_ARCHIVE))
endif
endif
ifeq ($(RDK_MODULES_STATIC),true)
	(RDK_CFG=$(TOP)/$(RDK_REPO_NAME)/recipes-kernel/linux/frags/rdk-modules.cfg ; \
	sed -i 's/=m$$/=y/' $$RDK_CFG ; \
	cd `dirname $$RDK_CFG` ; \
	git commit -a -m"Set modules to static for $(META_AXXIA_REL)")
endif
endif

.PHONY: get-simics
get-simics: $(TOP)/meta-intel-axxia
ifeq ($(INCLUDE_SIMICSFS),true)
	$(call check-file-exists, $(SIMICS_FILE))
	(set -xe                                       ; \
         echo "Extracting Simicsfs client"             ; \
         mkdir -p $(TOP)/$(AXXIA_REPO_NAME)/downloads/ ; \
         cd $(TOP)/$(AXXIA_REPO_NAME)/downloads        ; \
         tar xz --wildcards -f $(SIMICS_FILE) ase/simics/simics*/src/misc ase/simics/simics*/linux64/bin/craff ; \
         cd ase/simics                                 ; \
         rm -fr simics-qsp*                            ; \
         SIMICS_VER=`ls -d simics*`                    ; \
         tar cfz $(TOP)/$(AXXIA_REPO_NAME)/downloads/$$SIMICS_VER.tgz $$SIMICS_VER ; \
	 cd $(TOP)/$(AXXIA_REPO_NAME)/downloads        ; \
	 echo "Created Simicsfs client $(TOP)/$(AXXIA_REPO_NAME)/downloads/$$SIMICS_VER.tgz")
endif

fs: build
	$(call bitbake, $(BB_IMAGE_TYPE))

sdk: fs
	$(call bitbake-task, $(BB_IMAGE_TYPE_SDK), populate_sdk)

.PHONY: install-sdk
install-sdk:
ifdef SDK_INSTALL_DIR
	set -e                      ; \
	cd $(BLDDIR)/tmp/deploy/sdk ; \
	mkdir -p $(SDK_INSTALL_DIR) ; \
	rm -rf $(SDK_INSTALL_DIR)/* ;\
	./$(DISTRO)-glibc-x86_64-$(BB_IMAGE_TYPE_SDK)*toolchain--$(RELEASE_VERSION).sh -y -d $(SDK_INSTALL_DIR) ; \
	cd $(SDK_INSTALL_DIR)       ; \
	unset LD_LIBRARY_PATH       ; \
	source environment-setup*   ; \
	./external-modules-setup.sh
else
	$(error SDK_INSTALL_DIR is undefined, run 'make help' for usage)
endif

.PHONY: craff_gen
craff_gen:
ifeq ($(INCLUDE_SIMICSFS),true)
	set -e ;                                    \
	cd $(BLDDIR) ;                              \
	CRAFF_BIN=$$(find tmp/work/*/simicsfs-client -name craff | grep linux64 | grep bin | head -n 1) ; \
	cd $(BLDDIR)/tmp/deploy/images/$(BB_MACHINE_NAME) ; \
	IMAGE_SUFFIX=".rootfs.wic";         \
	$(BLDDIR)/$$CRAFF_BIN -o $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME).craff $(BB_IMAGE_TYPE)-$(BB_MACHINE_NAME)--$(RELEASE_VERSION)$${IMAGE_SUFFIX}
endif

clobber:
	$(RM) -r $(BLDDIR)
	$(RM) -r $(TOP)/poky
	$(RM) -r $(TOP)/meta-*
	$(RM) -r $(TOP)/openembedded-*
	$(RM) -r $(TOP)/bitbake

clean:
	$(RM) -r $(BLDDIR)

cleansstate: build
	$(call bitbake-cleansstate, $(BB_IMAGE_TYPE))

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
	echo "$$ make fs craff_gen KERNEL_SOURCE=linux-intel-private KBRANCH_INTEL_PRIVATE=<user-branch>" ; \
	echo "    Build the linux kernel from a specific branch of the" ; \
	echo "    specified kernel tree." ; \
	echo "    Defaults to linux-korg" ; \
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
	echo "INCLUDE_SIMICSFS: Optional: Default = true:" ; \
	echo "    When set to 'true', builds with simics,"; \
	echo "    Set to 'false' builds Linux without simics"; \
	echo "INCLUDE_I3C_TOOLS: Optional : Detault = true" ; \
	echo "    When set to 'true', includes the i3c-tools" ; \
	echo "I3C_TOOLS_REPO: Optional : Default = public" ; \
	echo "    Choices are : 'private' | 'public'" ; \
	echo "    Only valid when INCLUDE_i3C_TOOLS is true" ; \
	echo "    \"private\" points to Intel's internal repository" ; \
	echo "RDK_SRC_PATH=<Location-of-RDK-Archives>"; \
	echo "    Required: when INCLUDE_RDK='true'"; \
	echo "    Default:  <current-dir>"; \
	echo "RDK_TOOLS_ARCHIVE=<Name-of-RDK-USERSPACE-TOOLS-Archive>"; \
	echo "    Required: when INCLUDE_RDK='true' and INCLUDE_RDK_TOOLS='true'"; \
	echo "    Default:  <current-dir>/rdk_user_src.tgz"; \
	echo "USE_RDK_REPO: Optional: Default = false:"; \
	echo "    When set to 'true', get RDK sources from a git repository."; \
	echo "    Depends on INCLUDE_RDK=true"; \
	echo "    Special access permissions from Intel are required to be able to fetch"; \
	echo "    RDK sources from the Intel GitHub repositories."; \
	echo "RDK_REPO=<RDK-Git-repository>:"; \
	echo "    Required: when USE_RDK_REPO='true'"; \
	echo "    Defaults to https://github.com/intel-collab/networking.wireless.ran.bts-rdk-releases"; \
	echo "    Depends on INCLUDE_RDK=true and USE_RDK_REPO=true"; \
	echo "RDK_REPO_REV=<tag/commit>:"; \
	echo "    Required: when USE_RDK_REPO='true'"; \
	echo "    Should contain the git revision (tag or commit SHA) for the RDK git repository."; \
	echo "    Depends on INCLUDE_RDK=true and USE_RDK_REPO=true"; \
	echo "RDK_MODULES_STATIC: Indicates if the RDK modules should"; \
	echo "                    be statically linked in the linux kernel"; \
	echo "    Optional: Applies only when INCLUDE_RDK='true'" ; \
	echo "    Default:  false"; \
	echo "RDK_LTTNG_ENABLE=[true|false]:"; \
	echo "    Enable building RDK with LTTNG enabled in user space" ; \
	echo "    Defaults to 'false'" ; \
	echo "    When set to true, will build RDK with LTTNG enabled" ; \
	echo "    When set to false, will also disable all LTTng for kernel" ; \
	echo "SIMICS_FILE=<Location-of-Simics-archive>: Required"; \
	echo "    Default: <current-dir>/simics-*" ; \
	echo "BLDDIR=<build dir>: Optional" ; \
	echo "    Location of the build"; \
	echo "    Defaults to <current-dir>/axxia" ; \
	echo "BB_IMAGE_TYPE=<image name>: Optional"; \
	echo "    The type of image to build"; \
	echo "    Default: axxia-image-run" ; \
	echo "    Other choices are: axxia-image-dev, axxia-image-small"; \
	echo "BB_IMAGE_TYPE_SDK=<image name>: Optional"; \
	echo "    Choose the image-type for the SDK build" ; \
	echo "    This is needed only when running \"make sdk\"" ; \
	echo "    Builds the SDK, including all the utilities" ; \
	echo "    Default: axxia-image-dev" ; \
	echo "SDK_INSTALL_DIR=<sdk-install-location>"; \
	echo "    Required when 'install-sdk' is called"; \
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
	echo "    Defaults to $(BLDDIR)/downloads" ; \
	echo "ROOTFS_EXTRA_SPACE=\"<extra-space>\": Optional: " ; \
	echo "    Add extra space in the rootfs partition from the WIC image" ;\
	echo "    Specify as an integer value optionally followed by one" ; \
	echo "    of the units" ; \
	echo "        \“k\” or \“K\” for kilobyte," ; \
	echo "        \“M\” for megabyte or" ;  \
	echo "        \“G\” for gibibyte." ; \
	echo "    The default unit if none is given is M" ; \
	echo "    to be enabled in the rootfs partition." ; \
	echo "    For instance, to add 10GB of extra space, specify:" ; \
	echo "    ROOTFS_EXTRA_SPACE=\"10G\"" ; \
	echo "    Default : 1MB"

