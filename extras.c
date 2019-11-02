#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int* edge_table;
int num_edges;

void init_edge_table(long long n) {
    num_edges = n;
    edge_table = (int*)malloc(num_edges * sizeof(int));
    memset(edge_table, 0, num_edges * sizeof(int));
}

void took_edge(long long edge) {
    edge_table[edge]++;
}

void print_results() {
    long long i;
    for (i = 0; i < num_edges; i++) {
        printf("%lld = %d", i, edge_table[i]);
    }
    free(edge_table);
}