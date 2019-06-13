/******************************************************************************
 * Example using SB_VERIFY_MANIFEST To verify a manifest in Linux
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (C) 2003-2019 Intel Corporation. All rights reserved.
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





/* Simple test program to send SB_VERIFY_MANIFEST messages to ME */

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


#include "Manifest_Metadata.h"



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




#define MAX_CHUNK_SIZE  400

/* Flags */
#define MAN_DATA 1
#define MET_DATA 2
#define START_OVER_VERIFY 4
#define LAST_CHUNK 8


#pragma pack(1)

typedef struct
{ 
    	MKHI_MESSAGE_HEADER	Header;
	uint32_t		UsageIndex;
	uint32_t		ManifestSize;
	uint32_t		MetaDataSize;
	uint8_t			Flags;
	uint16_t		ChunkSize;
	uint8_t			Reserved;
	uint8_t 	        Data[MAX_CHUNK_SIZE];
} SB_VERIFY_MANIFEST_Request;
		

typedef struct
{ 
    	MKHI_MESSAGE_HEADER	Header;
	uint8_t			Flags;
	uint16_t		ChunkSize;
	uint8_t			Reserved;
} SB_VERIFY_MANIFEST_Response;

#pragma pack()




/* Struct containing info on MKHI connection */
struct mkhi_host_if {
    struct mei mei_cl;
    unsigned long send_timeout;
    bool initialized;
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
	unsigned int i;

	/* Enable fixed address interface */
	FILE *fp = fopen("/sys/kernel/debug/mei0/allow_fixed_address", "w");
	int ret = (fwrite("Y", sizeof(char), 1, fp) == 1) ? 0 : -1;    
	fclose(fp);
	

    	struct mkhi_host_if mkhi_cmd;


        /* Open MKHI comms with mei driver */
    	bool heci_init_call = mkhi_host_if_fix_init(&mkhi_cmd, 5000, true);
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





	/* Authenticate a manifest file using SB_VERIFY_MANIFEST */


	/*Read manifest into buffer */
	unsigned char manifestBuffer[2048];


	FILE *manifestFP = fopen(argv[1], "r");
	if (manifestFP == NULL)
	{
		printf("Error, could not open manifest file, exiting! \n");
		exit(-1);
	}

	fread(manifestBuffer,1,sizeof(manifestBuffer),manifestFP);

	if ( ferror(manifestFP) !=0 )
	{
		printf("Error reading manifest file, exiting! \n");
		fclose(manifestFP);
		exit(-1);
	}
	fclose(manifestFP);

	
	/* Read manifest header to get manifest size */
	MANIFEST_HEADER_t *manifestHeader = (MANIFEST_HEADER_t *)manifestBuffer;	
	unsigned int manifestSize = manifestHeader->Size * 4;
	printf("Manifest size: %d bytes \n",manifestSize);

	/*Check metadata header and size is OK (metadata is located immediately after manifest) */
	OS_NETLOADER_METADATA_t *metaData = (OS_NETLOADER_METADATA_t *)(&manifestBuffer[manifestSize]);

	printf("Metadata size: %d bytes \n",metaData->Length);
	
	if (metaData->Type = 0x22)
	{
		printf("Netloader manifest type found \n");
	}
	else if (metaData->Type = 0x23)
	{
		printf("OS manifest type found \n");
	}
	else
	{
		printf("Error, incorrect manifest type found: %x \n", metaData->Type);
		exit(-1);
	}

	printf("Manifest contains hash value: 0x");
	for (i=0; i < metaData->HashSize/4; i++)
	{
		printf("%04x",metaData->Hash[i]);
	}
	printf("\n");


/*  Print manifest data */
#if 0
	printf("\n");
	for (i=0; i < manifestSize; i++)
	{
		if ( i % 16 == 0)
		{
			printf("\n");		 	
		}			
		printf("%02x",  manifestBuffer[i] );
	}
	printf("\n");
#endif /* 0 */



	SB_VERIFY_MANIFEST_Request  verifyRequest;
	SB_VERIFY_MANIFEST_Response verifyResponse;


	/*Send manifest in chunks */
	unsigned char * chunkP = manifestBuffer;     	/* Ptr to start of chunk in manifest */
	unsigned int chunkCounter =0;			/* Number of chunks sent */
	unsigned int chunkSize = MAX_CHUNK_SIZE;   
	unsigned int chunks = manifestSize/chunkSize;  	/* Number of chunks in manifest  */
	if ( (manifestSize % chunkSize) !=0)
	{
		chunks++;
	}

	printf("Number of manifest chunks: %d \n", chunks);
	

