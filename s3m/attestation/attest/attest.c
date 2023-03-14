/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Intel(R) CPU Attestation Sample Code
 *
 * Copyright(c) 2022 Intel Corporation
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/limits.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "attest.h"
#include "oobmsm-dev.h"

int debug;
int delay;
char input[PATH_MAX];
char output[PATH_MAX];

static char command_help_text[] =
	"\n"
	"## Command Help ##\n"
	"##################\n\n"
	"## bad ##\n"
	"#########\n"
	"Send a bad command.\n\n"
	"## get_ver ##\n"
	"#############\n"
	"Send the GetVersion command and display the response.\n\n"
	"## get_caps ##\n"
	"##############\n"
	"Send the GetCaps command and display the response.\n\n"
	"## neg_algs ##\n"
	"##############\n"
	"Send the Negotiate Algorithms command and display the response.\n\n"
	"## get_csr ##\n"
	"#############\n"
	"Get a certificate signing request and dsiplay it.\n"
	"    Supports the output option.\n\n"
	"## set_crt ##\n"
	"#############\n"
	"Set the certificate.\n"
	"    Supports the input option as follows, digest:certificate.\n\n"
	"## get_dgst ##\n"
	"##############\n"
	"Get the digest.\n"
	"    Supports the output option.\n\n"
	"## get_crt ##\n"
	"#############\n"
	"Get the certificate chain.\n"
	"    Supports the output option as follows, digest:certificate.\n\n"
	"## chlg ##\n"
	"##########\n"
	"Challenge.\n"
	"    Supports the output option.\n\n"
	"## get_mmts ##\n"
	"##############\n"
	"Get measurements.\n"
	"    Supports the input option as follows, if a file is given,\n"
	"        it will be used to fill in the nonce value.\n"
	"    Supports the output option as follows,\n"
	"        <directory for measurements>:nonce:signature\n\n";

static void command_help(void) {
	printf("%s", command_help_text);

	exit(0);
}

static const char *sopts = "c:dehi:o:v";
static const struct option lopts[] = {
	{"command", required_argument, NULL, 'c'},
	{"debug",   no_argument,       NULL, 'd'},
	{"delay",   no_argument,       NULL, 'e'},
	{"help",    no_argument,       NULL, 'h'},
	{"input",   required_argument, NULL, 'i'},
	{"output",  required_argument, NULL, 'o'},
	{"version", required_argument, NULL, 'v'}
};

static void usage(int exit_code)
{
	printf("attest: [-c|--command <command>] [-d|--debug] [-h|--help] [-e|--delay] [-v]\n"
	       "  -c|--command : Process the given command (-c help for details).\n"
	       "  -d|--debug   : Output extra debug information.\n"
	       "  -e|--delay   : Sleep for 10s to allow timeout testing.\n"
	       "  -h|--help    : Output this help text!\n"
	       "  -i|--input   : Read from file(s).  [attest -c help for details]\n"
	       "  -o|--output  : Write to file. [attest -c help for details]\n"
	       "  -v|--version : Display the version.\n");

	exit(exit_code);
}

int main(int argc, char *argv[])
{
	int opt, rc = 0;
	char *command = NULL;

	while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'c':
			command = strdup(optarg);

			if (!strncmp(command, "help", strlen("help")))
				command_help();

			break;
		case 'd':
			debug = 1;
			break;
		case 'e':
			delay = 1;
			break;
		case 'h':
			usage(0);
			break;
		case 'i':
			if (strlen(optarg) > PATH_MAX) {
				PRERR("Argument for 'input' is too long!\n");
				usage(1);
			}
			strcpy(input, optarg);
			break;
		case 'o':
			if (strlen(optarg) > PATH_MAX) {
				PRERR("Argument for 'output' is too long!\n");
				usage(1);
			}
			strcpy(output, optarg);
			break;
		case 'v':
			printf("%s\n", VERSION);
			return 0;
		default:
			break;
		}
	}

	if (!command) {
		PRERR("No Command Given!\n");
		return 0;
	}

	if (0 == strncmp(command, "bad", strlen("bad")))
		rc = bad();
	else if (0 == strncmp(command, "get_ver", strlen("get_ver")))
		rc = get_ver();
	else if (0 == strncmp(command, "get_caps", strlen("get_caps")))
		rc = get_caps();
	else if (0 == strncmp(command, "neg_algs", strlen("neg_algs")))
		rc = neg_algs();
	else if (0 == strncmp(command, "get_csr", strlen("get_csr")))
		rc = get_csr();
	else if (0 == strncmp(command, "set_crt", strlen("set_crt")))
		rc = set_crt();
	else if (0 == strncmp(command, "get_dgst", strlen("get_dgst")))
		rc = get_dgst();
	else if (0 == strncmp(command, "get_crt", strlen("get_crt")))
		rc = get_crt();
	else if (0 == strncmp(command, "chlg", strlen("chlg")))
		rc = chlg();
	else if (0 == strncmp(command, "get_mmts", strlen("get_mmts")))
		rc = get_mmts();
	else
		PRERR("Unhandled Command: %s", command);

	free(command);

	return rc;
}
