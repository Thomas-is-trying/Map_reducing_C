#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test9_zero_size(void) {
    printf("\n=== Test 9: value/result di dimensione 0 con puntatore NULL ===\n");
    system("mkdir -p build/it_input_zero");
    FILE *f = fopen("build/it_input_zero/lines.txt", "w");
    fprintf(f, "x\ny\nz\n");
    fclose(f);

    run_instance(zero_mapper, zero_reducer,
                 "build/it_input_zero", "build/it_output_zero.mro",
                 "build/logs/test9.log.txt", "build/stats/test9.stats.txt");

    void *data;
    size_t len;

    int r0 = get_nth_matching_record("build/it_output_zero.mro", "marker0", 0, &data, &len);
    CHECK(r0 == 1 && len == sizeof(int) && data != NULL && *(int *)data == 3,
          "value di dimensione 0 ricevuti e raggruppati correttamente (values_count=3)");
    if (r0 == 1 && data) { free(data); }

    int r1 = get_nth_matching_record("build/it_output_zero.mro", "marker0", 1, &data, &len);
    CHECK(r1 == 1 && len == 0, "result di dimensione 0 (data NULL) accettato e scritto correttamente");
    if (r1 == 1 && data) { free(data); }
}
