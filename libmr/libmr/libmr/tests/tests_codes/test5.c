#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test5_determinism(void) {
    printf("\n=== Test 5: determinismo dell'output ===\n");
    run_instance(it_mapper, it_reducer,
                 "build/it_input_1", "build/it_output_5a.mro",
                 "build/logs/test5a.log.txt", "build/stats/test5a.stats.txt");
    run_instance(it_mapper, it_reducer,
                 "build/it_input_1", "build/it_output_5b.mro",
                 "build/logs/test5b.log.txt", "build/stats/test5b.stats.txt");

    int eq = files_equal("build/it_output_5a.mro", "build/it_output_5b.mro");
    CHECK(eq == 1, "due esecuzioni sullo stesso input producono output identico byte per byte");
}
