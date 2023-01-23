/******************************************************************************
 * SDOS  Encrypt, Decrypt and Get Info Example Application
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (C) 2003-2022 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 *
 *
 * BSD LICENSE
 *
 * Copyright (C) 2003-2022 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdbool.h>
#include <regex.h>


#include "sdos.h"

#define DEVNAME "/dev/oobmsm"

/* Supported modes/commands */
#define ENCRYPT_MODE  "encrypt"
#define DECRYPT_MODE  "decrypt"
#define GET_INFO_MODE "info"


/* Values used in DOE header */
#define DOE_VENDOR 0x8086;
#define DOE_TYPE 0xc;



static int debug_flag = 0;       /* Flag to enable debug prints */


typedef enum
{
    SDOS_ENCRYPT_MODE 	= 0x300,
    SDOS_DECRYPT_MODE 	= 0x301,
    SDOS_GET_INFO_MODE 	= 0x303,
} t_mode;



static void usage(int exit_code)
{
	printf("Usage: sdos [options] -m mode\n"
	       "--help 		-h	Print this help\n"
	       "--debug			Print extra debug information.\n"
	       "--input		-i	Input filename\n"
	       "--output	-o	Output file name\n"
	       "--ownerID	-d	ownerID (in hex, no leading 0x)\n"
	       "--iv		-v	IV (in hex, no leading 0x)\n"	
	       "--mode		-m	Mode - values: encrypt, decrypt, info\n"
	       "\n\n"
	       "Examples: \n"
	       "Encrypt:   sdos  -i input_file.bin -o encrypted_output_file.bin --iv 55555555555555555555555555555555  --ownerID aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m encrypt\n\n"
	       "Decrypt:   sdos  -i encrypted_input_file.bin -o decrypted_output_file.bin --ownerID aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -m decrypt\n\n"
	       "Info:	   sdos -m info\n\n"
	       );


	exit(exit_code);
}

static void dump_doe(const char *title, struct s_doe *doe)
{
	unsigned char *data;
	int i;

	if (debug_flag == 0)
		return;

	printf("---- %s [0x%p] ----\n", title ? title : " ", doe);
	printf("vendor=0x%x type=0x%x length=0x%x/%d payload=0x%p\n",
	       doe->vendor, doe->type, doe->length, doe->length, doe->payload);

	data = (unsigned char *)doe;

	for (i = 0; i < (doe->length*4); i++) {
		printf("0x%02x ", *data++);
	}

	printf("\n");
}



/* In:  ptr to string containig hex value, ptr to out array to store result */
static int parse_ownerID_iv(char *ptrInputString, unsigned char * ptrOutputArray)
{	
	int i,j,rc;
	unsigned char  outputValue[16];
			
			
	/* Check input is a hex number */
	regex_t regex;
	rc = regcomp(&regex, "^0(x|X)", REG_EXTENDED);
	if (rc) {
	    printf("Regex error\n");
	    exit(1);
	}
	rc = regexec(&regex,ptrInputString,0,  NULL, 0);
	if (rc == 0) {
	    printf("Input has leading 0x \n");            /* TODO Handle numbers with leading 0x */
	}	
	
	
	rc = regcomp(&regex, "^[0-9a-fA-F]+$" , REG_EXTENDED);		
	if (rc) {
	    printf("Regex error\n");
	    exit(1);
	}
	rc = regexec(&regex,ptrInputString,0,  NULL, 0);
	if (rc) {
	    printf("Input is not a hex number!\n");
	    exit(1);
	}
			
	/* Get length of input */
	unsigned int input_length = strlen(ptrInputString);
	unsigned char temp[2] = {0,0};

	if (input_length > 32) {
		printf(" Input value too long! \n");
		exit(1);
	}	
			
	for (i=0;i<16;i++) 
		outputValue[i] = 0;   /* Zero ownerID contents */
	
	j=15;
	for (i= (input_length -2); i>=0; i-=2) {
		temp[0] = *((unsigned char *)(ptrInputString +i) );
		temp[1] = *((unsigned char *)(ptrInputString  +i + 1) );
		outputValue[j] = (unsigned char) strtol(temp,0,16);
		j--;
	}
	if ( (input_length % 2) != 0) 	{  			/* If input length is an odd number, first char still needs to be processed*/
		temp[0] = '0';             			/* Prepend a 0 to first char */
		temp[1] = *((unsigned char *)ptrInputString );	/* First char */			
		outputValue[j--] = (unsigned char) strtol(temp,0,16);
	}

	for (i=0;i<16;i++) {
		*(ptrOutputArray +i) = outputValue[i];
	}	
}


