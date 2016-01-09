#include "pushtf-client.h"

static size_t get_file(void *ptr, size_t size, size_t nmemb, void *userp)
{
  sfile_t *f = (sfile_t *)userp;
  int rb;

  if (f->h->status_code != 200)
    return(size * nmemb);

  rb = write(f->fd, ptr, (size * nmemb));
  if (rb == -1) {
    perror("output file");
  }

  f->fh_cur_pos += rb;
  progress_bar(f);

  return (rb);
}

int get_free_filename(char *filename, char force)
{
  char *tmp_filename;
  int i;

  if (access(filename, F_OK) == -1 || force)
    return (0);

  tmp_filename = malloc(strlen(filename) + 6);
  for (i = 1; i < 10000; i++) {
    sprintf(tmp_filename, "%s.%i", filename, i);

    if (access(tmp_filename, F_OK) < 0) {
        filename = realloc(filename, strlen(tmp_filename) + 1);
        strcpy(filename, tmp_filename);
        free(tmp_filename);
        return (0);
    }
  }

  fprintf(stderr, "Unable to find free filename for %s\n", filename);
  return (1);
}

int open_file(sfile_t *sfile, char *output_filename, char force)
{
  if (get_free_filename(output_filename, force))
    return (1);

  if ((sfile->fd = open(output_filename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1) {
    perror(output_filename);
    return(1);
  }
  return(0);
}

int get(CURL *curl, char *id, char *output_filename, char force)
{
  CURLcode res;
  char *url;
  char *tmp_filename = 0;
  char *actual_output_filename = 0;
  sfile_t sfile;
  sheader_fields_t h;

  if (output_filename && !strlen(output_filename)) {
      fprintf(stderr, "Error: output filename can't be empty\n");
      return (1);
  }

  tmp_filename = strdup(id);

  url = malloc((strlen(ADDR_BASE) + strlen(id) + 2) * sizeof(char));
  sprintf(url, "%s/%s", ADDR_BASE, id);

  memset(&sfile, 0, sizeof(sfile_t));
  memset(&h, 0, sizeof(sheader_fields_t));
  sfile.h = &h;

  if (output_filename && !strcmp(output_filename, "-")) {
    sfile.fd = 1;
    g_quiet = 1;
  } else if (open_file(&sfile, tmp_filename, 0)) {
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
    curl_error(curl, res, &h);
  }

  close(sfile.fd);

  if (!g_quiet) printf("\n");

  if (output_filename)
    actual_output_filename = strdup(output_filename);
  else if (!output_filename)
    actual_output_filename = strdup(h.filename);

  if (sfile.fd != 1 &&
      !get_free_filename(actual_output_filename, force)) {
    if (rename(tmp_filename, actual_output_filename) == -1) {
        perror("rename");
        fprintf(stderr, "filename should be: %s\n", tmp_filename);
    }
    else if (!g_quiet)
        printf("filename: %s\n", actual_output_filename);
  }

  free_sheader_fields(&h);
  free(url);
  free(tmp_filename);
  free(actual_output_filename);

  return (0);
}
