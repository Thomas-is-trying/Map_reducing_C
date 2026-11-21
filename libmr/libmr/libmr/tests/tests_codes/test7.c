#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test7_single_file_input(void) {
    printf("\n=== Test 7: input come singolo file regolare (non directory) ===\n");
    system("mkdir -p build/it_input_single");
    FILE *f = fopen("build/it_input_single/only.txt", "w");
    fprintf(f, "hello world hello\n");
    fclose(f);

    run_instance(it_mapper, it_reducer,
                 "build/it_input_single/only.txt", "build/it_output_single.mro",
                 "build/logs/test7.log.txt", "build/stats/test7.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_single.mro", "hello", &count);
    CHECK(found == 1 && count == 2, "elaborazione di un singolo file: 'hello' conta 2");

    found = find_token_count("build/it_output_single.mro", "world", &count);
    CHECK(found == 1 && count == 1, "elaborazione di un singolo file: 'world' conta 1");
}
