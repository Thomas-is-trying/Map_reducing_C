#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test3_edge_cases(void) {
    printf("\n=== Test 3: casi limite (file vuoto, righe vuote, riga singola, no newline finale) ===\n");
    system("mkdir -p build/it_input_edge");

    FILE *f_empty = fopen("build/it_input_edge/empty.txt", "w");
    fclose(f_empty);

    FILE *f_blank = fopen("build/it_input_edge/blank_lines.txt", "w");
    fprintf(f_blank, "\n\nword\n\n");
    fclose(f_blank);

    FILE *f_single = fopen("build/it_input_edge/single_line.txt", "w");
    fprintf(f_single, "solo\n");
    fclose(f_single);

    FILE *f_nonewline = fopen("build/it_input_edge/no_newline.txt", "w");
    fprintf(f_nonewline, "lastword");
    fclose(f_nonewline);

    run_instance(it_mapper, it_reducer,
                 "build/it_input_edge", "build/it_output_edge.mro",
                 "build/logs/test3.log.txt", "build/stats/test3.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_edge.mro", "word", &count);
    CHECK(found == 1 && count == 1, "token da righe vuote miste ('word') e' corretto");

    found = find_token_count("build/it_output_edge.mro", "solo", &count);
    CHECK(found == 1 && count == 1, "token da file con una sola riga e' corretto");

    found = find_token_count("build/it_output_edge.mro", "lastword", &count);
    CHECK(found == 1 && count == 1, "token dall'ultima riga senza newline e' corretto");
}
