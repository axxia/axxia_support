/******************************************************************************
 * SDOS  Encrypt, Decrypt and Get Info Example Application
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (C) 2003-2023 Intel Corporation. All rights reserved.
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
 * Copyright (C) 2003-2023 Intel Corporation. All rights reserved.
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

#ifndef _SDOS_H_
#define _SDOS_H_


#define DOE_MAX_PAYLOAD_SIZE  (1024 - 2)

struct s_doe {
	__u16 vendor;
	__u8  type;
	__u8  reserved;
	__u32 length;
	__u32 payload[DOE_MAX_PAYLOAD_SIZE];
};


#define OOBMSM_IOC_MAGIC _IOWR('a', 'c', struct s_doe *)



struct s_sdos_encrypt_cmd {
	__u32 commandID;
	__u32 ownerID[4];
	__u32 iv[4];
	__u32 length;
	__u32 data[ DOE_MAX_PAYLOAD_SIZE -  44 ];
};

struct s_sdos_encrypt_rsp {
	__u32 status;
	__u32 hmac[8];
	__u32 length;
	__u32 data[ DOE_MAX_PAYLOAD_SIZE -  44 ];
};


struct s_sdos_decrypt_cmd {
	__u32 commandID;
	__u32 ownerID[4];
	__u32 iv[4];
	__u32 hmac[8];
	__u32 length;
	__u32 data[ DOE_MAX_PAYLOAD_SIZE -  76 ];
};


struct s_sdos_decrypt_rsp {
	__u32 status;
	__u32 length;
	__u32 data[ DOE_MAX_PAYLOAD_SIZE -  12 ];
};



struct s_sdos_stateInfo {
	__u32	sdosEnabled 		: 1;
	__u32	zeroizeTriggered 	: 1;
	__u32	sdosActive		: 1;
	__u32	srkSet 			: 1;
	__u32	srkCommitted 		: 1;
	__u32	eom 			: 1;
	__u32   pendingVAB		: 1;
	__u32	debugMode		: 1;
	__u32	endHostPriv		: 1;
	__u32   reserved		: 23;
};


struct s_sdos_srkState {
	__u8	valid		: 1;
	__u8	revoked		: 1;
	__u8	reserved	: 6;
};


struct s_sdos_get_info_cmd {
	__u32 commandID;
};


struct s_sdos_get_info_rsp {
	__u32 				status;
	__u32 				maxChunkLength;
	struct s_sdos_stateInfo		stateInfo;
	struct s_sdos_srkState  	srk1State;
	struct s_sdos_srkState  	srk2State;	
	struct s_sdos_srkState  	srk3State;
	__u8  				reserved;	
};





#endif	/* _SDOS_H_ */
