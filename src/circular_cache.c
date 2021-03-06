#include  "circular_cache.h"



struct circular_cache *circular_cache_find(struct circular_cache *list, char *key, int length) {
  assert(list > 0);
  assert(key);
  assert(length > 0);
  int i = 0;
  for(i = 0; i < length; i++) {
    if(list[i].key) {
      if(strcmp(key, list[i].key) == 0) {
        return &list[i];
      }
    }
  }
  return NULL;
}

int circular_cache_add(struct circular_cache *list, char *key, void *value, int length)  {
  assert(list > 0);
  assert(key);
  assert(value);
  assert(length > 0);
  struct circular_cache tmp = {strdup(key), value};
  int i = 0;
  for(i = 0; i < length; i++) {
    if(!list[i].key) {
      list[i] = tmp;
      return 0;
    }
  }
  return -1;
}

void circular_cache_cleanup(struct circular_cache *list, int length, circular_cache_cleanup_cb func) {
  assert(list > 0);
  assert(length > 0);

  int i = 0;
  srand(time(NULL));
  for(i = 0; i < (length + 1) / 2; i++) {
    int id = rand() % length;
    if(list[id].key) {
      func(&list[id]);
      free(list[id].key);
      list[id].key = NULL;
    }
  }
}