unsigned int sendMessage(struct s_doe * doe, int fd_device) {
	
	unsigned long command;
	int rc;

	dump_doe("Msg data to the driver", doe);
	command = OOBMSM_IOC_MAGIC;
	rc = ioctl(fd_device, command, doe);
	if (rc == -1) {
		fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
		free(doe);
		return -1;
	}				
	dump_doe("Msg data from the driver", doe);

	return 0;		
}



void displayInfo(struct s_sdos_get_info_rsp * getInfoResp) {
	printf("SDOS  Get Info Response: \n");
	printf("Max Chunk Length: %x \n", getInfoResp->maxChunkLength);
	printf("StateInfo: \n");
	printf("	SDOS Enabled: 		%x \n",getInfoResp->stateInfo.sdosEnabled);
	printf("	Zeriozation Triggered: 	%x \n",getInfoResp->stateInfo.zeroizeTriggered );
	printf("	SDOS Active: 		%x \n",getInfoResp->stateInfo.sdosActive);
	printf("	SRK Set: 		%x \n",getInfoResp->stateInfo.srkSet);
	printf("	SDOS Committed: 	%x \n",getInfoResp->stateInfo.srkCommitted);
	printf("	EOM: 			%x \n",getInfoResp->stateInfo.eom);
	
	printf("SRK1: \n");
	printf("	Valid:		%x \n",getInfoResp->srk1State.valid);
	printf("	Revoked:	%x \n",getInfoResp->srk1State.revoked);
	
	printf("SRK2: \n");
	printf("	Valid:		%x \n",getInfoResp->srk2State.valid);
	printf("	Revoked:	%x \n",getInfoResp->srk2State.revoked);
	
	printf("SRK3: \n");
	printf("	Valid:		%x \n",getInfoResp->srk3State.valid);
	printf("	Revoked:	%x \n",getInfoResp->srk3State.revoked);
	
}




