#include "cachelab.h"
#include <getopt.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


int s = -1, E = -1, b = -1;
char *tracefile = NULL;
int verbose = 0;

void print_usage() {
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
}

void parse_addr(uint64_t addr, int s, int b, uint64_t *label, uint64_t *set_index, uint64_t *block_index) {
    uint64_t block_mask = (1ULL << b) - 1;
    *block_index = addr & block_mask;
    uint64_t set_mask = (1ULL << s) - 1;
    *set_index = (addr >> b) & set_mask;
    *label = addr >> (b + s);
}

struct Block {
    bool valid_bit;
    uint64_t label;
    uint64_t timestamp;
};

void print_cache(struct Block *cache, int S, int E) {
    printf("### Cache ###\n");
    for (int i = 0; i < S; ++i) {
        printf("[Set:%d] ", i);
        for (int j = 0; j < E; ++j) {
            printf("(valid=%d label=%lu timestamp=%lu data=whatever),", cache[i * E + j].valid_bit, cache[i * E + j].label, cache[i * E].timestamp);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int opt = -1;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(opt) {
            case 'h':
                print_usage();
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                tracefile = optarg;
                break;
            default:
                print_usage();
                return 1;
        }
    }

    if (s == -1 || E == -1 || b == -1 || tracefile == NULL) {
        print_usage();
        return 1;
    }

    int hit_count = 0, miss_count = 0, eviction_count = 0;
    int S = pow(2, s);
    struct Block *cache = malloc(S * E * sizeof(struct Block));
    for (int i = 0; i < S * E; ++i) {
        cache[i].valid_bit = false;
    }
    uint64_t global_timestamp = 0;

    FILE *trace = fopen(tracefile, "r");
    if (trace == NULL) {
        perror("Error opening file");
        return 114514;
    }

    char line_buffer[50];
    char op;
    uint64_t addr, label, set_index, block_index;
    uint64_t v_len;
    while (fgets(line_buffer, sizeof(line_buffer), trace)) {
        if (sscanf(line_buffer, " %c %lx,%lu", &op, &addr, &v_len) == 3) {
            parse_addr(addr, s, b, &label, &set_index, &block_index);
            switch(op) {
                case 'S':
                case 'L': {
                print_cache(cache, S, E);
                bool vacant = false;
                int e_index = -1;
                bool hit = false;
                for(int i = 0; i < E; ++i) {
                    if (cache[set_index * E + i].valid_bit && cache[set_index * E + i].label == label) {
                        hit = true;
                        cache[set_index * E + i].timestamp = global_timestamp++;
                        break;
                    }
                    if (!vacant) {
                        if (!cache[set_index * E + i].valid_bit) {
                            e_index = i;
                            vacant = true;
                        } else {
                            if (e_index == -1) {
                                e_index = i;
                            } else if (cache[set_index * E + i].timestamp < cache[set_index * E + e_index].timestamp) {
                                e_index = i;
                            }
                        }
                    }
                }
                if (hit) {
                    hit_count++;
                    if (verbose) {
                        printf("%c %lx,%lu hit \n", op, addr, v_len);
                    }
                    break;
                }

                ++miss_count;
                cache[set_index * E + e_index].valid_bit = true;
                cache[set_index * E + e_index].label = label;
                cache[set_index * E + e_index].timestamp = global_timestamp++;
                if (!vacant) {
                    eviction_count++;
                    if (verbose) {
                        printf("%c %lx,%lu miss eviction \n", op, addr, v_len);
                    }
                } else {
                    if (verbose) {
                        printf("%c %lx,%lu miss \n", op, addr, v_len);
                    }
                }
                break;
                }
                case 'M': {
                print_cache(cache, S, E);
                bool vacant = false;
                int e_index = -1;
                bool hit = false;
                for(int i = 0; i < E; ++i) {
                    if (cache[set_index * E + i].valid_bit && cache[set_index * E + i].label == label) {
                        hit = true;
                        cache[set_index * E + i].timestamp = global_timestamp++;
                        break;
                    }
                    if (!vacant) {
                        if (!cache[set_index * E + i].valid_bit) {
                            e_index = i;
                            vacant = true;
                        } else {
                            if (e_index == -1) {
                                e_index = i;
                            } else if (cache[set_index * E + i].timestamp < cache[set_index * E + e_index].timestamp) {
                                e_index = i;
                            }
                        }
                    }
                }
                if (hit) {
                    hit_count = hit_count + 2;
                    if (verbose) {
                        printf("%c %lx,%lu hit hit \n", op, addr, v_len);
                    }
                    break;
                }

                ++miss_count;
                ++hit_count;
                cache[set_index * E + e_index].valid_bit = true;
                cache[set_index * E + e_index].label = label;
                cache[set_index * E + e_index].timestamp = global_timestamp++;
                if (!vacant) {
                    eviction_count++;
                    if (verbose) {
                        printf("%c %lx,%lu miss hit eviction \n", op, addr, v_len);
                    }
                } else {
                    if (verbose) {
                        printf("%c %lx,%lu miss hit \n", op, addr, v_len);
                    }
                }
                break;
                }
            }
        } else {
            if (verbose) {
                fprintf(stderr, "Failed to parse line, should be an I: %s", line_buffer);
            }
        }
    }
    
    printSummary(hit_count, miss_count, eviction_count);
    free(cache);
    return 0;
}
