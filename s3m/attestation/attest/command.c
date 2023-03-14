/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Intel(R) CPU Attestation Sample Code
 *
 * Copyright(c) 2022 Intel Corporation
 */

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "attest.h"
#include "oobmsm-dev.h"

static void display(const char *header, void *data, int length)
{
	unsigned char *data8 = data;
	unsigned long offset = 0;

        if (header)
                printf("---- %s ----\n", header);

        while (0 < length) {
		char buffer[256];
                int this_line;
		char *string;
		int i;

                string = buffer;
                string += sprintf(string, "%06lx ", offset);
                this_line = (16 > length) ? length : 16;

                for (i = 0; i < this_line; ++i) {
                        string += sprintf(string, "%02x ", *data8++);
                        --length;
                        ++offset;
                }

                printf("%s\n", buffer);
        }
}

/**
 * Check to see if the response indicates an error.
 *
 * -1 : invalid data
 *  0 : no error
 *  1 : error
 */

static int error_check(void *data)
{
	struct doe *doe = data;
	struct spdm_error *spdm_error = (data + 12);
	char buffer[256];

	/**
	 * Make sure a response is included in the DOE (3rd dword) and
	 * the version is correct.
	 */

	if (doe->length < 4 || spdm_error->version != SPDM_FIRMWARE_VERSION) {
		PRERR("Invalid Data\n");
		return -1;
	}

	if (spdm_error->response != SPDM_ERR)
		return 0;

	switch (spdm_error->code) {
	case SPDM_ERR_INVALID_REQUEST:
		strcpy(buffer, "Invalid Request");
		break;
	case SPDM_ERR_BUSY:
		strcpy(buffer, "Busy");
		break;
	case SPDM_ERR_UNEXPECTED_REQUEST:
		strcpy(buffer, "Unexpected Request");
		break;
	case SPDM_ERR_UNSPECIFIED:
		strcpy(buffer, "Unspecified");
		break;
	case SPDM_ERR_DECRYPT_ERROR:
		strcpy(buffer, "Decrypt Error");
		break;
	case SPDM_ERR_UNSUPPORTED_REQUEST:
		sprintf(buffer, "Unsupported Request (0x%x)", spdm_error->data);
		break;
	case SPDM_ERR_REQUEST_IN_FLIGHT:
		strcpy(buffer, "Request In Flight");
		break;
	case SPDM_ERR_INVALID_RESPONSE_CODE:
		strcpy(buffer, "Invalid Response Code");
		break;
	case SPDM_ERR_SESSION_LIMIT_EXCEEDED:
		strcpy(buffer, "Session Limit Exceeded");
		break;
	case SPDM_ERR_SESSION_REQUIRED:
		strcpy(buffer, "Session Required");
		break;
	case SPDM_ERR_RESET_REQUIRED:
		strcpy(buffer, "Reset Required");
		break;
	case SPDM_ERR_RESPONSE_TOO_LARGE:
		strcpy(buffer, "Response Too Large");
		break;
	case SPDM_ERR_REQUEST_TOO_LARGE:
		strcpy(buffer, "Request Too Large");
		break;
	case SPDM_ERR_LARGE_RESPONSE:
		strcpy(buffer, "Large Response");
		break;
	case SPDM_ERR_MAJOR_VERSION_MISMATCH:
		strcpy(buffer, "Major Version Mismatch");
		break;
	case SPDM_ERR_RESPONSE_NOT_READY:
		strcpy(buffer, "Response Not Ready");
		break;
	case SPDM_ERR_REQUEST_RESYNC:
		strcpy(buffer, "Request Resync");
		break;
	default:
		sprintf(buffer, "Unknown Error Code: 0x%x\n", spdm_error->code);
		break;
	}

	PRERR("SPDM Error: %s\n", buffer);
	return 1;
}

