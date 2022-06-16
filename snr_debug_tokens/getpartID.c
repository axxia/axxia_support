/******************************************************************************
 * Get Part ID
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





/* Example program to get PartID, Nonce and Timebase using Hotham GetPartID Heci command */

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


#pragma pack(1)


/* Hotham (debug) header */

typedef struct
{
   uint32_t	MsgClass	:2;
   uint32_t	TargetId	:3;
   uint32_t	SequenceNo	:3;
   uint32_t	ReqCode		:8;
   uint32_t	MsgLength	:12;
   uint32_t	Reserved	:3;
   uint32_t	HeaderType	:1;
} HOTHAM_HECI_HEADER;




/* Hotham fw version number struct */
typedef struct
{
    uint16_t                             CodeMajor;
    uint16_t                             CodeMinor;
    uint16_t                             CodeBuildNo;
    uint16_t                             CodeHotFix;
}  HTM_FW_VERSION;




/* Hotham Commands */

typedef struct
{
    HOTHAM_HECI_HEADER                 Header;
}  HothamCmdGetPartId_Request;


#define  HOTHAM_PART_ID_SIZE                     12

typedef struct
{
    HOTHAM_HECI_HEADER                 	Header;
    uint32_t                            PartIdTypeVer;
    uint32_t                            ReqFlags;
    HTM_FW_VERSION                     	HtmFwVer;
    uint32_t                            NumOfPartIDs;
    uint32_t                            LpcDid;
    uint8_t       			PartId[HOTHAM_PART_ID_SIZE] ;
    uint32_t                            Nonce;
    uint32_t                            Timestamp;
}  HothamCmdGetPartId_Response;


#pragma pack()



/* Struct containing info on Hotham connection */
struct hotham_host_if {
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
        mei_err(me, "IOCTL_MEI_CONNECT_CLIENT receive message. err=%d  errno=%d  %s  \n",result, errno, strerror(errno));
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

    fd_set set;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000000;
    FD_ZERO(&set);
    FD_SET(me->fd, &set);
    rc = select(me->fd + 1, &set, NULL, NULL, &tv);
    if (rc > 0 && FD_ISSET(me->fd, &set)) {
        mei_msg(me, "read data available, return value %zd\n", rc);
    }

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




const uuid_le MEI_HOTHAM_HIF = UUID_LE(0x082ee5a7, 0x7c25, 0x470a, 0x96, 0x43, 0x0c, 
    0x06, 0xf0, 0x46, 0x6e, 0xa1);
 
 

static bool hotham_host_if_init(struct hotham_host_if *acmd,
        			  unsigned long send_timeout, 
				  bool verbose) 
{
    acmd->send_timeout = (send_timeout) ? send_timeout : 20000;
    acmd->initialized = mei_init(&acmd->mei_cl, &MEI_HOTHAM_HIF, 0, verbose);
    return acmd->initialized;
}



int main(int argc, char **argv) 
{
	unsigned int i, j;

	/* Enable fixed address interface */
	FILE *fp = fopen("/sys/kernel/debug/mei0/allow_fixed_address", "w");
	int ret = (fwrite("Y", sizeof(char), 1, fp) == 1) ? 0 : -1;    
	fclose(fp);


	struct hotham_host_if  hotham_cmd;
	bool heci_init_call;


	/*Open Hotham comms with MEI driver */
    	heci_init_call = hotham_host_if_init(&hotham_cmd, 5000, false);
    	if (!heci_init_call) 
	{
		printf("Hotham i/f failed to initialise \n");       		
		mei_deinit(&hotham_cmd.mei_cl);
		return false;
	}
	else
	{
		printf("Hotham i/f initialised \n");
	}

 
	/* Send GetpartID  command */


	HothamCmdGetPartId_Request	partIdRequest;
	HothamCmdGetPartId_Response	partIdResponse;

	partIdRequest.Header.MsgClass = 1;  
	partIdRequest.Header.TargetId = 1;  
	partIdRequest.Header.SequenceNo = 0;
	partIdRequest.Header.ReqCode = 0x10;
	partIdRequest.Header.MsgLength = 0;
	partIdRequest.Header.HeaderType = 0;



	mei_send_msg(&hotham_cmd.mei_cl,  (const unsigned char *) &partIdRequest,4,5000);

	printf("GetPartID sent \n");


	/* Get response */
	mei_recv_msg(&hotham_cmd.mei_cl, (unsigned char *)&partIdResponse , sizeof(partIdResponse), 5000);  
       


	/* Display PartID info */

	printf("\nPartID Response \n");
	printf("==================\n");

#if 0
	printf("Msg Class :  0x%02x \n", partIdResponse.Header.MsgClass );  
	printf("Target ID :  0x%02x \n", partIdResponse.Header.TargetId );  
	printf("Sequence Number :  0x%02x \n", partIdResponse.Header.SequenceNo );  
	printf("ReqCode:  0x%02x \n", partIdResponse.Header.ReqCode);  
	printf("MsgLength:  0x%02x \n", partIdResponse.Header.MsgLength); 
	printf("HeaderType:  0x%02x \n", partIdResponse.Header.HeaderType); 

	printf("PartIdTypeVer:  0x%04x \n", partIdResponse.PartIdTypeVer); 
	printf("ReqFlags:  0x%02x \n", partIdResponse.ReqFlags); 

 	printf("HtmFwVer: \n");
	printf("	CodeMajor:    0x%04x \n", partIdResponse.HtmFwVer.CodeMajor); 
	printf("	CodeMinor:    0x%04x \n", partIdResponse.HtmFwVer.CodeMinor); 
	printf("	CodeBuildNo:  0x%04x \n", partIdResponse.HtmFwVer.CodeBuildNo); 
	printf("	CodeHotFix:   0x%04x \n", partIdResponse.HtmFwVer.CodeHotFix); 

  	printf("NumOfPartIDs:  0x%08x \n", partIdResponse.NumOfPartIDs); 
   	printf("LpcDid:  0x%08x \n", partIdResponse.LpcDid); 
#endif
	printf("PartID: 0x");
	for(i=0; i< HOTHAM_PART_ID_SIZE; i++)
	{
		printf("%02x",partIdResponse.PartId[i]);
	}
	printf("\n");

   	printf("Nonce:  0x%08x \n", partIdResponse.Nonce); 
   	printf("Timestamp:  0x%08x \n", partIdResponse.Timestamp); 

	
	mei_deinit(&hotham_cmd.mei_cl);
}

