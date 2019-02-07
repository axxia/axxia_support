/******************************************************************************
 * IntelME Interface Example Application
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (C) 2003-2018 Intel Corporation. All rights reserved.
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
 * Copyright (C) 2003-2018 Intel Corporation. All rights reserved.
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





/* Simple test program to send messages to ME */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <string.h>

#include <linux/mei.h>


#define mei_msg(_me, fmt, ARGS...) do {         \
	if (_me->verbose)                       \
		fprintf(stderr, fmt, ##ARGS);	\
} while (0)

#define mei_err(_me, fmt, ARGS...) do {         \
	fprintf(stderr, "Error: " fmt, ##ARGS); \
} while (0)



/* Struct containing info on mei client connection */
struct mei {
    uuid_le guid;
    bool initialized;
    bool verbose;
    unsigned int buf_size;
    unsigned char prot_ver;
    int fd;
};


/* MKHI header */
typedef struct {
    uint32_t GroupId :8;
    uint32_t Command :7;
    uint32_t IsResponse :1;
    uint32_t Reserved :8;
    uint32_t Result :8;
} MKHI_MESSAGE_HEADER;


/* MKHI commands */
MKHI_MESSAGE_HEADER  GEN_GET_FW_VERSION = { .GroupId = 0xff, .Command = 0x2, .IsResponse =0, .Reserved =0, .Result=0 };


#define READ_FILE_MAX_PATH_LENGTH  64
#define READ_FILE_MAX_DATA_SIZE    128    /* This can be up to 4096, using 128, since fuses are much shorter than 4096 */

typedef struct
{
    MKHI_MESSAGE_HEADER                	 Header;
    uint8_t                              FileName[READ_FILE_MAX_PATH_LENGTH];
    uint32_t                             Offset;
    uint32_t				 Size;
    unsigned char                        Flags;
}  ME_READ_FILE_Request;

/*
typedef struct
{
    MKHI_MESSAGE_HEADER                Header;
    uint8_t                            Result;
    uint32_t			       Size;
    uint8_t			       Data[READ_FILE_MAX_DATA_SIZE];
}  ME_READ_FILE_Response;
*/
typedef struct
{
    MKHI_MESSAGE_HEADER                Header;
    uint32_t			       Size;
    uint8_t			       Data[READ_FILE_MAX_DATA_SIZE];
}  ME_READ_FILE_Response;



typedef struct
{
    MKHI_MESSAGE_HEADER                Header;
    uint16_t                             CodeMinor;
    uint16_t                             CodeMajor;
    uint16_t                             CodeBuild;
    uint16_t                             CodeHotfix;
    uint16_t                             NFTPMinor;
    uint16_t                             NFTPMajor;
    uint16_t                             NFTPBuild;
    uint16_t                             NFTPHotfix;
}  GEN_GET_FW_VERSION_Response;




/* Struct containing info on MKHI connection */
struct mkhi_host_if {
    struct mei mei_cl;
    unsigned long send_timeout;
    bool initialized;
};


/*********** Fuse name array  **************/
typedef struct 
{
	char FuseName[32];  /* Path to fuse file in ME*/
	int  FuseSize;      /* Size of fuse field (bits)  */
} FUSE_INFO_t;


FUSE_INFO_t  FuseList[] = {
	
	{"FtpmEnabled",	1},
	{"SocCfgLock",	1},
	{"SBValid",	1},
	{"VendorDefId",	16},
	{"OemCred",	256},
	{"SbPolicies",	16},
	{"Enf0",	1},
	{"Enf1",	1},
	{"OemKp",	1},
	{"TxtEnabled",	1},
	{"FDVEnabled",	1},
	{"DbgDisabled",	1},
	{"PTimeStOff",	15},
	{"PTimeStCntr",	240},
	{"PchCosignEn",	1},
	{"CpuCosignEn",	1},
	{"OemRHashEn",	1},
	{"OemHashKey",	1},
	{"OemDAuthEn",	1},
	{"KeySplitEn",	1},
	{"ArbSvnEn",	1},
	{"ArbMSvn",	15},
	{"VlnEn",	1},
	{"SbAcmSvn",	15},
	{"SbKmSvn",	15},
	{"SbBsmmSvn",	40},
	{"PmcMSvn",	15},
	{"IdlmMSvn",	15},
	{"RbeMSvn",	32},
	{"UcodeMSvn",	15},
	{"OemKmSvn",	40},
	{"NwldMSvn",	40},
	{"OsMSvn",	40},
	{"CpuFwKmSvn",	15},
	{"AcmSvnEn",	1},
	{"KmSvnEn",	1},
	{"BsmmSvnEn",	1},
	{"PmcSvnEn",	1},
	{"IdlmSvnEn",	1},
	{"RbeSvnEn",	1},
	{"UcodeSvnEn",	1},
	{"OemKmSvnEn",	1},
	{"NwldSvnEn",	1},
	{"OsSvnEn",	1},
	{"CpuFwSvnEn",	1},
	{"OemKH0_s",	1},
	{"OemRsa0_s",	1},
	{"OemKH0_r",	1},
	{"OemKH0",	512},
	{"OemKH1_s",	1},
	{"OemRsa1_s",	1},
	{"OemKH1_r",	1},
	{"OemKH1",	512},
	{"OemKH2_s",	1},
	{"OemRsa2_s",	1},
	{"OemKH2_r",	1},
	{"OemKH2",	512},
	{"StorageKey",	256}
};