int bad(void)
{
	struct spdm_ver *spdm_ver;
	struct doe *doe;

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("Unable to Allocate DOE\n");
		return -1;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 5;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = 0x1001;
	doe->payload[2] = (SPDM_GET_VER << 8 | SPDM_FIRMWARE_VERSION);

	if (message(doe))
		goto error;

	if (error_check(doe))
		goto error;

	if (doe->payload[1] != (SPDM_VER << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("Invalid Version or Response Code: 0x%x\n",
		      doe->payload[1]);
		goto error;
	}

	if (doe->length < 5) {
		PRERR("Response Too Short\n");
		goto error;
	}

	spdm_ver = (struct spdm_ver *)(&doe->payload[2]);

	printf("Version Number Entry Count: %d\n"
	       "      Version Number Entry: 0x%x\n",
	       spdm_ver->version_number_entry_count,
	       spdm_ver->version_number_entry);

	free(doe);
	return 0;

 error:
	free(doe);
	return -1;
}

int get_ver(void)
{
	struct spdm_ver *spdm_ver;
	struct doe *doe;

	if (strlen(input) || strlen(output)) {
		PRERR("get_ver doesn't support input/output from/to a file.\n");
		return -1;
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		return -1;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 5;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (SPDM_GET_VER << 8 | SPDM_FIRMWARE_VERSION);

	if (message(doe))
		goto error;

	if (error_check(doe))
		goto error;

	if (doe->payload[1] != (SPDM_VER << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("Invalid Version or Response Code: 0x%x\n",
		      doe->payload[1]);
		goto error;
	}

	if (doe->length < 5) {
		PRERR("Response Too Short\n");
		goto error;
	}

	spdm_ver = (struct spdm_ver *)(&doe->payload[2]);

	printf("Version Number Entry Count: %d\n"
	       "      Version Number Entry: 0x%x\n",
	       spdm_ver->version_number_entry_count,
	       spdm_ver->version_number_entry);

	free(doe);
	return 0;

 error:
	free(doe);
	return -1;
}

int get_caps(void)
{
	struct spdm_caps *spdm_caps;
	struct doe *doe;

	if (strlen(input) || strlen(output)) {
		PRERR("get_caps doesn't support input/output from/to a file.\n");
		return -1;
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		return -1;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 5;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (SPDM_GET_CAPS << 8 | SPDM_FIRMWARE_VERSION);

	if (message(doe))
		goto error;

	if (error_check(doe))
		goto error;

	if (doe->payload[1] != (SPDM_CAPS << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("Invalid Version or Response Code: 0x%x\n",
		      doe->payload[1]);
		goto error;
	}

	if (doe->length < 6) {
		PRERR("Response Too Short\n");
		goto error;
	}

	spdm_caps = (struct spdm_caps *)(&doe->payload[2]);

	printf("       Detailed Version: 0x%x\n"
	       "Crypto Timeout Exponent: %d\n"
	       "                  Flags: 0x%x\n"
	       "              CACHE_CAP: %s\n"
	       "               CERT_CAP: %s\n"
	       "               CHAL_CAP: %s\n"
	       "               MEAS_CAP: %s\n"
	       "         MEAS_FRESH_CAP: %s\n",
	       spdm_caps->detailed_version,
	       spdm_caps->crypto_tmout_exp,
	       spdm_caps->flags,
	       spdm_caps->flags & CAPS_CACHE_CAP ? "*INVALID*" : "disabled",
	       spdm_caps->flags & CAPS_CERT_CAP ? "enabled" : "*INVALID*",
	       spdm_caps->flags & CAPS_CHAL_CAP ? "enabled" : "*INVALID*",
	       (((spdm_caps->flags & CAPS_MEAS_CAP_M) >> CAPS_MEAS_CAP_S) ==
		CAPS_MEAS_CAP_RSP_SUPPORT) ? "enabled" : "*INVALID*",
	       spdm_caps->flags & CAPS_MEAS_FRESH_CAP ?
	       "*INVALID*" : "disabled");

	free(doe);
	return 0;

 error:
	free(doe);
	return -1;
}

int neg_algs(void)
{
	struct spdm_neg_algs_cmd *spdm_neg_algs_cmd;
	struct spdm_neg_algs_resp *spdm_neg_algs_resp;
	struct doe *doe;

	if (strlen(input) || strlen(output)) {
		PRERR("neg_algs doesn't support input/output from/to a file.\n");
		return -1;
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		return -1;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 12;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;

	spdm_neg_algs_cmd = (struct spdm_neg_algs_cmd *)(&doe->payload[2]);

	spdm_neg_algs_cmd->version = SPDM_FIRMWARE_VERSION;
	spdm_neg_algs_cmd->code = SPDM_NEG_ALGS;
	spdm_neg_algs_cmd->length = 0x20;
	spdm_neg_algs_cmd->measurement_specification = 1;
	spdm_neg_algs_cmd->base_asym_algo = 0x80;
	spdm_neg_algs_cmd->base_hash_algo = 0x2;
	spdm_neg_algs_cmd->ext_asym_count = 0;
	spdm_neg_algs_cmd->ext_hash_count = 0;
	memset(spdm_neg_algs_cmd->data, 0, sizeof(spdm_neg_algs_cmd->data));

	if (message(doe))
		goto error;

	if (error_check(doe))
		goto error;

	if (doe->payload[1] != (SPDM_ALGS << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("invalid version or response code: 0x%x\n",
		      doe->payload[1]);
		goto error;
	}

	if (doe->length < 12) {
		PRERR("response is too short\n");
		goto error;
	}

	spdm_neg_algs_resp = (struct spdm_neg_algs_resp *)(&doe->payload[2]);

	printf("                             Length: 0%d\n"
	       "Measurement Specification Selection: 0x%x\n"
	       "         Measurement Hash Algorithm: 0x%x\n"
	       "          Base Asymmetric Selection: 0x%x\n"
	       "                Base Hash Selection: 0x%x\n",
	       spdm_neg_algs_resp->length,
	       spdm_neg_algs_resp->measurement_specification_sel,
	       spdm_neg_algs_resp->measurement_hash_algo,
	       spdm_neg_algs_resp->base_asym_sel,
	       spdm_neg_algs_resp->base_hash_sel);

	free(doe);
	return 0;

 error:
	free(doe);
	return -1;
}

int get_csr(void)
{
	struct spdm_csr *spdm_csr;
	unsigned char *data;
	FILE *out = NULL;
	struct doe *doe;
	int rc = 0;

	if (strlen(input)) {
		PRERR("get_csr doesn't support input from file.\n");
		return -1;
	}

	if (strlen(output)) {
		out = fopen(output, "w");

		if (!out) {
			PRERR("fopen(%s) failed: %s\n",
			      output, strerror(errno));
			return -1;
		}
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 6;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;

	spdm_csr = (struct spdm_csr *)(&doe->payload[2]);
	spdm_csr->version = SPDM_FIRMWARE_VERSION;
	spdm_csr->code = SPDM_GET_CSR;
	spdm_csr->data = (__u64)&doe->payload[3];
	data = (unsigned char *)spdm_csr->data;
	memset(data, 0, 4);

	if (message(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (error_check(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (doe->payload[1] !=
	    (SPDM_GET_CSR_RESP << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("invalid version or response code: 0x%x\n",
		      doe->payload[1]);
		rc = -1;
		goto cleanup;
	}

	if (out)
		fwrite((void *)&doe->payload[3],
		       sizeof(char), ((doe->length - 5) * 4), out);
	else
		display(NULL, (void *)&doe->payload[3], (doe->length - 5) * 4);

 cleanup:

	if (out)
		fclose(out);

	if (doe)
		free(doe);

	return rc;
}

int set_crt(void)
{
	struct spdm_cert_chain *cert_chain;
	struct doe *doe = NULL;
	FILE *dgst = NULL;
	FILE *crt = NULL;
	char *dgst_name;
	char *crt_name;
	long dgst_sz;
	long crt_sz;
	int rc = 0;

	if (strlen(output)) {
		PRERR("set_cert doesn't support output to a file.\n");
		return -1;
	}

	/* <digest>:<certificate> */

	dgst_name = strtok(input, ":");
	crt_name = strtok(NULL, " ");

	if (!strlen(input) || !dgst_name || !crt_name) {
		PRERR("set_crt requires a digest and a certificate!\n");
		return -1;
	}

	dgst = fopen(dgst_name, "r");

	if (!dgst) {
		PRERR("fopen(%s) failed: %s\n", dgst_name, strerror(errno));
		return -1;
	}

	crt = fopen(crt_name, "r");

	if (!crt) {
		PRERR("fopen(%s) failed: %s\n", crt_name, strerror(errno));
		rc = -1;
		goto cleanup;
	}

	fseek(dgst, 0L, SEEK_END);
	dgst_sz = ftell(dgst);
	rewind(dgst);

	fseek(crt, 0L, SEEK_END);
	crt_sz = ftell(crt);
	rewind(crt);

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 5 + ((4 + dgst_sz + crt_sz) / 4);

	if ((dgst_sz + crt_sz) % 4)
		++doe->length;

	doe->payload[0] = 
		(4 - ((dgst_sz + crt_sz) % 4)) << 24 |
		S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (SPDM_SET_CERT << 8 | SPDM_FIRMWARE_VERSION);
	cert_chain = (struct spdm_cert_chain *)(&doe->payload[3]);
	cert_chain->length = (4 + dgst_sz + crt_sz);

	if (dgst_sz != fread(cert_chain->data,
			     sizeof(unsigned char), dgst_sz, dgst)) {
		PRERR("fread() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	if (crt_sz != fread(&cert_chain->data[dgst_sz],
			    sizeof(unsigned char), crt_sz, crt)) {
		PRERR("fread() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	if (message(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (error_check(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (doe->payload[1] !=
	    (SPDM_SET_CERT_RESP << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("invalid version or response code: 0x%x\n",
		      doe->payload[1]);
		rc = -1;
		goto cleanup;
	}

 cleanup:

	if (dgst)
		fclose(dgst);

	if (crt)
		fclose(crt);

	if (doe)
		free(doe);

	return rc;
}

int get_dgst(void)
{
	FILE *out = NULL;
	struct doe *doe;
	int rc = 0;

	if (strlen(input)) {
		PRERR("get_dgsts doesn't support input from file.\n");
		return -1;
	}

	if (strlen(output)) {
		out = fopen(output, "w");

		if (!out) {
			PRERR("fopen(%s) failed: %s\n", output, strerror(errno));
			return -1;
		}
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 5;
	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (SPDM_GET_DGSTS << 8 | SPDM_FIRMWARE_VERSION);

	if (message(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (error_check(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (doe->payload[1] !=
	    (DGST_SLOT_MSK << 24 | SPDM_DGSTS << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("Invalid Version or Response Code: 0x%x\n",
		      doe->payload[1]);
		rc = -1;
		goto cleanup;
	}

	if (out)
		fwrite((void *)&doe->payload[2], sizeof(char), DGST_SIZE, out);
	else
		display(NULL, (void *)&doe->payload[2], DGST_SIZE);

 cleanup:

	if (out)
		fclose(out);

	if (doe)
		free(doe);

	return rc;
}

int get_crt(void)
{
	int o = 0, s, p, rc = 0;
	struct doe *doe = NULL;
	unsigned short length;
	FILE *dgst = NULL;
 	void *buf = NULL;
	FILE *crt = NULL;
	char *dgst_name;
	char *crt_name;

	if (strlen(input)) {
		PRERR("get_dgsts doesn't support input from file.\n");
		return -1;
	}

	/* <digest>:<certificate> */

	dgst_name = strtok(output, ":");
	crt_name = strtok(NULL, " ");

	if (!strlen(output) || !dgst_name || !crt_name) {
		PRERR("get_crt requires filenames for a digest and a certification!\n");
		return -1;
	}

	dgst = fopen(dgst_name, "w");

	if (!dgst) {
		PRERR("fopen(%s) failed: %s\n", dgst_name, strerror(errno));
		return -1;
	}

	crt = fopen(crt_name, "w");

	if (!crt) {
		PRERR("fopen(%s) failed: %s\n", crt_name, strerror(errno));
		rc = -1;
		goto cleanup;
	}

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	for (;;) {
		memset(doe, 0, sizeof(struct doe));
		doe->vendor = 0x8086;
		doe->type = 0xc;
		doe->length = 6;
		doe->payload[0] = S3M_PROXY_COMMAND;
		doe->payload[1] = CPU_ATTESTATION_COMMAND;
		doe->payload[2] = (SPDM_GET_CERT << 8 | SPDM_FIRMWARE_VERSION);
		doe->payload[3] = (MAX_CERT_SIZE << 16 | o);

		if (message(doe)) {
			rc = -1;
			goto cleanup;
		}

		if (error_check(doe)) {
			rc = -1;
			goto cleanup;
		}

		if (doe->payload[1] !=
		    (SPDM_CERT << 8 | SPDM_FIRMWARE_VERSION)) {
			PRERR("Invalid Version or Response Code: 0x%x\n",
			      doe->payload[1]);
			rc = -1;
			goto cleanup;
		}

		p = (doe->payload[2] & 0xffff);

		if (buf == NULL) {
			s = p + ((doe->payload[2] & 0xffff0000) >> 16);

			buf = malloc(s);

			if (!buf) {
				PRERR("unable to allocate buf\n");
				rc = -1;
				goto cleanup;
			}
		}

		if ((o + p) > s) {
			PRERR("Certificate Longer Than Expected\n");
			rc = -1;
			goto cleanup;
		}

		memcpy(buf + o, (void *)(&doe->payload[3]), p);
		o += p;

		if ((doe->payload[2] & 0xffff0000) == 0)
			break;
	}

	/**
	   The buffer contains...

	   length (2 bytes, little endian)
	   reserved (2 bytes)
	   digest of the root certificate, 48 bytes (big endian)
	   certificate chain
	*/

	length = *(unsigned short *)buf;

	if (length != s) {
		fprintf(stderr, "Invalid Length: %d != %d\n", length, s);
		rc = -1;
		goto cleanup;
	}

	if (dgst && crt) {
		fwrite(buf + 4, sizeof(char), 48, dgst);
		fwrite(buf + 52, sizeof(char), s - 52, crt);
	} else {
		display("digest", buf + 4, 48);
		display("certificate", buf + 52, s - 52);
	}

 cleanup:

	if (dgst)
		fclose(dgst);

	if (crt)
		fclose(crt);

	if (doe)
		free(doe);

	if (buf)
		free(buf);

	return rc;
}

int chlg(void)
{
	char *nonce_name, *chash_name, *mhash_name, *sig_name;
	FILE *nonce, *chash, *mhash, *sig;
	struct spdm_challenge_resp *resp;
	unsigned char nbuf[NONCE_LENGTH];
	struct doe *doe = NULL;
	void *buf = NULL;
	int rc = 0;
	int sz;

	/**
	 * If an input file is specified, assume it is the nonce for
	 * the request.  If not, get 32 bytes from /dev/random.
	 *
	 * There are five possible outputs.  So, either five filenames
	 * should be specified, or all five outputs will be displayed
	 * on stdout.
	 */

	if (strlen(input))
		/* Read the nonce for the request. */
		nonce_name = input;
	else
		/* Get 32 bytes from /dev/random. */
		nonce_name = "/dev/random";

	nonce = fopen(nonce_name, "r");

	if (!nonce) {
		PRERR("fopen(%s) failed: %s\n", "/dev/random", strerror(errno));
		return -1;
	}

	sz = fread(nbuf, sizeof(unsigned char), NONCE_LENGTH, nonce);

	if (sz != NONCE_LENGTH) {
		PRERR("fread() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	rc = fclose(nonce);

	if (rc) {
		PRERR("fclose() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	nonce = NULL;

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 13;

	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (0xff << 24 | /* all measurements */
			   SPDM_CHALLENGE << 8 |
			   SPDM_FIRMWARE_VERSION);

	memcpy((void *)&doe->payload[3], nbuf, NONCE_LENGTH);

	if (message(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (error_check(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (doe->payload[1] !=
	    (1 << 24 | SPDM_CHALLENGE_AUTH << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("invalid version or response code: 0x%x\n",
		      doe->payload[1]);
		rc = -1;
		goto cleanup;
	}

	buf = (void *)&doe->payload[2];
	resp = (struct spdm_challenge_resp *)buf;

	/* There should be no opaque data... make sure! */
	if (0 != resp->opaque_length) {
		PRERR("Unexpected Opaque Data");
		rc = -1;
		goto cleanup;
	}

	if (strlen(output)) {
		chash_name = strtok(output, ":");
		nonce_name = strtok(NULL, ":");
		mhash_name = strtok(NULL, ":");
		sig_name = strtok(NULL, " ");

		chash = fopen(chash_name, "w");
		nonce = fopen(nonce_name, "w");
		mhash = fopen(mhash_name, "w");
		sig = fopen(sig_name, "w");

		if (!chash || !nonce || !mhash || !sig) {
			PRERR("Error opening an output file.\n");
			rc = -1;
			goto cleanup;
		}

		fwrite(resp->chash, sizeof(char), HASH_LENGTH, chash);
		fwrite(resp->nonce, sizeof(char), NONCE_LENGTH, nonce);
		fwrite(resp->mhash, sizeof(char), HASH_LENGTH, mhash);
		fwrite(resp->signature, sizeof(char), SIGNATURE_LENGTH, sig);
	} else {
		display("CertChainHash", resp->chash, HASH_LENGTH);
		display("Nonce", resp->nonce, NONCE_LENGTH);
		display("MeasurementSummaryHash", resp->mhash, HASH_LENGTH);
		display("Signature", resp->signature, SIGNATURE_LENGTH);
	}

 cleanup:

	if (nonce)
		fclose(nonce);

	return rc;
}

int get_mmts(void)
{
	unsigned char nbuf[NONCE_LENGTH];
	struct doe *doe = NULL;
	char *nonce_name;
	FILE *nonce;
	unsigned char *buf;
	int i, ms, rc = 0, sz, t;
	static char *mmt_names[] = {
		"01_soc-boot-time-fw_mmt.bin",
		"02_platform-strap-configuration_mmt.bin",
		"03_fit-record-4_mmt.bin",
		"04_ucode-fit-patch_mmt.bin",
		"05_startup-acm_mmt.bin",
		"06_vendor-authorization-policy_mmt.bin",
		"07_pch-measurement_mmt.bin",
		"08_boot-policy-manifest_mmt.bin",
		"13_cpk-mini-loader_mmt.bin",
		"14_cpk-nvm-block_mnt.bin",
		"17_user-image-1_mmt.bin",
		"18_user-image-2_mmt.bin",
		"19_user-image-3_mmt.bin",
		"20_user-image-4_mmt.bin"
	};

	/**
	 * If an input file is specified, assume it is the nonce for
	 * the request.  If not, get 32 bytes from /dev/random.
	 */

	if (strlen(input))
		/* Read the nonce for the request. */
		nonce_name = input;
	else
		/* Get 32 bytes from /dev/random. */
		nonce_name = "/dev/random";

	nonce = fopen(nonce_name, "r");

	if (!nonce) {
		PRERR("fopen(%s) failed: %s\n", "/dev/random", strerror(errno));
		return -1;
	}

	sz = fread(nbuf, sizeof(unsigned char), NONCE_LENGTH, nonce);

	if (sz != NONCE_LENGTH) {
		PRERR("fread() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	rc = fclose(nonce);

	if (rc) {
		PRERR("fclose() failed: %s\n", strerror(errno));
		rc = -1;
		goto cleanup;
	}

	nonce = NULL;

	doe = malloc(sizeof(struct doe));

	if (!doe) {
		PRERR("unable to allocate doe\n");
		rc = -1;
		goto cleanup;
	}

	memset(doe, 0, sizeof(struct doe));
	doe->vendor = 0x8086;
	doe->type = 0xc;
	doe->length = 13;

	doe->payload[0] = S3M_PROXY_COMMAND;
	doe->payload[1] = CPU_ATTESTATION_COMMAND;
	doe->payload[2] = (0xff << 24 |
			   1 << 16 |
			   SPDM_GET_MEASUREMENTS << 8 |
			   SPDM_FIRMWARE_VERSION);

	memcpy((void *)&doe->payload[3], nbuf, NONCE_LENGTH);

	if (message(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (error_check(doe)) {
		rc = -1;
		goto cleanup;
	}

	if (doe->payload[1] !=
	    (SPDM_MEASUREMENTS << 8 | SPDM_FIRMWARE_VERSION)) {
		PRERR("invalid version or response code: 0x%x\n",
		      doe->payload[1]);
		rc = -1;
		goto cleanup;
	}

	buf = (unsigned char *)&doe->payload[2];
	ms = buf[0];
	t = (buf[3] << 16 | buf[2] << 8 | buf[1]);
	printf("%d measurements, %d bytes total\n", ms, t);
	buf = (unsigned char *)&doe->payload[3];

	if (strlen(output)) {
		char *mmt_dir_name;
		char *nonce_name;
		char *sig_name;
		struct stat st = {0};
		FILE *out;
		char out_name[PATH_MAX];

		mmt_dir_name = strtok(output, ":");
		nonce_name = strtok(NULL, ":");
		sig_name = strtok(NULL, " ");

		if (stat(mmt_dir_name, &st) == -1) {
			rc = mkdir(mmt_dir_name, 0700);

			if (rc) {
				PRERR("mkdir() failed: %s\n", strerror(errno));
				rc = -1;
				goto cleanup;
			}
		}

		for (i = 0; i < ms; ++i) {
			sz = (buf[3] << 8 | buf[2]);
			buf += 4;
			strcpy(out_name, mmt_dir_name);
			strcat(out_name, "/");
			strcat(out_name, mmt_names[i]);
			out = fopen(out_name, "w+");

			if (!out) {
				PRERR("fopen(%s) failed: %s\n",
				      out_name, strerror(errno));
				rc = -1;
				goto cleanup;
			}

			fwrite(buf, sizeof(char), sz, out);
			fclose(out);
			buf += sz;
		}

		out = fopen(nonce_name, "w+");

		if (!out) {
			PRERR("fopen(%s) failed: %s\n",
			      nonce_name, strerror(errno));
			rc = -1;
			goto cleanup;
		}

		fwrite(buf, sizeof(char), NONCE_LENGTH, out);
		fclose(out);
		buf += NONCE_LENGTH;

		/* There should be no opaque data... make sure! */
		if (0 != (buf[0] << 8 | buf[1])) {
			PRERR("Unexpected Opaque Data");
			rc = -1;
			goto cleanup;
		}

		out = fopen(sig_name, "w+");

		if (!out) {
			PRERR("fopen(%s) failed: %s\n",
			      sig_name, strerror(errno));
			rc = -1;
			goto cleanup;
		}

		fwrite(buf, sizeof(char), SIGNATURE_LENGTH, out);
		fclose(out);
	} else {
		char title[80];

		for (i = 0; i < ms; ++i) {
			sz = (buf[3] << 8 | buf[2]);
			sprintf(title, "%s (%d bytes)", mmt_names[i], sz);
			buf += 4;
			display(title, buf, sz);
			buf += sz;
		}

		display("nonce", buf, NONCE_LENGTH);
		buf += NONCE_LENGTH;

		/* There should be no opaque data... make sure! */
		if (0 != (buf[0] << 8 | buf[1])) {
			PRERR("Unexpected Opaque Data");
			rc = -1;
			goto cleanup;
		}

		display("signature", buf, SIGNATURE_LENGTH);
	}

 cleanup:

	if (nonce)
		fclose(nonce);

	return rc;
}
