#include "buffer.h"

void abAppend(struct apbuffer *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == 0) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}
void abFree(struct apbuffer *ab) {
  free(ab->b);
}