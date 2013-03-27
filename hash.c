#include "MyleftServer.h"

static int hash_size = 0;
// BKDR Hash Function

unsigned int BKDRHash(const char *str) {
    log_write(LOG_DEBUG, "BKDRHash%s,%d", __FILE__, __LINE__);
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str){
        hash=hash*seed+(*str++);
    }
    return (hash & 0x7FFFFFFF);
}

int hash_create(int size) {
    hash_size = size;
    hash_table = (hash_item **) malloc(size * sizeof (hash_item*));
    if (hash_table == NULL) return 0;
    int i;
    for (i = 0; i < size; i++) {
        hash_table[i] = NULL;
    }
    return 1;
}

void hash_destroy() {
    int i;
    for (i = 0; i < hash_size; i++) {
        if (hash_table[i] != NULL) free(hash_table[i]);
    }
    if (hash_table != NULL) free(hash_table);
}

hash_item *hash_search(const char *key) {
    log_write(LOG_DEBUG, "hash_search key:%s, %s,%d", key, __FILE__, __LINE__);
    if (hash_size > 0) {
        log_write(LOG_DEBUG, "hash_search2%s,%d", __FILE__, __LINE__);
        unsigned int index = BKDRHash(key) % hash_size;

        hash_item *item = hash_table[index];
        while (item != NULL) {
            if (strcmp(item->key, key) == 0) {
                log_write(LOG_DEBUG, "hash_search found,%s,%d", __FILE__, __LINE__);
                return item;
            }
            item = item->next;
        }
    }
    log_write(LOG_DEBUG, "hash_search end,%s,%d", __FILE__, __LINE__);
    return NULL;
}

int hash_add(hash_item *item) {
    int ret = 0;
    log_write(LOG_DEBUG, "hashlock%s,%d", __FILE__, __LINE__);
    if (item != NULL) {
        pthread_mutex_lock(&t_mutex_hash);
        log_write(LOG_DEBUG, "hash_add%s,%d", __FILE__, __LINE__);
        unsigned int index = BKDRHash(item->key) % hash_size;
        hash_item *found_item = hash_table[index];
        hash_item *prev = NULL;

        while (found_item != NULL) {
            prev = found_item;
            found_item = found_item->next;
        }

        log_write(LOG_DEBUG, "hash_add2%s,%d", __FILE__, __LINE__);
        if (prev != NULL) {
            prev->next = item;
        } else {
            hash_table[index] = item;
        }
        ret = 1;
        pthread_mutex_unlock(&t_mutex_hash);
    }    
    return ret;
}

int hash_del(const char *key, long fd) {
    
    int ret = 0;
    log_write(LOG_DEBUG, "hash_del%s,%d", __FILE__, __LINE__);
    if (key != NULL) {
        pthread_mutex_lock(&t_mutex_hash);
        log_write(LOG_DEBUG, "hash_del1%s,%d", __FILE__, __LINE__);
        unsigned int index = BKDRHash(key) % hash_size;
        hash_item *item = hash_table[index];
        hash_item *prev = NULL;
        while (item != NULL) {
            if (strcmp(item->key, key) == 0 && (long)item->data==fd) {
                if (prev != NULL) {
                    prev->next = item->next;
                } else {
                    hash_table[index] = item->next;
                }
                free(item);
                ret = 1;
                break;
            }

            prev = item;
            item = item->next;
        }
        pthread_mutex_unlock(&t_mutex_hash);
    }
    log_write(LOG_DEBUG, "hash_del end%s,%d", __FILE__, __LINE__);
    return ret;
}
