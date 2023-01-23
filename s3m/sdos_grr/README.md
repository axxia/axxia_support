# sdos_grr
GRR SDOS Example App For Linux

Compile with: gcc -o ./sdos  ./sdos.c


Type './sdos -h' to display help text:



./sdos  -h

Usage: sdos [options] -m mode

--help          -h      Print this help

--debug                 Print extra debug information.

--input         -i      Input filename

--output        -o      Output file name

--ownerID       -d      ownerID (in hex, no leading 0x)

--iv            -v      IV (in hex, no leading 0x)

--mode          -m      Mode - values: encrypt, decrypt, info



Examples:
 
Encrypt:   sdos  -i input_file.bin -o encrypted_output_file.bin --iv 55555555555555555555555555555555  --ownerID aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m encrypt


Decrypt:   sdos  -i encrypted_input_file.bin -o decrypted_output_file.bin  --ownerID aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m decrypt


Info:      sdos -m info

