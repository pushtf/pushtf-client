#include "pushtf-client.h"

static size_t get_id(void *ptr, size_t size, size_t nmemb, void *userp)
{
  sfile_t *f = (sfile_t *)userp;
  size_t len = size * nmemb;

  f->id = calloc((++len), sizeof(char));
  f->token = calloc((len), sizeof(char));

  if (sscanf(ptr, "%[A-Za-z0-9]:%s\r\n", f->id, f->token) != 2) {
    fprintf(stderr, "Unable to get file id.\n");
    exit(EXIT_FAILURE);
  }

  return (size * nmemb);
}

static size_t get_post_response(void *ptr, size_t size, size_t nmemb, void *userp)
{
  (void)ptr;
  return (size * nmemb);
}

static size_t post_read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  sfile_t *f = (sfile_t *)userp;
  ssize_t rb;

  if (size * nmemb < 1)
    return (0);

  if (f->fh_cur_pos - f->fh_st_pos >= UPLOAD_SLICE)
    return (0);

  rb = read(f->fd, ptr, (size * nmemb));

  if (rb == -1)
    perror("read");

  f->fh_cur_pos += rb;
  f->eof = rb;

  if (rb)
    progress_bar(f);

  return (rb);
}

static struct curl_httppost* post_formadd(sfile_t *f, char *filename)
{
   struct curl_httppost* post = NULL;
   struct curl_httppost* last = NULL;
   char seek[20];

   curl_formadd(&post, &last,
		CURLFORM_COPYNAME, "id",
		CURLFORM_COPYCONTENTS, f->id,
		CURLFORM_END);
   curl_formadd(&post, &last,
		CURLFORM_COPYNAME, "token",
		CURLFORM_COPYCONTENTS, f->token,
		CURLFORM_END);
   if (f->fh_st_pos > 0) {
#if defined(__i386__) || defined(__arm__) || defined(__APPLE__)
     sprintf(seek, "%lld", f->fh_st_pos);
#else
     sprintf(seek, "%ld", f->fh_st_pos);
#endif
     curl_formadd(&post, &last,
		  CURLFORM_COPYNAME, "seek",
		  CURLFORM_COPYCONTENTS, seek,
		  CURLFORM_END);
   }
   curl_formadd(&post, &last,
		CURLFORM_COPYNAME, "upload_file",
		CURLFORM_FILENAME, basename(filename),
		CURLFORM_STREAM, f,
		CURLFORM_END);
   return (post);
}

char *get_get_id_url(char hardened, char *maxdl, char *expiration)
{
  char *parameters;
  char *param_t[3];
  char *url;
  int param_p = 0;
  int i;

  if (hardened) {
    param_t[param_p] = malloc(strlen("mode=hardened") + 1);
    strcpy(param_t[param_p++], "mode=hardened");
  }
  if (maxdl) {
    param_t[param_p] = malloc(strlen("maxdl=") + strlen(maxdl) + 1);
    strcpy(param_t[param_p], "maxdl=");
    strcat(param_t[param_p++], maxdl);
  }
  if (expiration) {
    param_t[param_p] = malloc(strlen("expiration=") + strlen(expiration) + 1);
    strcpy(param_t[param_p], "expiration=");
    strcat(param_t[param_p++], expiration);
  }

  parameters = malloc(2);
  strcpy(parameters, "?");
  for (i = 0; i < param_p; i++) {
    parameters = realloc(parameters, strlen(parameters) +
			 strlen(param_t[i]) + 1 + (i ? 1 : 0));
    if (i)
      strcat(parameters, "&");
    strcat(parameters, param_t[i]);
    free(param_t[i]);
  }

  url = malloc(strlen(ADDR_ID) + 1);
  strcpy(url, ADDR_ID);
  if (param_p) {
    url = realloc(url, strlen(url) + strlen(parameters) + 1);
    strcat(url, parameters);
  }
  free(parameters);

  return (url);
}

int push (CURL *curl, char *filename, char hardened, char *maxdl, char *expiration)
{
  CURLcode res;
  struct curl_httppost* post = NULL;
  struct curl_slist *chunk = NULL;
  sfile_t *f;
  sheader_fields_t h;
  char *id_url;

  f = malloc(sizeof(sfile_t));
  memset(&h, 0, sizeof(sheader_fields_t));

  if (!strcmp(filename, "-")) {
    filename = 0;
    f->st.st_size = 0;
  } else if (stat(filename, &f->st) == -1) {
    perror(filename);
    curl_easy_cleanup(curl);
    return(1);
  }

  if (filename && S_ISDIR(f->st.st_mode)) {
    fprintf(stderr, "%s is a directory\n", filename);
    curl_easy_cleanup(curl);
    return(1);
  }

  if (curl) {

    /***** GET ID *****/
    id_url = get_get_id_url(hardened, maxdl, expiration);
    if (g_debug)
      printf("id url : %s\n", id_url);

    curl_config(curl, id_url);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_id);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, get_header);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &h);

    res = curl_easy_perform(curl);
    curl_error(curl, res, &h);

    free(id_url);

    if (g_verbose) {
      printf("--> ID: %s\n", f->id);
      printf("--> TOKEN: %s\n", f->token);
    }

    /** RESET **/
    curl_easy_reset(curl);
    memset(&h, 0, sizeof(sheader_fields_t));

    /***** UPLOAD FILE *****/
    curl_easy_setopt(curl, CURLOPT_URL, ADDR_UPLOAD);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
      
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, post_read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, f);
      
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_post_response);
      
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, get_header);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &h);
      
    chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
    chunk = curl_slist_append(chunk, "Expect:");
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    if (g_verbose) {
      printf("file size: %i\n", (int)f->st.st_size);
      if (f->st.st_size > UPLOAD_SLICE)
	printf("This file has to be sent sliced\n");
    }

    if (!filename) {
      f->fd = 0;
    } else {
      f->fd = open(filename, O_RDONLY);
      if (f->fd == -1) {
	perror(filename);
	curl_easy_cleanup(curl);
	exit(EXIT_FAILURE);
      }
    }

    f->fh_cur_pos = f->fh_st_pos = f->speedometer = 0;
    f->eof = 1;

    f->pb_timer = time(NULL);
      
    while (f->eof > 0) {

      /* Create multipart form */
      post = post_formadd(f, filename ? filename : "stdin");
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, post_read_callback);

      /* Perform the request, res will get the return code */
      res = curl_easy_perform(curl);
      curl_error(curl, res, &h);

      f->fh_st_pos = f->fh_cur_pos;

      curl_formfree(post);
    }
    curl_slist_free_all(chunk);

    if (!g_quiet)
      printf("\n");

    printf("%s : %s/%s\n", filename, ADDR_BASE, f->id);

    /* always cleanup */
    curl_easy_cleanup(curl);

    free_sheader_fields(&h);

    close(f->fd);
    free(f);
  }

  return (0);
}