static void mei_deinit(struct mei *cl) {
    if (cl->fd != -1)
        close(cl->fd);
    cl->fd = -1;
    cl->buf_size = 0;
    cl->prot_ver = 0;
    cl->initialized = false;
}





static bool mei_init(struct mei *me, const uuid_le *guid,
        unsigned char req_protocol_version, bool verbose) {
    int result;
    struct mei_client *cl;
    struct mei_connect_client_data data;

    me->verbose = verbose;
    me->fd = open("/dev/mei0", O_RDWR);
    
    if (me->fd == -1) {
        if (!geteuid()) {
            mei_err(me, "Cannot establish a handle to the Intel(R) MEI driver. \n");
            exit(-1);
        } else {
            mei_err(me, "Please run this program with root privilege.\n");
            mei_deinit(me);
            exit(-1);
        }
        goto err;
    }
    memcpy(&me->guid, guid, sizeof(*guid));
    memset(&data, 0, sizeof(data));
    me->initialized = true;

    memcpy(&data.in_client_uuid, &me->guid, sizeof(me->guid));
    result = ioctl(me->fd, IOCTL_MEI_CONNECT_CLIENT, &data);
    if (result) 
    {
        mei_err(me, "IOCTL_MEI_CONNECT_CLIENT receive message. err=%d\n",result);
        goto err;
    }
    cl = &data.out_client_properties;
    mei_msg(me, "max_message_length %d\n", cl->max_msg_length);
    mei_msg(me, "protocol_version %d\n", cl->protocol_version);

    if ((req_protocol_version > 0)
            && (cl->protocol_version != req_protocol_version)) {
        mei_err(me, "Intel(R) MEI protocol version not supported\n");
        goto err;
    }

    me->buf_size = cl->max_msg_length;
    me->prot_ver = cl->protocol_version;

    return true;
    err: mei_deinit(me);
    return false;
}


static ssize_t mei_recv_msg(struct mei *me, unsigned char *buffer, ssize_t len,
        unsigned long timeout) {
    ssize_t rc;

    mei_msg(me, "call read length = %zd\n", len);

    rc = read(me->fd, buffer, len);
    if (rc < 0) {
        mei_err(me, "read failed with status %zd %s\n", rc, strerror(errno));
        mei_deinit(me);
    } else {
        mei_msg(me, "read succeeded with result %zd\n", rc);
    }
    return rc;
}


static ssize_t mei_send_msg(struct mei *me, const unsigned char *buffer,
        ssize_t len, unsigned long timeout) {
    struct timeval tv;
    ssize_t written;
    ssize_t rc;
    fd_set set;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000000;

    mei_msg(me, "call write length = %zd\n", len);

    written = write(me->fd, buffer, len);
    if (written < 0) {
        rc = -errno;
        mei_err(me, "write failed with status %zd %s\n", written,
                strerror(errno));
        goto out;
    }

    FD_ZERO(&set);
    FD_SET(me->fd, &set);
    rc = select(me->fd + 1, &set, NULL, NULL, &tv);
    if (rc > 0 && FD_ISSET(me->fd, &set)) {
        mei_msg(me, "write success\n");
    } else if (rc == 0) {
        mei_err(me, "write failed on timeout with status\n");
        goto out;
    } else { /* rc < 0 */
        mei_err(me, "write failed on select with status %zd\n", rc);
        goto out;
    }

    rc = written;
    out: if (rc < 0)
        mei_deinit(me);

    return rc;
}



const uuid_le MEI_MKHI_HIF_FIX = UUID_LE(0x55213584, 0x9a29, 0x4916, 0xba, 0xdf, 0xf, 
    0xb7, 0xed, 0x68, 0x2a, 0xeb);

static bool mkhi_host_if_fix_init(struct mkhi_host_if *acmd,
        			  unsigned long send_timeout, 
				  bool verbose) 
{
    acmd->send_timeout = (send_timeout) ? send_timeout : 20000;
    acmd->initialized = mei_init(&acmd->mei_cl, &MEI_MKHI_HIF_FIX, 0, verbose);
    return acmd->initialized;
}



