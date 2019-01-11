#!/usr/bin/env python3
#sign_rev.py - external signing script used with IBST, reverses byte order of result

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

#Get total numbers of args passed
total = len(sys.argv)
#Get the arguments list
cmdargs = str(sys.argv)
#Print args
print ("Total args: %d " % total)
print ("Args list: %s " % cmdargs)

datatosign = str(sys.argv[4])
print ("\n\nData to sign: %s " % datatosign )

outputfile = str(sys.argv[3])
print ("\n\nSig output file: %s " % outputfile )
print ("\n\n")

privkey = 'c:/Users/jlogan/Documents/SNR/SPS/SigningExamples/Keys/NL_3K_PRIVATE.pem'

#Call openssl to sign the data in datatosign
print ("Calling openssl")
signmanifest = subprocess.call( ['c:/Program Files/Git/mingw64/bin/openssl.exe ', 'dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32', '-sign', privkey, '-out', outputfile, datatosign]  )

f= open(outputfile,"rb")
contents= f.read()
print ("Original contents")
print (contents)
print (len(contents))
f.close()

rev_contents= contents[::-1]
print ("Reversed contents")
print (rev_contents)
print (len(rev_contents))

f= open(outputfile, "wb")
f.write(rev_contents)
f.close()

#print return code - could check this to see if openssl worked
print signmanifest
print ("Finished openssl")
