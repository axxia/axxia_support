/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Intel(R) CPU Attestation Sample Code
 *
 * Copyright(c) 2022 Intel Corporation
 */

#ifndef _ATTEST_H_
#define _ATTEST_H_

#include "oobmsm-dev.h"

/**
 * DOE Codes
 */

#define S3M_PROXY_COMMAND 0x00000002 /* DOE Payload Dword 0 */
#define CPU_ATTESTATION_COMMAND 0x00001001 /* DOE Payload Dword 1 */

/**
 * S3M Command
 *
 * This goes in the DOE payload (after S3M_PROXY_COMMAND and
 * CPU_ATTESTAION_COMMAND).
 */

#define SPDM_FIRMWARE_VERSION 0x10

/**
 * SPDM Messages
 */

#define MAX_CERT_CHAIN_LENGTH 2048

/**
 * Constants -- Due to the Algorithm Supported
 */

#define HASH_LENGTH 48
#define SIGNATURE_LENGTH 96

/**
 * Error Response
 */

#define SPDM_ERR 0x7f

#define SPDM_ERR_INVALID_REQUEST        0x01
#define SPDM_ERR_BUSY                   0x03
#define SPDM_ERR_UNEXPECTED_REQUEST     0x04
#define SPDM_ERR_UNSPECIFIED            0x05
#define SPDM_ERR_DECRYPT_ERROR          0x06
#define SPDM_ERR_UNSUPPORTED_REQUEST    0x07
#define SPDM_ERR_REQUEST_IN_FLIGHT      0x08
#define SPDM_ERR_INVALID_RESPONSE_CODE  0x09
#define SPDM_ERR_SESSION_LIMIT_EXCEEDED 0x0a
#define SPDM_ERR_SESSION_REQUIRED       0x0b
#define SPDM_ERR_RESET_REQUIRED         0x0c
#define SPDM_ERR_RESPONSE_TOO_LARGE     0x0d
#define SPDM_ERR_REQUEST_TOO_LARGE      0x0e
#define SPDM_ERR_LARGE_RESPONSE         0x0f
#define SPDM_ERR_MAJOR_VERSION_MISMATCH 0x41
#define SPDM_ERR_RESPONSE_NOT_READY     0x42
#define SPDM_ERR_REQUEST_RESYNC         0X43
#define SPDM_ERR_VENDOR                 0xff

struct spdm_error {
	__u8 version;		/* must be SPDM_FIRMWARE_VERSION */
	__u8 response;		/* must be 0x7f (error) */
	__u8 code;
	__u8 data;		/* 0x00 or reserved unless code is 0xff */
	__u8 ext_data[32];
};

/**
 * Get Version
 */

#define SPDM_GET_VER 0x84
#define SPDM_VER     0x04

struct spdm_ver {
	__u32 :8;
	__u32 version_number_entry_count :8;
	__u32 version_number_entry :16;
};

/**
 * Get Capabilities
 */

#define SPDM_GET_CAPS 0xe1
#define SPDM_CAPS     0x61

struct spdm_caps {
	__u32 detailed_version :8;
	__u32 crypto_tmout_exp :8;
	__u32 :0;
	__u32 flags;
};

#define CAPS_CACHE_CAP                   (1 << 0)
#define CAPS_CERT_CAP                    (1 << 1)
#define CAPS_CHAL_CAP                    (1 << 2)
#define CAPS_MEAS_CAP_S                  3
#define CAPS_MEAS_CAP_M                  (3 << CAPS_MEAS_CAP_S)
#define CAPS_MEAS_CAP_RSP_NO_SUPPORT     0
#define CAPS_MEAS_CAP_RSP_SUPPORT_NO_SIG 1
#define CAPS_MEAS_CAP_RSP_SUPPORT        2
#define CAPS_MEAS_FRESH_CAP              (1 << 5)

/**
 * Negotiate Algorithms
 *
 * For now, just use a preset request and expect a specific response.
 * See neg_algs() in command.c.
 */

#define SPDM_NEG_ALGS 0xe3
#define SPDM_ALGS     0x63

