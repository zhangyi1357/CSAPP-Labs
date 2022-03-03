#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cachelab.h"

#define MAXN 256

int hits = 0, misses = 0, evictions = 0;

void cache_simulation(int s, int E, int b, FILE* fp, bool verbose) {
    char c;
    char buf[MAXN];
    while ((c = fgetc(fp)) != EOF) {
        fgets(buf, MAXN, fp);
        switch (c) {
            case ' ':
                break;
            default:
                continue;
        }
        printf("%s", buf);
    }
}

int main(int argc, char* argv[]) {
    int s, E, b;
    bool verbose = false;
    FILE* trace_fp;
    char c;
    while ((c = getopt(argc, argv, "s:E:b:t:v")) != -1) {
        switch (c) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 't':
                trace_fp = fopen(optarg, "r");
                break;
            default:
                printf("Illegall arg");
                exit(1);
        }
    }
    cache_simulation(s, E, b, trace_fp, verbose);
    fclose(trace_fp);
    printSummary(hits, misses, evictions);
    return 0;
}
