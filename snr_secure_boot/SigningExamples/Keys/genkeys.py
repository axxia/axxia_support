#!/usr/bin/env python3
#genkeys.py     Script to create all keys and key hashes used for Signing Examples
#Version 0.2  16/01/19

"""
INTEL CONFIDENTIAL
Copyright 2017-2019 Intel Corporation.
This software and the related documents are Intel copyrighted materials, and
your use of them is governed by the express license under which they were
provided to you (License).Unless the License provides otherwise, you may not
use, modify, copy, publish, distribute, disclose or transmit this software or
the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.
"""



import sys
import subprocess



#openssl_call = 'c:/Program Files/Git/mingw64/bin/openssl.exe'
openssl_call = 'openssl'

#All keys except debug token header are RSA 3K PSS. Debug token header is RSA 2K pkcs1_5
key_size = '3072'
debug_token_header_key_size = '2048'


kak0_private_key_name = 'KAK0_3K_PRIVATE.pem'
kak0_public_key_name = 'KAK0_3K_PUBLIC.pem'
kak1_private_key_name = 'KAK1_3K_PRIVATE.pem'
kak1_public_key_name = 'KAK1_3K_PUBLIC.pem'
kak2_private_key_name = 'KAK2_3K_PRIVATE.pem'
kak2_public_key_name = 'KAK2_3K_PUBLIC.pem'

rbe_bup_private_key_name = 'RBE_BUP_3K_PRIVATE.pem'
rbe_bup_public_key_name = 'RBE_BUP_3K_PUBLIC.pem'

pmcp_private_key_name = 'PMCP_3K_PRIVATE.pem'
pmcp_public_key_name = 'PMCP_3K_PUBLIC.pem'

idlm_private_key_name = 'IDLM_3K_PRIVATE.pem'
idlm_public_key_name = 'IDLM_3K_PUBLIC.pem'

debug_token_header_private_key_name = 'DEBUG_TOKEN_HEADER_2K_PRIVATE.pem'
debug_token_header_public_key_name = 'DEBUG_TOKEN_HEADER_2K_PUBLIC.pem'

debug_token_footer_private_key_name = 'DEBUG_TOKEN_FOOTER_3K_PRIVATE.pem'
debug_token_footer_public_key_name = 'DEBUG_TOKEN_FOOTER_3K_PUBLIC.pem'

nl_private_key_name = 'NL_3K_PRIVATE.pem'
nl_public_key_name = 'NL_3K_PUBLIC.pem'

os_private_key_name = 'OS_3K_PRIVATE.pem'
os_public_key_name = 'OS_3K_PUBLIC.pem'

bpm_private_key_name = 'BPM_3K_PRIVATE.pem'
bpm_public_key_name = 'BPM_3K_PUBLIC.pem'

acm_private_key_name = 'ACM_3K_PRIVATE.pem'
acm_public_key_name = 'ACM_3K_PUBLIC.pem'

fitp_private_key_name = 'FITP_3K_PRIVATE.pem'
fitp_public_key_name = 'FITP_3K_PUBLIC.pem'





def create_keypair(private_key_name, public_key_name, key_size):
	#Call openssl to create  private key 
	ret = subprocess.call( [openssl_call, 'genrsa', '-f4', '-out', private_key_name, key_size] )
	#Call openssl to extract public key
	ret = subprocess.call( [openssl_call, 'rsa', '-in', private_key_name, '-outform', 'PEM', '-pubout', '-out', public_key_name] )


create_keypair(kak0_private_key_name, kak0_public_key_name, key_size)
create_keypair(kak1_private_key_name, kak1_public_key_name, key_size)
create_keypair(kak2_private_key_name, kak2_public_key_name, key_size)

create_keypair(rbe_bup_private_key_name, rbe_bup_public_key_name, key_size)
create_keypair(pmcp_private_key_name, pmcp_public_key_name, key_size)
create_keypair(idlm_private_key_name, idlm_public_key_name, key_size)

create_keypair(debug_token_header_private_key_name, debug_token_header_public_key_name, debug_token_header_key_size)
create_keypair(debug_token_footer_private_key_name, debug_token_footer_public_key_name, key_size)

create_keypair(nl_private_key_name, nl_public_key_name, key_size)
create_keypair(os_private_key_name, os_public_key_name, key_size)
create_keypair(bpm_private_key_name, bpm_public_key_name, key_size)
create_keypair(acm_private_key_name, acm_public_key_name, key_size)
create_keypair(fitp_private_key_name, fitp_public_key_name, key_size)




