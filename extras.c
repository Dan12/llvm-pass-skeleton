#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct table_entry {
   int* table;
   int n;
   char* func_name;
   struct table_entry* next;
} table_entry;

table_entry* first = NULL;
table_entry* last = NULL;

void* init_edge_table(int size, char* func_name) {
    // printf("init table for %s\n", func_name);
    table_entry* entry = malloc(sizeof(table_entry));
    int* table = malloc(size*sizeof(int));
    memset(table, 0, size * sizeof(int));

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

void inc_table_entry(table_entry* entry, int i) {
    // printf("increment entry %d of %s\n", i, entry->func_name);
    entry->table[i]++;
}

void print_results() {
    table_entry* next = first;
    while(next != NULL) {
        int i;
        printf("%s:\n", next->func_name);
        for (i = 0; i < next->n; i++) {
            printf("\t%d = %d\n", i, next->table[i]);
        }
        free(next->table);
        table_entry* prev = next;
        next = next->next;
        free(prev);
    }
}

// for each function
// function var: is_init = false, table
// if !is_init:
//    table = create_func_table(func_name, size);
//    is_init = true
// elsewhere:
//   inc_table_entry(table, i)

// create_func_table(size)
// linked_list of tables, on main return -> print_results