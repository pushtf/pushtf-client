#include <getopt.h>
#include "pushtf-client.h"

char g_verbose	= 0;
char g_debug	= 0;
char g_quiet	= 0;

char *get_errorbuffer() {
  static char *errorbuffer = 0;
  if (!errorbuffer)
    errorbuffer = malloc(CURL_ERROR_SIZE * sizeof(char));
  return (errorbuffer);
}

void curl_config(CURL *curl, char *addr)
{
  curl_easy_setopt(curl, CURLOPT_VERBOSE, g_debug);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, g_debug ? 0 : 1);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, get_errorbuffer());

  curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
  curl_easy_setopt(curl, CURLOPT_URL, addr);
}

void curl_error(CURL *curl, CURLcode res, sheader_fields_t *h)
{
  long http_code = 0;

  /* Check for errors */
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    fprintf(stderr, "%s\n", get_errorbuffer());
    curl_easy_cleanup(curl);
    exit(EXIT_FAILURE);
  } else {
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 200 && res != CURLE_ABORTED_BY_CALLBACK)
      {
	// Ok
      }
    else
      {
	if (h->status_code)
	  fprintf(stderr, "Error: %s\n", h->status_code);
	else
	  fprintf(stderr, "Something goes wrong: error code %ld\n", http_code);
	curl_easy_cleanup(curl);
	free_sheader_fields(h);
	exit(EXIT_FAILURE);
      }
  }
}

void usage(const char *progname)
{
  fprintf(stderr, "push.tf command line client\n"
	  "Usage:\n"
	  "   %1$s [options] file_or_ID\n\n"
	  "Options:\n"
	  "  -d | --debug		debug mode\n"
	  "  -g | --get		get a file\n"
	  "  -h | --help		this help\n"
	  "  -o <filename>		output filename w/ --get\n"
	  "  -q | --quiet		quiet mode\n"
	  "  -v | --verbose	verbose mode\n"
	  "  -V | --version	display components versions\n"
	  "\n"
	  "Push files:\n"
	  "%1$s FILE [FILE ..]\n"
	  "\n"
	  "Get files:\n"
	  "%1$s -g ID [ID ..]\n"
	  "\n",
	  progname);
  exit(EXIT_FAILURE);
}

void version() {
  printf("program version: %s\n", VERSION);
  printf("libcurl version: %s\n", curl_version());
  exit (EXIT_SUCCESS);
}

int main(int ac, char **av)
{
  CURL *curl;
  int c;
  char get_f = 0;
  char *output_filename = 0;

  struct option longopts[] = {
    { "debug", 0, 0, 'd' },
    { "get", 0, 0, 'g'},
    { "help", 0, 0, 'h' },
    { "quiet", 0, 0, 'q' },
    { "verbose", 0, 0, 'v' },
    { "version", 0, 0, 'V' },
    { NULL, 0, 0, 0 }
  };

  while ((c = getopt_long(ac, av, "dvgVho:q",
			  longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 'g':
	  get_f = 1;
	  break;
	case 'v':
	  printf("verbose mode: on\n");
	  g_verbose = 1;
	  break;
	case 'd':
	  printf("debug mode: on\n");
	  g_debug = 1;
	  break;
	case 'V':
	  version();
	  break;
	case 'h':
	  usage(av[0]);
	  break;
	case 'o':
	  output_filename = optarg;
	  break;
	case 'q':
	  g_quiet = 1;
	  break;
	default:
	  usage(av[0]);
	}
    }

  if (optind == ac) {
    // user forgot the input file
    usage(av[0]);
  }

  setbuf(stdout, NULL);

  for (;optind < ac; optind++) {
    curl = curl_easy_init();
    if (get_f)
      get(curl, av[optind], output_filename);
    else
      push(curl, av[optind]);
  }

  exit(EXIT_SUCCESS);
}
