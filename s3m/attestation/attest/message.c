/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Intel(R) CPU Attestation Sample Code
 *
 * Copyright(c) 2022 Intel Corporation
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "attest.h"
#include "oobmsm-dev.h"

static void dump_doe(const char *title, void *data)
{
	struct doe *doe;
	int i;

	if (debug == 0)
		return;

	printf("---- %s [0x%p] ----\n", title ? title : " ", data);
	doe = (struct doe *)data;
	printf("vendor=0x%x type=0x%x length=0x%x/%d [0x%08x 0x%08x]\n",
	       doe->vendor, doe->type, doe->length, doe->length,
	       ((unsigned int *)doe)[0], ((unsigned int *)doe)[1]);

	for (i = 2; i < doe->length; ++i)
		printf("dword%02d: 0x%08x\n", i, ((unsigned int *)doe)[i]);
}

int message(struct doe *doe)
{
	int fd, rc;

	/* Open the character device. */
	fd = open("/dev/" OOBMSM_DEV_NAME, O_RDWR);

	if (fd < 0) {
		PRERR("open() failed: %s\n", strerror(errno));
		return -1;
	}

	if (delay) {
		printf("Sleeping for 10s to allow timeout testing... ");
		fflush(stdout);
		sleep(10);
		printf("awake!\n");
	}

	dump_doe("Sending", doe);

	rc = ioctl(fd, OOBMSM_IOC_MAGIC, doe);

	if (rc == -1) {
		PRERR("ioctl() failed: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	dump_doe("Received", doe);

	/* Close the character device. */
	rc = close(fd);

	if (rc == -1) {
		PRERR("close() failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
