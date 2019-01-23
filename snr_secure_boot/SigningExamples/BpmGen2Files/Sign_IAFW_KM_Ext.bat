
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


set OPENSSL=C:\OpenSSL-Win64\bin\openssl.exe
set PRIVKEY=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\Keys\KAK0_3K_PRIVATE.pem
set INPUTFILE=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\BpmGen2Files\datatosign.dat
set OUTPUTFILE=C:\Users\jlogan\Documents\SNR\SPS\SigningExamples\BpmGen2Files\IAFW_KM_SigFile.sig


%OPENSSL% dgst -sha256 -sigopt rsa_padding_mode:pss -sigopt rsa_pss_saltlen:32 -sign  %PRIVKEY%  -out %OUTPUTFILE%  %INPUTFILE%

::Uncomment below line to cause script to stop and wait for user to press return key (useful for debug)
::pause