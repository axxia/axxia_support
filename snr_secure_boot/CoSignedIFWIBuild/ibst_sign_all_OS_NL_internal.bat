
@REM Script to run IBST to sign ME Region, BIOS Region and OS/Netloader Related Images
@REM Sign  RBE, BUP, PMC Patch in ME Region, inserting manifests for each.  Creates ME Region Key Manifest
@REM Creates manifests  for ACM and FIT Patch.
@REM Create OS and Netloader Manifests
@REM Create bootx64.efi manifest
@REM Create testImage manifest

@REM INTEL CONFIDENTIAL
@REM Copyright (c) 2016 - 2019, Intel Corporation. <BR>
@REM
@REM The source code contained or described herein and all documents related to the
@REM source code ("Material") are owned by Intel Corporation or its suppliers or
@REM licensors. Title to the Material remains with Intel Corporation or its suppliers
@REM and licensors. The Material may contain trade secrets and proprietary    and
@REM confidential information of Intel Corporation and its suppliers and licensors,
@REM and is protected by worldwide copyright and trade secret laws and treaty
@REM provisions. No part of the Material may be used, copied, reproduced, modified,
@REM published, uploaded, posted, transmitted, distributed, or disclosed in any way
@REM without Intel's prior express written permission.
@REM
@REM No license under any patent, copyright, trade secret or other intellectual
@REM property right is granted to or conferred upon you by disclosure or delivery
@REM of the Materials, either expressly, by implication, inducement, estoppel or
@REM otherwise. Any license under such intellectual property rights must be
@REM express and approved by Intel in writing.
@REM
@REM Unless otherwise agreed by Intel in writing, you may not remove or alter
@REM this notice or any other notice embedded in Materials by Intel or
@REM Intel's suppliers or licensors in any way.
@REM



echo off

::Path to Python install
set PY_PATH=c:\Python36

::Path to IBST installation
set IBST_PATH=C:\share\CoSignedIFWIBuild\Tools\SPS_SoC-A_05.00.01.030.0\Tools\IbstTool

::Path to IBST override files
set OVERRIDE_PATH=c:\share\CoSignedIFWIBuild\Overrides

::Path to folder containing keys
set KEY_PATH=c:\share\CoSignedIFWIBuild\Keys

::Path to folder to store output
set OUTPUT_PATH=c:\share\CoSignedIFWIBuild\Output

::Path to folder containing ME Region.bin file
set ME_REGION_PATH=C:\share\CoSignedIFWIBuild\Tools\SPS_SoC-A_05.00.01.030.0\

::Path to PMC PATCH (pmcp.bin)
set PMCP_PATH=C:\share\CoSignedIFWIBuild\Tools\SPS_SoC-A_05.00.01.030.0\Tools\FlashImageTool

::Path to OS image
set OS_PATH=c:\share\CoSignedIFWIBuild\Output


pushd  %IBST_PATH%


echo =====================IBST  Test Script ===================================
echo 	@@@@@@@@@@@@@@@@@  Signing  ME Region  Firmware @@@@@@@@@@@@@@@@@@@@@@@
echo 		################# Signing RBE and BUP  in MERegion.bin #################

%PY_PATH%/python.exe %IBST_PATH%/ibst.py  config/CoSign.xml --config_override %OVERRIDE_PATH%/CoSign_override_RBE_BUP_internal.xml -s cosign_key="%KEY_PATH%/RBE_BUP_3K_PRIVATE.pem"  input_file="%ME_REGION_PATH%/MERegion.bin" output_name="%OUTPUT_PATH%/MERegion_Cosigned_internal.bin"

echo 		################# RBE and BUP Done #################

echo 		################# Signing PMC Patch #################

%PY_PATH%/python.exe %IBST_PATH%/ibst.py  config/CoSignPmcp.xml --config_override %OVERRIDE_PATH%/CoSign_override_PMCP_With_Footer_internal.xml  -s cosign_key="%KEY_PATH%/PMCP_3K_PRIVATE.pem"   input_file="%PMCP_PATH%/SNR.02.01.1008_pmcp.bin" output_name="%OUTPUT_PATH%/PMCP_Cosigned_internal.bin"

