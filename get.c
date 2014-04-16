#include "pushtf-client.h"

static size_t get_file(void *ptr, size_t size, size_t nmemb, void *userp)
{
  sfile_t *f = (sfile_t *)userp;
  int rb;

  rb = write(f->fd, ptr, (size * nmemb));
  if (rb == -1) {
    perror("output file");
  }

  f->fh_cur_pos += rb;
  progress_bar(f);
  
  return (rb);
}

int get (CURL *curl, char *id, char *output_filename)
{
  CURLcode res;
  char *url;
  sfile_t sfile;
  sheader_fields_t h;

  url = malloc((strlen(ADDR_BASE) + strlen(id) + 2) * sizeof(char));
  sprintf(url, "%s/%s", ADDR_BASE, id);

  memset(&sfile, 0, sizeof(sfile_t));
  memset(&h, 0, sizeof(sheader_fields_t));
  sfile.h = &h;

  if (output_filename && !strcmp(output_filename, "-")) {
    sfile.fd = 1;
    g_quiet = 1;
  } else if ((sfile.fd = open(id, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1) {
    perror(id);
    curl_easy_cleanup(curl);
    exit(EXIT_FAILURE);
  }

  if (curl) {
    curl_config(curl, url);

    /* setup redirection capabilities */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3);
      
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sfile);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, get_header);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &h);

    res = curl_easy_perform(curl);
    printf("\n");
    curl_error(curl, res, &h);
  }

  if (!output_filename || strcmp(output_filename, "-")) {
    if (output_filename && strlen(output_filename)) {
      if (rename(id, output_filename) == -1) {
	perror("rename");
      }
      printf("filename : %s\n", output_filename);
    } else if (h.filename && strlen(h.filename)) {
      if (rename(id, h.filename) == -1) {
	perror("rename");
      }
      printf("filename : %s\n", h.filename);
    } else {
      printf("filename : %s\n", id);
    }
  }

  free_sheader_fields(&h);
  close(sfile.fd);

  free(url);

  return (0);
}
