
@REM INTEL CONFIDENTIAL
@REM Copyright (c) 2016 - 2018, Intel Corporation. <BR>
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

::set BPMGEN2_Tool="c:\share\bpmgen2release2018-05-16 (1)\BpmGen2.exe"
set BPMGEN2_Tool=c:\share\bios\FDbin\Tools\BpmGen\BpmGen2.exe


%BPMGEN2_Tool% GEN ^
	..\Images\JBVLCRB.86B.WR.64.2019.01.7.07.1708_0008.D04_P90000_SNR_CDF_Svp_Nic_mode.bin ^
	BPM_external.bpDef ^
	-BPM BPM_external.bin ^
	-U ..\Output\BiosOutput.bin ^
	-KM IAFW_KM_external.bin ^
	-V:2
	
	