#include <curl/curl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libgen.h>

#ifndef PUSHTF_CLI_H
#define PUSHTF_CLI_H

#define VERSION		"1.0-beta-14"
#define USERAGENT	"pushtf-client/1.0-beta-14"
#define ADDR_BASE	"http://push.tf"
#define ADDR_ID		"http://push.tf/id"
#define ADDR_UPLOAD	"http://u.push.tf"

#define UPLOAD_SLICE	1024000

// global boolean
extern char g_verbose;
extern char g_debug;
extern char g_quiet;

typedef struct {
  char			*status_code;
  char			*filename;
  unsigned long		content_length;
} sheader_fields_t;

typedef struct {
  char			*id;
  char			*token;
  int			fd;
  struct stat		st;
  time_t		pb_timer;
  off_t			fh_cur_pos;
  off_t			fh_st_pos; // position in the file at the beginning of the slice
  unsigned long		eof;
  unsigned long		speedometer;
  sheader_fields_t	*h;
} sfile_t;

extern char g_verbose;
extern char g_debug;
extern char g_quiet;

void	curl_config(CURL *curl, char *addr);
void	curl_error(CURL *curl, CURLcode res, sheader_fields_t *h);
size_t	get_header(void *ptr, size_t size, size_t nmemb, void *userdata);
void	free_sheader_fields(sheader_fields_t *s);
int	get(CURL *curl, char *id, char *output_filename);
int	push(CURL *curl, char *filename);
void	progress_bar(sfile_t *f);

#endif /* PUSHTF_CLI_H */