struct spdm_neg_algs_cmd {
	__u32 version :8;
	__u32 code :8;
	__u32 :0;
	__u32 length :16;
	__u32 measurement_specification :8;
	__u32 :0;
	__u32 base_asym_algo;
	__u32 base_hash_algo;
	__u32 reserved[3];
	__u32 ext_asym_count :8;
	__u32 ext_hash_count :8;
	__u32 :0;

	/**
	 * <(4 * ext_asym_count) bytes>: ext_asym
	 * <(4 * ext_hash_count) bytes>: ext_hash
	 * either count can be <= 8, but the sum must also be <=8
	 */
	__u8 data[32];
};

struct spdm_neg_algs_resp {
	__u32 version :8;
	__u32 code :8;
	__u32 :0;
	__u32 length :16;
	__u32 measurement_specification_sel :8;
	__u32 :0;
	__u32 measurement_hash_algo;
	__u32 base_asym_sel;
	__u32 base_hash_sel;
	__u32 reserved[3];
	__u32 ext_asym_sel_count :8;
	__u32 ext_hash_sel_count :8;
	__u32 :0;

	/**
	 * <(4 * ext_asym_sel_count) bytes>: ext_asym_sel
	 * <(4 * ext_hash_sel_count) bytes>: ext_hash_sel
	 * either count can be <= 8, but the sum must also be <=8
	 */
	__u8 data[32];
};

/**
 * Get CSR (Certificate Signing Request)
 */

#define SPDM_GET_CSR      0xed
#define SPDM_GET_CSR_RESP 0x6d

struct spdm_csr {
	__u32 version :8;
	__u32 code :8;
	__u32 :0;

	/**
	 * Get Certificate Signing Request
	 *
	 * After the first word...
	 *   <requester info length> requester info length in bytes, can be 0.
	 *   <opaque data length> opaque data length in bytes, can be 0.
	 *   <requester info>
	 *   <opaque data>
	 *
	 * Response
	 *
	 * After the first word...
	 *   <certificate signing request length> in bytes [16 bits].
	 *   <16 bits unused>
	 *   <certificate signing request> 
	 */

	__u64 data;
};

/**
 * Set Certificate
 */

#define SPDM_SET_CERT      0xee
#define SPDM_SET_CERT_RESP 0x6e

#define SPDM_SET_CERT_SLOT_ID_M GENMASK(3, 0)

struct spdm_cert_chain {
	__u32 length :16;
	__u32 :0;
	__u8 data[];
};

/**
 * Get Digests
 *
 * As the slot mask must be 0x01, the maximum number of digests is 1,
 * and the digest size is 48 bytes, no structure is needed.
 */

#define SPDM_GET_DGSTS 0x81
#define SPDM_DGSTS     0x01

#define MAX_NUM_DGSTS 1
#define DGST_SLOT_MSK 0x01
#define DGST_SIZE     48

/**
 * Get Certificate
 */

#define SPDM_GET_CERT 0x82
#define SPDM_CERT     0x02

#define MAX_CERT_SIZE 0x800

/**
 * Challenge Command
 */

#define SPDM_CHALLENGE      0x83
#define SPDM_CHALLENGE_AUTH 0x03

#define NONCE_LENGTH 32

struct spdm_challenge_resp {
	__u8 chash[HASH_LENGTH];
	__u8 nonce[NONCE_LENGTH];
	__u8 mhash[HASH_LENGTH];
	__u16 opaque_length;
	__u8 signature[SIGNATURE_LENGTH];
};

/**
 * Get Measurements
 */

#define SPDM_GET_MEASUREMENTS     0xe0
#define SPDM_MEASUREMENTS         0x60

/**
 * Prototypes and Macros
 */

extern int debug;
extern int delay;
extern char input[];
extern char output[];

#define PRERR(msg, ...) do { \
	fprintf(stderr, "%s:%d - ", __FILE__, __LINE__); \
	fprintf(stderr, msg , ## __VA_ARGS__); \
} while (0);

int message(struct doe *);

int bad(void);
int get_ver(void);
int get_caps(void);
int neg_algs(void);
int get_csr(void);
int set_crt(void);
int get_dgst(void);
int get_crt(void);
int chlg(void);
int get_mmts(void);

#endif	/* _ATTEST_H_ */
