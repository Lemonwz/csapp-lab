#include "cachelab.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "getopt.h"

/**
 * Simulation parameters 
 * @param s - Numbers of set index bits
 * @param E - Associativity (number of lines per set)
 * @param b - Number of block bits
 * @param t - Name of the valgrind trace to replay
 */
int s,E,b,S;

int hits, misses, evictions;

char t[1000];

typedef struct
{
    int valid;
    int tag;
    int counter;
}line, *associativity, **cache;

cache simulator = NULL;


void printHelper(){

    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");

}

/** 
 * cache[S][E]
 * S = 2^s is the number of sets
 * E(Associativity) is the number of lines per set
 */
void initSimulator(){

    simulator = (cache)malloc(sizeof(associativity) * S);

    for(int i=0; i<S; ++i){

        simulator[i] = (associativity)malloc(sizeof(line) * E);

        for(int j=0; j<E; ++j){

            simulator[i][j].valid = 0;
            simulator[i][j].tag = -1;
            simulator[i][j].counter = -1;

        }   
    }
}

void updateCounter(){

    for(int i=0; i<S; ++i){

        for(int j=0; j<E; ++j){

            if(simulator[i][j].valid == 1)
                ++simulator[i][j].counter;
        }
    }
}

void execOperation(unsigned int address){

    int tagAddr = address >> (b+s);
    int setIndexAddr = (address >> b) & (0xffffffff >> (64-s));

    int max_counter = 0;
    int max_counter_index = -1;

    // hit
    for(int i=0; i<E; ++i){

        if(simulator[setIndexAddr][i].tag == tagAddr){
            simulator[setIndexAddr][i].counter = 0;
            ++hits;
            return;
        }
    }

    // find empty line in set
    for(int i=0; i<E; ++i){

        if(simulator[setIndexAddr][i].valid == 0){
            simulator[setIndexAddr][i].valid = 1;
            simulator[setIndexAddr][i].tag = tagAddr;
            simulator[setIndexAddr][i].counter = 0;
            ++misses;
            return;
        }
    }

    // find no empty line
    // uses the LRU (least-recently used) replacement policy to choose which cache line to evict.

    ++misses;
    ++evictions;

    for(int i=0; i<E; ++i){
        
        if(simulator[setIndexAddr][i].counter > max_counter){
            max_counter = simulator[setIndexAddr][i].counter;
            max_counter_index = i;
        }
    }

    simulator[setIndexAddr][max_counter_index].tag = tagAddr;
    simulator[setIndexAddr][max_counter_index].counter = 0;
    return;

}

void closeSimulator(){

    for(int i=0; i<S; ++i)
        free(simulator[i]);

    free(simulator);

}

void simulateTrace(){

    FILE *pFile = fopen(t, "r");
    if(pFile == NULL){
        printf("open file error");
        exit(-1);
    }

    char operation;
    unsigned int address;
    int size;

    while (fscanf(pFile, " %c %x,%d", &operation, &address, &size) > 0)
    {
        switch (operation){
        
            // Load Instruction
            case 'I':
                continue;

            // Load Data
            case 'L':
                execOperation(address);
                break;
            
            // M(Modify) = L + S
            case 'M':
                execOperation(address);

            // Store Data
            case 'S':
                execOperation(address);
                break;         
        }

        updateCounter();
    }

    fclose(pFile);

}

int main(int argc, char** argv)
{
    int opt;

    hits = misses = evictions = 0;

    while (-1 != (opt = (getopt(argc, argv, "hvs:E:b:t:"))))
    {
        switch (opt)
        {
        case 'h':
            printHelper();
            break;
        
        case 'v':
            printHelper();
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
            strcpy(t, optarg);
            break;

        default:
            printHelper();
            break;
        }
    }

    if(s<=0 || E<=0 || b<=0 || t==NULL)
        return -1;

    S = 1 << s; // S = 2^s

    initSimulator();
    simulateTrace();
    closeSimulator();
    
    printSummary(hits, misses, evictions);

    return 0;
}
