#ifndef MR_HASH_UTILITIES_H
#define MR_HASH_UTILITIES_H

#include <sys/types.h>

/* [ADDENDUM] hash di default usata se l'utente non ne specifica una
 * con mr_attr_set_hash_function(). */
size_t mr_default_hash(const char *token, size_t token_len, void *user_arg);
#endif
