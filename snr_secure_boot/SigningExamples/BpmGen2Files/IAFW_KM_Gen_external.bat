
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

::set BPMGEN2_Tool="c:\share\bpmgen2release2018-05-16 (1)\BpmGen2.exe"

set BPMGEN2_Tool=c:\share\bios\FDbin\Tools\BpmGen\BpmGen2.exe


set DATAFILE=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\BpmGen2Files\datatosign.dat
set SIGNINGSCRIPT=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\BpmGen2Files\Sign_IAFW_KM_Ext.bat
set SIGFILE=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\BpmGen2Files\IAFW_KM_SigFile.sig

::Delete files from any previous runs
del %DATAFILE%
del %SIGFILE%

::Run bpmgen2 to generate SIGFILE
%BPMGEN2_Tool% KMGEN ^
	-XSIGN   %DATAFILE%  %SIGNINGSCRIPT%  %SIGFILE%    ^
	-SIGNPUBKEY ..\Keys\KAK0_3K_PUBLIC.pem ^
	-KEY ..\Keys\BPM_3K_PUBLIC.pem SHA256 BPM ^
	-KEY ..\Keys\FITP_3K_PUBLIC.pem SHA256 FPM ^
	-KEY ..\Keys\ACM_3K_PUBLIC.pem SHA256 ACMM  ^
	-KM IAFW_KM_Gen_external.bin  ^
	-KmRevision 1  ^
	-SVN 1  ^
	-KMID 1  ^
	-SCHEME RSAPSS  ^
	-KMKHASH SHA512  
	    
::Call external batch file
%SIGNINGSCRIPT%

::Call bpmgen2 second time to build KM
%BPMGEN2_Tool% KMGEN ^
	-XSIGN   %DATAFILE%  %SIGNINGSCRIPT%  %SIGFILE%    ^
	-SIGNPUBKEY ..\Keys\KAK0_3K_PUBLIC.pem ^
	-KEY ..\Keys\BPM_3K_PUBLIC.pem SHA256 BPM ^
	-KEY ..\Keys\FITP_3K_PUBLIC.pem SHA256 FPM ^
	-KEY ..\Keys\ACM_3K_PUBLIC.pem SHA256 ACMM  ^
	-KM IAFW_KM_Gen_external.bin  ^
	-KmRevision 1  ^
	-SVN 1  ^
	-KMID 1  ^
	-SCHEME RSAPSS  ^
	-KMKHASH SHA512  