int main(int argc, char **argv) 
{
	unsigned char i;

	/* Enable fixed address interface */
	FILE *fp = fopen("/sys/kernel/debug/mei0/allow_fixed_address", "w");
	int ret = (fwrite("Y", sizeof(char), 1, fp) == 1) ? 0 : -1;    
	fclose(fp);


    	struct mkhi_host_if mkhi_cmd;


        /* Open MKHI comms with mei driver */
    	bool heci_init_call = mkhi_host_if_fix_init(&mkhi_cmd, 5000, false);
       	if (!heci_init_call) 
	{
    		printf( "MKHI fixed i/f failed to initialise \n");
       		mei_deinit(&mkhi_cmd.mei_cl);
       		return false;
      	} else 
	{
    		printf( "MKHI fixed i/f initialised \n");
        }         

 	/* Send simple MKHI  command to get f/w version number */
	GEN_GET_FW_VERSION_Response resp;

	mei_send_msg(&mkhi_cmd.mei_cl,  (const unsigned char *) &GEN_GET_FW_VERSION, sizeof(GEN_GET_FW_VERSION),5000);

	mei_recv_msg(&mkhi_cmd.mei_cl, (unsigned char *)&resp , sizeof(GEN_GET_FW_VERSION_Response),
        5000);


	
	printf("Build Maj Min Hotfix : %x, %x, %x, %x \n", resp.CodeBuild, resp.CodeMajor, resp.CodeMinor, resp.CodeHotfix);





	/* Read all fuses listed in FuseList array. Check if response is OK, print result */
	

	ME_READ_FILE_Request   readRequest;  /* Buffer to store reqtesr message*/
	ME_READ_FILE_Response  readResp;     /* Buffer to store response */

	char FusePath[64]; 
	char BasePath[] = "/fpf/intel/";

	int LoopCtr;
	int NumFuses = sizeof(FuseList)/sizeof(FUSE_INFO_t) ;
	
 	printf("Number of fuse fields in list: %d \n", NumFuses);

	for (LoopCtr=0; LoopCtr < NumFuses; LoopCtr++)
	{
	
	       /* Set up ME READ FUSE request header*/
	       readRequest.Header.GroupId = 0x0a;
	       readRequest.Header.Command = 0x2;
	       readRequest.Header.IsResponse = 0;
	       readRequest.Header.Reserved = 0;
	       readRequest.Header.Result = 0;
	       
		/* Create path to fuse file (BasePath+FuseName)*/
		strcpy(FusePath, BasePath);
		strcat(FusePath, FuseList[LoopCtr].FuseName);		

		memset(readRequest.FileName, 0, sizeof(readRequest.FileName) );
		strcpy(readRequest.FileName, FusePath);       

	       readRequest.Offset = 0;
	       
		/* Calculate number of bytes to read */
		if ( (FuseList[LoopCtr].FuseSize % 8) != 0)
		{
			readRequest.Size =   (FuseList[LoopCtr].FuseSize / 8) + 1; 
		}
		else
		{
			readRequest.Size =   (FuseList[LoopCtr].FuseSize / 8);  
		}

	       readRequest.Flags = 0x0;


		//printf("ME Read Fuse Request: \n");	

		printf(" Reading fuse file:  %s .", readRequest.FileName);
		printf(" Read size: %d byte(s).", readRequest.Size);



/*
		printf("\n");
		for (i = 0; i< sizeof(readRequest); i++)
		{
		 	printf("  %x ",  *((unsigned char *)&readRequest + i) );
		}
		printf("\n");
*/

		/* Send request, read response */
		mei_send_msg(&mkhi_cmd.mei_cl,  (const unsigned char *) &readRequest,77 ,5000);

		mei_recv_msg(&mkhi_cmd.mei_cl, (unsigned char *)&readResp , sizeof(ME_READ_FILE_Response),
		5000);
	
		/* Check if response OK */
		if (readResp.Header.Result !=0)
		{
			printf("Error!  Response result = 0x%x \n", readResp.Header.Result);
		}
		else
		{
			printf("Response data value:  0x");
			for (i =0; i< readResp.Size; i++)
				printf("%02x", readResp.Data[i]);
			printf("\n");
		}

						

/*
		printf("\n");
		printf("Size of response: %ld \n", sizeof(readResp));	
		for (i = 0; i< sizeof(readResp); i++)
		{
		 	printf("  %x ",  *((unsigned char *)&readResp + i) );
		}
		printf("\n");
*/

	}
		


	mei_deinit(&mkhi_cmd.mei_cl);


}