int main(int argc, char *argv[])
{
	struct s_doe *doe ;
	int i, j, c, fd_device, fd_input, fd_output, rc;

	bool inputFileValid = false;
	bool outputFileValid = false;
	bool ownerIDValid = false;
	bool ivValid = false;

	bool atEndOfData = false;
	
	t_mode mode; 	
	unsigned char  ownerID[16];
	unsigned char  iv[16];
	unsigned char  hmac[32];
	
	unsigned char  headerBuffer[8];

	unsigned int   maxChunkLength;	
	unsigned int   dataLength;
	unsigned int   nextBlockLength;
	unsigned char  paddingLength;

	while (1) {
		static struct option long_options[] = {
			{"debug", no_argument, &debug_flag, 1},
			{"help",  no_argument, 0, 'h'},
			{"input", required_argument, 0, 'i'},
			{"output", required_argument, 0, 'o'},
			{"mode", required_argument, 0, 'm'},
			{"ownerID", required_argument, 0, 'd'},
			{"iv", required_argument, 0, 'v'},			
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "i:o:m:d:v:h", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (long_options[option_index].flag != 0)
				break; /* Option set a flag, do nothing. */
			printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf(".\n");
			break;
		case 'h':
			usage(0);
			break;
		case 'i':
			if (debug_flag != 0)
				printf("Input filename: %s\n", optarg);
			fd_input = open(optarg, O_RDONLY );
			if (fd_device < 0) {
				fprintf(stderr, "open input file failed failed: %s\n", strerror(errno));
				return -1;
			}
		 	inputFileValid = true;
			break;	
		case 'o':
			if (debug_flag != 0)
				printf("Output filename: %s\n", optarg);
			fd_output = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd_output < 0) {
				fprintf(stderr, "open output file failed failed: %s\n", strerror(errno));
				return -1;
			}
			outputFileValid = true;
			break;	
		case 'm':
			if (debug_flag != 0)
				printf("Mode: %s\n", optarg);
			if (strcmp(optarg, "encrypt") == 0) {
				if (debug_flag != 0)
					printf("Encrypt mode \n");
				mode = SDOS_ENCRYPT_MODE;
			}else if (strcmp(optarg, "decrypt") == 0) {
				if (debug_flag != 0)
					printf("Decrypt mode \n");
				mode = SDOS_DECRYPT_MODE;				
			}else if (strcmp(optarg, "info") == 0) {
				if (debug_flag != 0)
					printf("Info mode \n");
				mode = SDOS_GET_INFO_MODE;			
			}
			break;	
		case 'd':
			if (debug_flag != 0)
				printf("ownerID input: %s\n", optarg);

			parse_ownerID_iv(optarg, (unsigned char *)ownerID);

			if (debug_flag != 0) {
				printf ("Owner ID value: 0x");
				for (i=0;i<16;i++) {
					printf("%02x", ownerID[i]);
				}
				printf("\n");  
			}
			ownerIDValid = true;
			break;	
		case 'v':
			if (debug_flag != 0)
				printf("IV input: %s\n", optarg);
			
			parse_ownerID_iv(optarg, (unsigned char *)iv);
			
			if (debug_flag != 0) {
				printf ("IV value: 0x");
				for (i=0;i<16;i++) {
					printf("%02x", iv[i]);
				}
				printf("\n");  
			}			
			ivValid = true;
			break;							
		default:
			fprintf(stderr, "Error parsing options!\n");
			return -1;
		}
	}

	/* Open the character device. */
	fd_device = open(DEVNAME, O_RDWR);

	if (fd_device < 0) {
		fprintf(stderr, "opening device driver failed: %s\n", strerror(errno));

		return -1;
	}

	/* Create a DOE  mailbox buffer */
	doe = malloc(sizeof(*doe));
	if (!doe) {
		fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
		return -1;
	}

	if (debug_flag != 0)
		printf("Size of DOE structure: %ld \n",sizeof(struct s_doe ));

	doe->vendor = DOE_VENDOR;
	doe->type = DOE_TYPE;

	/* SDOS ENCRYPT  */
	if (mode == SDOS_ENCRYPT_MODE) {

		/* Check required info is available */
		if (inputFileValid && outputFileValid && ivValid && ownerIDValid) {

		/* Send SDOS Get Info msg to retriev Max Chunk length */
		
			doe->vendor = DOE_VENDOR;
			doe->type = DOE_TYPE;
			doe->length = 4;
			doe->payload[0] = 0x00000002;
			doe->payload[1] = SDOS_GET_INFO_MODE;  				/* Command ID */

			/* Send message */
			if ( sendMessage(doe, fd_device) !=0) {
				printf("Sending SDOS Get Info msg failed! \n");
				return -1;
			}		

			/* Handle response here */
			if (doe->payload[1] != 0)  {   /* If Status !=0)  */
				fprintf(stderr, "SDOS Info response error: %s\n", strerror(errno)); /* TODO What are status values ?*/
				free(doe);
				return -1;
			}

			/* Map SDO Get Info response msg structure to DOE payload buffer */
			struct s_sdos_get_info_rsp * getInfoResp = (struct s_sdos_get_info_rsp *)&doe->payload[1];;

			maxChunkLength = getInfoResp->maxChunkLength;
			
			if (debug_flag != 0)			
				printf("Max Chunk Length: %d bytes\n", maxChunkLength);		

			rc = write(fd_output, "__SDOS__",8);      /*Write __SDOS__ header to output file */
			if (rc == -1) {
				fprintf(stderr, "Writing __SDOS__ to output failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}

			rc = write(fd_output, &iv,16);      /*Write IV to output file */
			if (rc == -1) {
				fprintf(stderr, "Writing IV to output failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}

			atEndOfData = false;
			do {
				/* Configure DOE header*/
				doe->vendor = DOE_VENDOR;
				doe->type = DOE_TYPE;
				doe->payload[0] = 0x00000002;  /*S3M payload */

				/* Map S3M SDOS encrypt message format to payload */
				struct s_sdos_encrypt_cmd * encrypt_cmd =  (struct s_sdos_encrypt_cmd *)&doe->payload[1];

				/* Write S3M Command ID */
				encrypt_cmd->commandID = SDOS_ENCRYPT_MODE;  				/* Command ID */


				/* Write ownerID to payload buffer */
				for(i=0; i<4;i++) {
					encrypt_cmd->ownerID[i] = * ((unsigned int *)ownerID + i);	/* ownerID */
				}

				/* Write IV to payload  buffer*/
				for(i=0; i<4;i++) {
					encrypt_cmd->iv[i] = * ((unsigned int *)iv + i);	/* IV */
				}												
				
				/* Read data into payload buffer */
				dataLength = 0;
				paddingLength =0;

				rc = read(fd_input,(unsigned char *)&encrypt_cmd->data[0], maxChunkLength); 	/* Read (up to) maxChunkLength bytes from input file */
				if (rc == -1) {
					fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}
				dataLength = rc;				

				if (debug_flag != 0)
					printf("Read %d bytes \n", dataLength);
				
				
				if (rc < maxChunkLength) {
					atEndOfData = true;
			   		if  ( (rc % 16) !=0 ) {	/* If not 16 byte aligned , add zero padding*/
				  		paddingLength = 16 - (rc %16);
				  		for (i=0;i <paddingLength; i++) {
				  			*(unsigned char *)    ( (unsigned char *)(&encrypt_cmd->data[0]) + dataLength +i) = 0;		
				  		}	
				  	}
				}
				dataLength = dataLength + paddingLength;

				if (debug_flag != 0) {
					printf("padding length: %d \n",paddingLength);
					printf("Data length after 16 byte align %d \n", dataLength);
				}			

				/* If dataLength = 0, then no more data to encrypt, exit do loop */
				if (dataLength == 0)
					break;
				
				encrypt_cmd->length = dataLength;				
		
				doe->length = (dataLength + 52)/4;						/* Length of payload in DWORDS*/
				
				/* Send message */
				if ( sendMessage(doe, fd_device) !=0) {
					printf("Sending SDOS Encrypt msg failed! \n");
					return -1;
				}		
				
				/* Map S3M SDOS encrypt response format to payload */
				struct s_sdos_encrypt_rsp * encrypt_rsp =  (struct s_sdos_encrypt_rsp *)&doe->payload[1];

				/* Handle response here */
				if (encrypt_rsp->status != 0)  {   /* If Status !=0)  */
					fprintf(stderr, "SDOS Encrypt response error\n"); /* TODO What are status values ?*/
					free(doe);
					return -1;			
				}
			
				/* Read HMAC from response */
				for (i=0; i<8;i++) {
					* ((unsigned int *)hmac + i) = encrypt_rsp->hmac[i];
				}
				
				if (debug_flag != 0) {
					printf("HMAC: 0x");
					for (i=0;i<32;i++) {
						printf("%02x",hmac[i]);
					}
					printf("\n");
				}
				
				/*Set IV for next message using HMAC from previous message */
				for (i=0;i<16;i++)
					iv[i] = hmac[16+i];	/* Using upper 16 bytes of HMAC value as IV */
				
				dataLength = encrypt_rsp->length;
				
				if (debug_flag != 0)
					printf("dataLength for encrypted data %d\n", dataLength);

				/* Store encrypted data length to output file */
				rc = write(fd_output, &dataLength,4);      /*Write encrypted data length to output file */
				if (rc == -1) {
					fprintf(stderr, "Writing encrypted data length to output failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}
				
				/* Store hmac in output file */
				rc = write(fd_output, &hmac,32);      /*Write hmac to output file */
				if (rc == -1) {
					fprintf(stderr, "Writing HMAC to output failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}

				/* Store received encypted data in output file */
				for (i=0;i<dataLength/4;i++) {
					rc = write(fd_output,&doe->payload[11+i],4);
					if (rc == -1) {
						fprintf(stderr, "write() failed: %s\n", strerror(errno));
						free(doe);
						return -1;
					}
				}
			} while (atEndOfData == false);	

			/* Store length = 0 in file */
			dataLength = 0;
			rc = write(fd_output, &dataLength,4);      /*Write data length of next block to output file */
			if (rc == -1) {
				fprintf(stderr, "Writing HMAC to output failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}

			/* Store padding length at end of file */
			rc = write(fd_output, &paddingLength,1);      /*Write padding length to output file */
			if (rc == -1) {
				fprintf(stderr, "Writing HMAC to output failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}
			
			rc = close(fd_output);
			if (rc == -1) {
				fprintf(stderr, "fd_output close() failed: %s\n", strerror(errno));
				free(doe);
			return -1;
			}
			rc = close(fd_input);
			if (rc == -1) {
				fprintf(stderr, "fd_input close() failed: %s\n", strerror(errno));
				free(doe);
			return -1;
			}
				
		}
		else {
			printf("Incorrect options for SDOS encrypt !!\n");
			free(doe);
			return -1;
		}

	}
	else if (mode == SDOS_DECRYPT_MODE) {
		struct s_sdos_decrypt_cmd * decrypt_cmd;
		struct s_sdos_decrypt_rsp * decrypt_rsp;
		
		if (inputFileValid && outputFileValid && ownerIDValid) {
		
			
			/* Check file for __SDOS__ header in input file */
			rc = read(fd_input,headerBuffer,8); 	/* Read first 8 bytes*/
			if (rc == -1) {
				fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}							
			if (rc < 8) {
				fprintf(stderr, "Failed to read SDOS header, file too small\n");
				free(doe);
				return -1;			
			}
			if (strncmp( "__SDOS__", (unsigned char *)headerBuffer, 8) != 0) {
				fprintf(stderr, "No SDOS header in input file\n");
				free(doe);
				return -1; 
			}
			
			if (debug_flag != 0)
				printf("Found __SDOS__ header \n");


			/* Read IV from input file*/
			rc = read(fd_input,iv,16); 	/* Read 16 byte IV*/
			if (rc == -1) {
				fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}							
			if (rc < 16) {
				fprintf(stderr, "Failed to read IV, file too small: %s\n", strerror(errno));
				free(doe);
				return -1;			
			}

			/* Read block length from input file */
			rc = read(fd_input,&dataLength,4); 	/* Read 4 byte data block length*/
			if (rc == -1) {
				fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}							
			if (rc < 4) {
				fprintf(stderr, "Failed to read block length, file too small: %s\n", strerror(errno));
				free(doe);
				return -1;			
			}
			
			if (debug_flag != 0)
				printf("Encrypted block length: %d \n",dataLength);
			
			if ( (dataLength % 16) != 0) {
				printf("Data block size not multiple of 16: \n");
				return -1;			
			}
			
			do {
				/* Set up SDOS_Decrypt command header */
				doe->vendor = DOE_VENDOR;
				doe->type = DOE_TYPE;
				doe->payload[0] = 0x00000002;

				/* Map S3M SDOS decrypt message format to payload */
				decrypt_cmd =  (struct s_sdos_decrypt_cmd *)&doe->payload[1];

				/* Write S3M Command ID */
				decrypt_cmd->commandID = SDOS_DECRYPT_MODE;  				/* Command ID */


				/*Write ownerID to payload buffer */
				for(i=0; i<4;i++) {
					decrypt_cmd->ownerID[i] = * ((unsigned int *)ownerID + i);	/* ownerID */
				}
				
				if (debug_flag != 0) {
					printf ("IV value: 0x");
					for (i=0;i<16;i++) {
						printf("%02x", iv[i]);
					}
					printf("\n");
				}
				
				/* Write IV to payload buffer */					
				for(i=0; i<4;i++) {
					decrypt_cmd->iv[i] = * ((unsigned int *)iv + i);	/* IV */
				}
				
				/* Read HMAC from input file*/
				rc = read(fd_input,hmac,32); 	/* Read 32 byte IV*/
				if (rc == -1) {
					fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}							
				if (rc < 32) {
					fprintf(stderr, "Failed to read HMAC, file too small: %s\n", strerror(errno));
					free(doe);
					return -1;			
				}
				
				if (debug_flag != 0) {
					printf ("HMAC value: 0x");
					for (i=0;i<32;i++) {
						printf("%02x", hmac[i]);
					}
					printf("\n");			
				}

				/* Write HMAC to payload buffer */
				for(i=0; i<8;i++) {
					decrypt_cmd->hmac[i] = * ((unsigned int *)hmac + i);	/* HMAC */
				}

				/* Write encrypted block length to payload buffer */
				decrypt_cmd->length = dataLength;				
								
				/* Read encrypted data from input file to payload buffer */
				rc = read(fd_input,&decrypt_cmd->data[0],dataLength); 	/* Read datablock*/

				if (rc == -1) {
					fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}							
				if (rc < dataLength) {
					fprintf(stderr, "Failed to read data block, file too small: %s\n", strerror(errno));
					free(doe);
					return -1;			
				}			
				
				/* Set DOE message length (in DWORDS)*/
				doe->length = dataLength/4 + 21;
				

				/* Send message */
				if ( sendMessage(doe, fd_device) !=0) {
					printf("Sending SDOS Decrypt msg failed! \n");
					return -1;
				}		
				
				/* Map S3M SDOS decrypt response format to payload */
				decrypt_rsp =  (struct s_sdos_decrypt_rsp *)&doe->payload[1];

				/* Handle response here */
				if (decrypt_rsp->status != 0)  {   /* If Status !=0)  */
					fprintf(stderr, "SDOS Decrypt response error\n"); /* TODO What are status values ?*/
					free(doe);
					return -1;			
				}

				/* Read decrypted data length from response */

				dataLength = decrypt_rsp->length;

				if (debug_flag != 0)
					printf("Decrypted data length %d \n",dataLength);
				
				/* Read next encrypted data length value from file */
				rc = read(fd_input,&nextBlockLength,4); 	/* Read 4 byte data block length*/
				if (rc == -1) {
					fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
					free(doe);
					return -1;
				}							
				if (rc < 4) {
					fprintf(stderr, "Failed to read block length, file too small: %s\n", strerror(errno));
					free(doe);
					return -1;			
				}
				
				if (debug_flag != 0)
					printf("Next block length: %d \n",nextBlockLength);
				
				if ( (nextBlockLength % 16) != 0) {
					fprintf(stderr, "Next block size not multiple of 16\n");
					free(doe);
					return -1;			
				}			
			
				if (nextBlockLength !=0)  {
					/* Write previous block of data to output file */

					rc = write(fd_output,&decrypt_rsp->data[0],dataLength);

					if (rc == -1) {
						fprintf(stderr, "Writing decrypted data to output failed: %s\n", strerror(errno));
						free(doe);
						return -1;
					}
					/* Get next IV (lowest 16 bytes of previous block's HMAC) t*/
					for (i=0;i<16;i++) {
						iv[i] = hmac[16+i];
					}
					
					dataLength = nextBlockLength;				
				}
			} while (nextBlockLength != 0);
			

			/* Last length read was zero */
			/* Read padding byte, remove any padding from end of previous decrypted data */
			rc = read(fd_input,&paddingLength,1); 	/* Read 1 byte padding length*/
			if (rc == -1) {
				fprintf(stderr, "Reading from input file failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}							
			if (rc < 1) {
				fprintf(stderr, "Failed to read padding length, file too small: %s\n", strerror(errno));
				free(doe);
				return -1;			
			}
			
			if (debug_flag != 0)
				printf("Padding length: %d \n",paddingLength);			
			if (paddingLength !=0) {
				dataLength = dataLength - paddingLength;  /* Subtract padding bytes from data block */
			}
			/* Write last block of data (minus any padding to output file */

			rc = write(fd_output,&decrypt_rsp->data[0],dataLength);
				
			if (rc == -1) {
				fprintf(stderr, "Writing decrypted data to output failed: %s\n", strerror(errno));
				free(doe);
				return -1;
			}
			
		}
		else {
			printf("Incorrect options for SDOS decrypt !!\n");
			free(doe);
			return -1;
		}	
	}
	else if (mode == SDOS_GET_INFO_MODE)  {
		doe->vendor = DOE_VENDOR;
		doe->type = DOE_TYPE;
		doe->length = 4;
		doe->payload[0] = 0x00000002;
		doe->payload[1] = SDOS_GET_INFO_MODE;  				/* Command ID */


		/* Send message */
		if ( sendMessage(doe, fd_device) !=0) {
			printf("Sending SDOS Get Info msg failed! \n");
			return -1;
		}		

		/* Handle response here */
		if (doe->payload[1] != 0)  {   /* If Status !=0)  */
			fprintf(stderr, "SDOS Info response error\n"); /* TODO What are status values ?*/
			free(doe);
			return -1;			
		}
		displayInfo((struct s_sdos_get_info_rsp *)&doe->payload[1]);

	}

	else {
		printf("Incorrect mode !!\n");
		free(doe);
		return -1;
	}

	free(doe);	
	printf("Done \n");
	return 0;
}


