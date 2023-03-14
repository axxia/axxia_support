/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Intel(R) OOBMSM RDK driver
 *
 * Copyright(c) 2023 Intel Corporation
 */

#ifndef _OOBMSM_IOCTL_H_
#define _OOBMSM_IOCTL_H_

#define DOE_MAX_SIZE 1024	/* number of words (32 bits) */
#define DOE_MAX_PAYLOAD_SIZE (DOE_MAX_SIZE - 2)
#define OOBMSM_IOC_MAGIC _IOWR('a', 'c', struct doe_old *)
#define OOBMSM_DEV_NAME "oobmsm"

struct doe {
	__u16 vendor;
	__u8  type;
	__u8  reserved;
	__u32 length;
	__u32 payload[DOE_MAX_PAYLOAD_SIZE];
};

#endif	/* _OOBMSM_IOCTL_H_ */
