/* 
 * File:   hash.h
 * Author: dreamszhu
 *
 * Created on 2009��12��5��, ����11:05
 */

#ifndef _HASH_H
#define	_HASH_H

#ifdef	__cplusplus
extern "C" {
#endif
    typedef struct hash_item {
        int isdel;
        char *key;
        void *data;
        struct hash_item *next;
    } hash_item;

    hash_item **hash_table;
    unsigned int BKDRHash(const char *str);
    int hash_create(int size);
    void hash_destroy();
    hash_item *hash_search(const char *key);
    int hash_add(hash_item *item);
    int hash_del(const char *key, long data);
#ifdef	__cplusplus
}
#endif

#endif	/* _HASH_H */
