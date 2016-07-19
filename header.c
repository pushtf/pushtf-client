#include "pushtf-client.h"

size_t get_header(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  sheader_fields_t *h = (sheader_fields_t *)userdata;
  char *buffer;
  char *str;
  char free_str = 1;
  size_t total_size = size * nmemb;

  buffer = calloc((total_size + 1), sizeof(char));
  str = calloc((total_size + 1), sizeof(char));
  memcpy(buffer, ptr, total_size);

  if (sscanf(buffer, "HTTP/1.1 %i %[^\r\n]", &h->status_code, str) == 2) {
    h->status_reason = str;
    free_str = 0;
  }

  if (sscanf(buffer, "Content-Disposition: attachment; filename=\"%[^\"\r\n]", str) == 1) {
    h->filename = str;
    free_str = 0;
  }

#if defined(__i386__) || defined(__arm__) || defined(__APPLE__)
  if (sscanf(buffer, "Content-length: %llu", &h->content_length) == 1) {
#else
  if (sscanf(buffer, "Content-length: %lu", &h->content_length) == 1) {
#endif
      /* Silent block */
  }

  if (free_str)
    free(str);
  free(buffer);

  return (total_size);
}

void free_sheader_fields(sheader_fields_t *s)
{
  if (s->status_reason)
    free(s->status_reason);
  if (s->filename)
    free(s->filename);
}
