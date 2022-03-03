#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

#define MAXN 256  // buffer size
#define M 32      // bits of memory address

int hits = 0, misses = 0, evictions = 0;

void cache_simulation(int s, int E, int b, FILE* fp, bool verbose) {
    // malloc space for valid bits and set all bits to 0
    size_t size_of_lines = (1 << s) * E;
    size_t bytes_of_valid_bits = sizeof(char) * size_of_lines / 8 + 1;
    char* valid_bits = malloc(bytes_of_valid_bits);
    memset(valid_bits, 0, bytes_of_valid_bits);
    printf("sizeof valid bits : %lu\n", sizeof(valid_bits));

    // malloc space for flags
    unsigned* flags = malloc(sizeof(unsigned) * size_of_lines);

    // malloc space for lines frequency
    size_t* hit_times = malloc(sizeof(size_t) * size_of_lines);
    memset(hit_times, 0, sizeof(size_t) * size_of_lines);

    // read the file and handle all the instructions
    char buf[MAXN], c;
    while ((c = fgetc(fp)) != EOF) {
        if (c != ' ') {  // ignore the line starting without space (I line)
            // skip this line
            fgets(buf, MAXN, fp);
            continue;
        }
        // read the type and address
        char type;
        size_t address;
        fscanf(fp, "%c %lx", &type, &address);
        fgets(buf, MAXN, fp);  // skip useless ", size" left in the line
        if (verbose)
            printf("%c %lx%s", type, address, buf);

        // decomposition of address
        //      flag          set_index   offset
        // bits M - (s + b)   s           b
        size_t set_index = (address >> b) & ((1 << (s + 1)) - 1);
        size_t flag = address >> (s + b);
        size_t start_line_index = set_index * E;

        size_t not_valid_bit_index = -1, i = 0;
        for (i = 0; i < E; ++i) {
            size_t index = start_line_index + i;
            if (valid_bits[index / 8] & (1 << (index % 8))) {  // valid bit
                if (flags[index] == flag)
                    break;  // hit
            } else {        // not valid bit
                not_valid_bit_index = index;
            }
        }

        if (i != E) {  // hit
            ++hits;
            if (verbose)
                printf("hit");
        } else if (not_valid_bit_index != -1) {  // empty line there
            ++misses;
            valid_bits[not_valid_bit_index / 8] |= (1 << (index % 8));
            flags[not_valid_bit_index] = flag;
            if (verbose)
                printf("miss");
        } else {  // eviction here
            ++misses;
            ++evictions;
        }
        if (type == M)
            ++hits;

        // printf("set index: %lx, flag: %lx\n", set_index, flag);
    }

    free(hit_times);
    free(valid_bits);
    free(flags);
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
