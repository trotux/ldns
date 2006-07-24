#define _GNU_SOURCE
#include "config.h"

#include <ldns/ldns.h>
#include <pcap.h>

#define SEQUENCE 1
#define QDATA    2
#define ADATA    3
#define EMPTY    0
#define LINES    4

#ifndef HAVE_GETDELIM
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
#endif

struct dns_info
{
	size_t seq;      /* seq number */
	char *qdata;     /* query data in hex */
	char *adata;     /* answer data in hex */
};

void
usage(FILE *fp)
{
	fprintf(fp, "pcat-diff FILE1 [FILE2]\n\n");
	fprintf(fp, "Show the difference between two pcat traces as generated by pcat.\n");
	fprintf(fp, "There are no options, is FILE2 is not given, standard input is read\n");
        fprintf(fp, "\nOUTPUT FORMAT:\n");
        fprintf(fp, "    Line based output format, each record consists of 4 lines:\n");
        fprintf(fp, "    1. xxx:yyy\t\tdecimal sequence numbers\n");
        fprintf(fp, "    2. hex dump\t\tquery in hex, network order\n");
        fprintf(fp, "    3. hex dump\t\tanswer of FILE 1 in hex, network order\n");
        fprintf(fp, "    4. hex dump\t\tanswer of FILE 2 in hex, network order\n\n");
        fprintf(fp, " If a difference in the query is spotted the sequence nmuber\n");
        fprintf(fp, " is prefixed by a 'q: ' and the query data is printed:\n");
        fprintf(fp, "    1. q: xxx:yyy\tdecimal sequence numbers\n");
        fprintf(fp, "    2. hex dump\t\tquery in hex, network order\n");
        fprintf(fp, "    3. hex dump\t\tquery of FILE 1 in hex, network order\n");
        fprintf(fp, "    4. hex dump\t\tquery of FILE 2 in hex, network order\n");
}

void
compare(struct dns_info *d1, struct dns_info *d2)
{
	if (strcmp(d1->qdata, d2->qdata) != 0) {
		fprintf(stderr, "Query differs!\n");
		fprintf(stdout, "q: %d:%d\n%s\n%s\n%s\n", (int)d1->seq, (int)d2->seq, 
			d1->qdata, d1->qdata, d2->qdata);
	} else {
		if (strcmp(d1->adata, d2->adata) != 0) {
			fprintf(stdout, "%d:%d\n%s\n%s\n%s\n", (int)d1->seq, (int)d2->seq, 
				d1->qdata, d1->adata, d2->adata);
		}
	}
}

int
main(int argc, char **argv)
{
	FILE *trace1;
	FILE *trace2;
	size_t i;
	ssize_t read1;
	size_t len1;
	char *line1;
	ssize_t read2;
	size_t len2;
	char *line2;

	struct dns_info d1;
	struct dns_info d2;

	i = 0;
	len1 = 0;
	line1 = NULL;
	len2 = 0;
	line2 = NULL;

	/* need two files */
	switch(argc) {
		case 1:
			usage(stdout);
			/* usage */
			exit(EXIT_FAILURE);
		case 2:
			if (!(trace1 = fopen(argv[1], "r"))) {
				fprintf(stderr, "Cannot open trace file `%s\'\n", argv[1]);
				exit(EXIT_FAILURE);
			}
			trace2 = stdin;
			break;
		case 3:
			if (!(trace1 = fopen(argv[1], "r"))) {
				fprintf(stderr, "Cannot open trace file `%s\'\n", argv[1]);
				exit(EXIT_FAILURE);
			}
			if (!(trace2 = fopen(argv[2], "r"))) {
				fprintf(stderr, "Cannot open trace file `%s\'\n", argv[1]);
				exit(EXIT_FAILURE);
			}
			break;
		default:
			exit(EXIT_FAILURE);
	}

	i = 1;

reread:
	read1 = getdelim(&line1, &len1, '\n', trace1);
	read2 = getdelim(&line2, &len2, '\n', trace2);
	if (read1 == -1 || read2 == -1) {
		fclose(trace1); fclose(trace2);
		exit(EXIT_SUCCESS);
	}
	if (read1 > 0) 
		line1[read1 - 1] = '\0';
	if (read2 > 0)
		line2[read2 - 1] = '\0';

	switch(i % LINES) {
		case SEQUENCE:
			d1.seq = atoi(line1);
			d2.seq = atoi(line2);
			break;
		case QDATA:
			d1.qdata = strdup(line1);
			d2.qdata = strdup(line2);
			break;
		case ADATA:
			d1.adata = strdup(line1);
			d2.adata = strdup(line2);
			break;
		case EMPTY:
			/* we now should have  */
			compare(&d1, &d2);
			free(d1.adata);
			free(d2.adata);
			free(d1.qdata);
			free(d2.qdata);
			break;
	}
	i++;
	goto reread;

	fclose(trace1);
	fclose(trace2);
	return 0;
}
