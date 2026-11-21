#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test2_multi_instance(void) {
    printf("\n=== Test 2: istanze multiple nello stesso processo ===\n");
    write_base_input("build/it_input_2");
    FILE *f3 = fopen("build/it_input_2/extra.txt", "w");
    fprintf(f3, "zzzsecondinstancemarker appears only here\n");
    fclose(f3);

    run_instance(it_mapper, it_reducer,
                 "build/it_input_2", "build/it_output_2.mro",
                 "build/logs/test2.log.txt", "build/stats/test2.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_2.mro", "zzzsecondinstancemarker", &count);
    CHECK(found == 1 && count == 1, "token esclusivo della seconda istanza e' presente");

    found = find_token_count("build/it_output_2.mro", "the", &count);
    CHECK(found == 1 && count == 5, "conteggio di 'the' nella seconda istanza e' comunque corretto");

    found = find_token_count("build/it_output_1.mro", "the", &count);
    CHECK(found == 1 && count == 5, "output della prima istanza non alterato dalla seconda");

    found = find_token_count("build/it_output_1.mro", "zzzsecondinstancemarker", &count);
    CHECK(found == 0, "token della seconda istanza non e' finito nell'output della prima");
}
