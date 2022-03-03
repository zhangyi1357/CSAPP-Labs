#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

#define MAXN 256  // buffer size

int hits = 0, misses = 0, evictions = 0;

void cache_simulation(int s, int E, int b, FILE* fp, bool verbose) {
    // caculate the total line number
    size_t size_of_lines = (1 << s) * E;

    // malloc space for flags
    size_t* flags = malloc(sizeof(*flags) * size_of_lines);

    // malloc space for when line was used and set all of that to 0
    size_t counter = 0;
    size_t* hit_time = malloc(sizeof(size_t) * size_of_lines);
    memset(hit_time, counter, sizeof(size_t) * size_of_lines);

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
        size_t address, size;
        fscanf(fp, "%c %lx, %lx", &type, &address, &size);
        fgets(buf, MAXN, fp);  // read extra character in end of line
        if (verbose)
            printf("%c %lx,%lx ", type, address, size);

        // decomposition of address
        //      flag          set_index   offset
        // bits M - (s + b)   s           b
        size_t set_index = (address >> b) & ((1 << s) - 1);
        size_t flag = address >> (s + b);
        size_t start_line_index = set_index * E;

        // check for all the lines in the set
        size_t least_recently_hit_index = start_line_index, i;
        for (i = 0; i < E; ++i) {
            size_t index = start_line_index + i;
            if (hit_time[index] > 0 && flags[index] == flag)  // hit
                break;
            if (hit_time[index] < hit_time[least_recently_hit_index])
                least_recently_hit_index = index;
        }

        ++counter;
        if (i != E) {  // hit
            ++hits;
            hit_time[start_line_index + i] = counter;
            if (verbose)
                printf("hit");
        } else if (hit_time[least_recently_hit_index] == 0) {  // empty line
            ++misses;
            hit_time[least_recently_hit_index] = counter;
            flags[least_recently_hit_index] = flag;
            if (verbose)
                printf("miss");
        } else {  // eviction
            ++misses;
            ++evictions;
            hit_time[least_recently_hit_index] = counter;
            flags[least_recently_hit_index] = flag;
            if (verbose)
                printf("miss eviction");
        }
        if (type == 'M') {
            ++hits;
            if (verbose)
                printf(" hit");
        }
        if (verbose)
            printf(" \n");

        // printf("set index: %lx, flag: %lx\n", set_index, flag);
    }

    free(hit_time);
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
    // fclose(trace_fp);
    printSummary(hits, misses, evictions);
    return 0;
}