	unsigned int messageSize = 0;

	
	for (chunkCounter = 0; chunkCounter <chunks; chunkCounter++)
	{	
		/* Setup request header in Request */
		verifyRequest.Header.GroupId = 0x0c;
		verifyRequest.Header.Command = 0x1;
		verifyRequest.Header.IsResponse = 0;
		verifyRequest.Header.Reserved = 0;
		verifyRequest.Header.Result = 0;

		verifyRequest.Reserved=0;
		       	
		/*Set usage index to netloader or OS in Request */
		if (metaData->Type = 0x22)
		{
			verifyRequest.UsageIndex = 38;  /* Netloader */
		}
		else 
		{
			verifyRequest.UsageIndex = 39;   /* OS */
		}
			
		  
		/* Set Manifest Size in Request */
		verifyRequest.ManifestSize = manifestSize/4;  /* Size in DWORDS */	

		/* Set MetaData Size in Request */
		verifyRequest.MetaDataSize = metaData->Length;  /* Size in bytes */	

		/* Set flags in Request */
		verifyRequest.Flags = 0;

		if (chunkCounter == 0)   /* if first chunk */
		{
			verifyRequest.Flags |= START_OVER_VERIFY;
		}
		verifyRequest.Flags |= MAN_DATA;

		/* Set chunk data size in Request */
		if (( (chunkCounter+1) * chunkSize) > manifestSize)  
		{
			verifyRequest.ChunkSize = (manifestSize - ((chunks-1) * chunkSize)) ;
		}
		else
		{
			verifyRequest.ChunkSize = chunkSize;
		}

		/*Copy chunk data into Request */
		memcpy(verifyRequest.Data,chunkP,chunkSize);



		messageSize = sizeof(SB_VERIFY_MANIFEST_Request) - MAX_CHUNK_SIZE + verifyRequest.ChunkSize ;



		/*Print msg contents */
		printf("Header: 0x%x \n",verifyRequest.Header);
		printf("UsageIndex: 0x%x \n",verifyRequest.UsageIndex);
		printf("ManifestSize: 0x%x \n",verifyRequest.ManifestSize);
		printf("MetaDataSize: 0x%x \n",verifyRequest.MetaDataSize);
		printf("Flags: 0x%x \n",verifyRequest.Flags);
		printf("ChunkSize: 0x%x \n",verifyRequest.ChunkSize);
		printf("Msg size: %d \n",messageSize );


/* Print actual bytes in verifyRequest */
#if 0
		for (i=0; i< sizeof(verifyRequest); i++)
		{
		   if (i %16 == 0)
			printf("\n");
		   printf("%02x",*((unsigned char *)&verifyRequest + i) );

		} 
		printf("\n\n\n");
#endif /* 0 */


		/*Send SB_VERIFY_MANIFEST_REQUEST */			
		mei_send_msg(&mkhi_cmd.mei_cl,  (const unsigned char *) &verifyRequest,messageSize ,5000);

		printf("SB_VERIFY_MANIFEST_Request sent \n");

		/* Get response */
		mei_recv_msg(&mkhi_cmd.mei_cl, (unsigned char *)&verifyResponse, sizeof(SB_VERIFY_MANIFEST_Response),
		5000);		

		printf("SB_VERIFY_MANIFEST_Response received \n");



		/* Check if response OK */
		if (verifyResponse.Header.Result !=1)
		{
			printf("Error!  Response result = 0x%x \n", verifyResponse.Header.Result);
			printf("Flags %x \n",verifyResponse.Flags);
			printf("ChunkSize %x \n",verifyResponse.ChunkSize);
			exit(-1);
		}
		else
		{
			printf("Sent chunk %d. \n\n\n\n",chunkCounter);
		}

		/* Move to next chunk */
		chunkP += chunkSize;
	}

	
	/* Send metadata  */
	verifyRequest.Header.GroupId = 0x0c;
	verifyRequest.Header.Command = 0x1;
	verifyRequest.Header.IsResponse = 0;
	verifyRequest.Header.Reserved = 0;
	verifyRequest.Header.Result = 0;
	if (metaData->Type = 0x22)
	{
		verifyRequest.UsageIndex = 38;  /* Netloader */
	}
	else 
	{
		verifyRequest.UsageIndex = 39;   /* OS */
	}
	verifyRequest.ManifestSize = manifestSize/4;  /* Size in DWORDS */	
	verifyRequest.MetaDataSize = metaData->Length;  /* Size in bytes */	
	verifyRequest.Flags = MET_DATA | LAST_CHUNK;
	verifyRequest.ChunkSize = metaData->Length;
	memcpy(verifyRequest.Data,metaData,metaData->Length);
	messageSize = sizeof(SB_VERIFY_MANIFEST_Request) - MAX_CHUNK_SIZE + verifyRequest.ChunkSize ;


	/*Print msg contents */
	printf("Header: 0x%x \n",verifyRequest.Header);
	printf("UsageIndex: 0x%x \n",verifyRequest.UsageIndex);
	printf("ManifestSize: 0x%x \n",verifyRequest.ManifestSize);
	printf("MetaDataSize: 0x%x \n",verifyRequest.MetaDataSize);
	printf("Flags: 0x%x \n",verifyRequest.Flags);
	printf("ChunkSize: 0x%x \n",verifyRequest.ChunkSize);
	printf("Msg size: %d \n",messageSize );

/* Print actual bytes in verifyRequest */
#if 0
		for (i=0; i< messageSize; i++)
		{
		   if (i %16 == 0)
			printf("\n");
		   printf("%02x",*((unsigned char *)&verifyRequest + i) );

		} 
		printf("\n\n\n");

#endif /* 0 */



	/*Send metaData */			
	mei_send_msg(&mkhi_cmd.mei_cl,  (const unsigned char *) &verifyRequest,messageSize ,5000);

	/* Get response */
	mei_recv_msg(&mkhi_cmd.mei_cl, (unsigned char *)&verifyResponse, sizeof(SB_VERIFY_MANIFEST_Response),
		5000);		


	/* Check if response OK */
	if (verifyResponse.Header.Result !=0)
	{
		printf("Error!  Response result = 0x%x \n", verifyResponse.Header.Result);
		exit(-1);
	}
	else
	{
		printf("Sent metadata, authentication sucessful \n");
	}


	mei_deinit(&mkhi_cmd.mei_cl);


}


