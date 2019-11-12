#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct table_entry {
   uint64_t* table;
   int n;
   char* func_name;
   struct table_entry* next;
} table_entry;

table_entry* first = NULL;
table_entry* last = NULL;

// create a table for each function and insert it into the 
// ll of function tables
void* init_table(int size, char* func_name) {
    // printf("init table for %s\n", func_name);
    table_entry* entry = malloc(sizeof(table_entry));
    uint64_t* table = malloc(size*sizeof(uint64_t));
    memset(table, 0, size * sizeof(uint64_t));

    // init data
    entry->table = table;
    entry->n = size;
    entry->func_name = func_name;
    entry->next = NULL;

    // insert into ll
    if (first == NULL) {
        first = entry;
        last = entry;
    } else {
        last->next = entry;
        last = entry;
    }
    return entry;
}

// increment index i of the given table
void inc_table_entry(table_entry* entry, int i) {
    // printf("increment entry %d of %s\n", i, entry->func_name);
    entry->table[i]++;
}

// print all the results for each function
void print_results() {
    table_entry* next = first;
    while(next != NULL) {
        int i;
        printf("%s:\n", next->func_name);
        for (i = 0; i < next->n; i++) {
            printf("\t%d = %lld\n", i, next->table[i]);
        }
        free(next->table);
        table_entry* prev = next;
        next = next->next;
        free(prev);
    }
}