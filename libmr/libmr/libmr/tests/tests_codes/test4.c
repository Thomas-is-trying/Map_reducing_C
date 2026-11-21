#include <stdio.h>
#include <errno.h>
#include "test_utils.h"

void test4_param_validation(void) {
    printf("\n=== Test 4: validazione parametri non validi ===\n");
    mr_attr_t attr;
    CHECK(mr_attr_init(&attr) == 0, "mr_attr_init riesce (setup test 4)");

    errno = 0;
    CHECK(mr_attr_set_mapper_threads(&attr, 0) == -1, "set_mapper_threads(0) viene rifiutato");
    CHECK(errno == EINVAL, "errno e' EINVAL dopo set_mapper_threads(0)");

    errno = 0;
    CHECK(mr_attr_set_reducer_threads(&attr, 0) == -1, "set_reducer_threads(0) viene rifiutato");
    CHECK(errno == EINVAL, "errno e' EINVAL dopo set_reducer_threads(0)");

    errno = 0;
    CHECK(mr_attr_set_queue_size(&attr, 0) == -1, "set_queue_size(0) viene rifiutato");
    CHECK(errno == EINVAL, "errno e' EINVAL dopo set_queue_size(0)");

    errno = 0;
    CHECK(mr_attr_init(NULL) == -1, "mr_attr_init(NULL) viene rifiutato");
    CHECK(errno == EINVAL, "errno e' EINVAL dopo mr_attr_init(NULL)");

    mr_attr_destroy(&attr);
}
