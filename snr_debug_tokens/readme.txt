This folder contains 3 source code files for Linux command line programs for LVL/SNR debug token usage:

getpartid.c   		- A Linux OS command line program to read the PartID and nonce values from SoC
delete_debug_token.c	- A Linux OS command line program to delete any previous OEM Debug Tokens present in the UTOK partition in the flash memory
write_debug_token.c 	- A Linux OS command line program to write an OEM Debug Token to the UTOK partition in flash memory


To compile the programs:

gcc -o ./getpartID ./getpartID.c
gcc -o ./delete_debug_token ./delete_debug_token.c
gcc -o ./write_debug_token ./write_debug_token.c

For further information, please refer to APplication Note 727118 - Jacbosville BTS - Creating OEM Debug Tokens