echo 		################# PMCP Done #################

echo 		################# Signing ME Region Key manifest #################

%PY_PATH%/python.exe %IBST_PATH%/ibst.py  config/OemKeyManifest.xml --skip_valid --config_override %OVERRIDE_PATH%/OemKeyManifest_override_all_keys_os_internal.xml ^
-s key="%KEY_PATH%/KAK0_3K_PRIVATE.pem"   ^
 rbe_bup_key="%KEY_PATH%/RBE_BUP_3K_PRIVATE.pem" ^
 pmc_key="%KEY_PATH%/PMCP_3K_PRIVATE.pem"   ^
 oem_debug_key="%KEY_PATH%/3k_test_key_private.pem" ^
 idlm_key="%KEY_PATH%/3k_test_key_private.pem" ^
 os_key="%KEY_PATH%/OS_3K_PRIVATE.pem" ^
 netl_key="%KEY_PATH%/NL_3K_PRIVATE.pem" ^
 fd0v_key="%KEY_PATH%/3k_test_key_private.pem" ^
 output_name="%OUTPUT_PATH%/ME_KM_internal.bin" 

echo 		###############  ME Region Key Manifest Done ####################


echo 	@@@@@@@@@@@@@  ME Region Firmware Steps Done @@@@@@@@@@@@@@@@@@@@
echo.

echo 	@@@@@@@@@@@ Signing BIOS Region Images  @@@@@@@@@@@@@@@@@@@@@@

echo 		################# Signing ACM #################

%PY_PATH%/python ./ibst.py  config/CoSigningManifest.xml --config_override %OVERRIDE_PATH%/CoSigningManifest_override_ACM_internal.xml  -s key="%KEY_PATH%/ACM_3K_PRIVATE.pem" binary_hash="d78e740b891915c01bba156e0be5bfc9b51fcb7ecc1f927bcf78d21af38deda1" output_name="%OUTPUT_PATH%/ACMM_internal.bin" 

echo 		################# ACM Done #################

echo 		################# Signing FIT Patch #################

%PY_PATH%/python ./ibst.py  config/CoSigningManifest.xml --config_override %OVERRIDE_PATH%/CoSigningManifest_override_FITP_internal.xml  -s key="%KEY_PATH%/FITP_3K_PRIVATE.pem" binary_hash="7d0429d3782677596aea360ab964ca6be3a260231c5fe16b71799d78a41f8cfd"  output_name="%OUTPUT_PATH%/FPM_internal.bin"

echo 		################# FIT Patch Done #################

echo  	@@@@@@@@@@@@@@ BIOS Images Done  @@@@@@@@@@@@@@@@@


echo 		################# Signing OS Kernel #################
%PY_PATH%/python ./ibst.py  config/CoSigningManifest.xml --config_override %OVERRIDE_PATH%/CoSigningManifest_override_OS_internal.xml  -s key="%KEY_PATH%/OS_3K_PRIVATE.pem"  module_bin="%OS_PATH%\vmlinuz_4.19" output_name="%OUTPUT_PATH%/vmlinuz_4.19.bin"                 


echo 		################# Signing bootx64.efi #################
%PY_PATH%/python ./ibst.py  config/CoSigningManifest.xml --config_override %OVERRIDE_PATH%/CoSigningManifest_override_NL_internal.xml  -s key="%KEY_PATH%/NL_3K_PRIVATE.pem"  module_bin="%OS_PATH%\bootx64.efi" output_name="%OUTPUT_PATH%/bootx64.efi.bin"                 

echo 		################# Signing testImage #################
%PY_PATH%/python ./ibst.py  config/CoSigningManifest.xml --config_override %OVERRIDE_PATH%/CoSigningManifest_override_NL_internal.xml  -s key="%KEY_PATH%/NL_3K_PRIVATE.pem"  module_bin="%OS_PATH%\testImage" output_name="%OUTPUT_PATH%/testImage.bin"  


popd 