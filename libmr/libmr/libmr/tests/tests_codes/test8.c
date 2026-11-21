#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test8_recursive_scan(void) {
    printf("\n=== Test 8: scansione ricorsiva delle sottodirectory (addendum) ===\n");
    system("mkdir -p build/it_input_recursive/sub1/sub2");

    FILE *f_root = fopen("build/it_input_recursive/root.txt", "w");
    fprintf(f_root, "alpha beta\n");
    fclose(f_root);

    FILE *f_sub1 = fopen("build/it_input_recursive/sub1/file_a.txt", "w");
    fprintf(f_sub1, "gamma alpha\n");
    fclose(f_sub1);

    FILE *f_sub2 = fopen("build/it_input_recursive/sub1/sub2/file_b.txt", "w");
    fprintf(f_sub2, "delta beta gamma\n");
    fclose(f_sub2);

    run_instance(it_mapper, it_reducer,
                 "build/it_input_recursive", "build/it_output_recursive.mro",
                 "build/logs/test8.log.txt", "build/stats/test8.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_recursive.mro", "alpha", &count);
    CHECK(found == 1 && count == 2, "token 'alpha' da root e sottocartella di 1 livello");

    found = find_token_count("build/it_output_recursive.mro", "beta", &count);
    CHECK(found == 1 && count == 2, "token 'beta' da root e sottocartella di 2 livelli");

    found = find_token_count("build/it_output_recursive.mro", "gamma", &count);
    CHECK(found == 1 && count == 2, "token 'gamma' da due livelli di sottocartelle diversi");

    found = find_token_count("build/it_output_recursive.mro", "delta", &count);
    CHECK(found == 1 && count == 1, "token 'delta', unico nella sottocartella piu' profonda");
}
