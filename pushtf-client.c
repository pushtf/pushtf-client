#include <getopt.h>
#include <ctype.h>
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
    if (http_code == 200 && res != CURLE_ABORTED_BY_CALLBACK) {
        /* Ok */
    } else {
	     if (h->status_code)
	       fprintf(stderr, "Error: %i %s\n", h->status_code, h->status_reason);
	     else
	       fprintf(stderr, "Something goes wrong: error code %ld\n", http_code);
	     curl_easy_cleanup(curl);
	     free_sheader_fields(h);
	     exit(EXIT_FAILURE);
    }
  }
}

char validate_numeric_parameter(char *param)
{
  int i;
  for (i = 0; param[i]; ++i)
    if (!isdigit(param[i]))
      return (1);
  return (0);
}

void usage(const char *progname)
{
  fprintf(stderr, "push.tf command line client\n"
	  "Usage:\n"
	  "   %1$s [options] file_or_ID\n\n"
	  "Options:\n"
	  "  -d | --debug       debug mode\n"
	  "  -e <expiration>    set file expiration in hours\n"
	  "  -f                 force file overwriting\n"
	  "  -g | --get         get a file\n"
	  "  -h | --help        this help\n"
	  "  -m <value>         set file maximum downloads\n"
	  "  -o <filename>      output filename w/ --get\n"
	  "  -q | --quiet       quiet mode\n"
	  "  -u                 turn on hardened url mode\n"
	  "  -v | --verbose     verbose mode\n"
	  "  -V | --version     display components versions\n"
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
  char *expiration = 0;
  char *maxdl = 0;
  char hardened = 0;
  char force = 0;

  struct option longopts[] = {
    { "debug", 0, 0, 'd' },
    { "get", 0, 0, 'g'},
    { "help", 0, 0, 'h' },
    { "quiet", 0, 0, 'q' },
    { "verbose", 0, 0, 'v' },
    { "version", 0, 0, 'V' },
    { NULL, 0, 0, 0 }
  };

  while ((c = getopt_long(ac, av, "de:vgVhm:o:quf",
			  longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 'g':
	  get_f = 1;
	  break;
	case 'e':
	  expiration = optarg;
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
	case 'm':
	  maxdl = optarg;
	  break;
	case 'o':
	  output_filename = optarg;
	  break;
	case 'q':
	  g_quiet = 1;
	  break;
	case 'u':
	  hardened = 1;
	  break;
	case 'f':
	  force = 1;
	  break;
	default:
	  usage(av[0]);
	}
    }

  if (optind == ac) {
    /* user forgot the input file */
    usage(av[0]);
  }

  if (maxdl && validate_numeric_parameter(maxdl)) {
    printf("Error: maximum dowload parameter is not valid\n");
    exit(1);
  }

  if (expiration && validate_numeric_parameter(expiration)) {
    printf("Error: expiration parameter is not valid\n");
    exit(1);
  }

  setbuf(stdout, NULL);

  for (;optind < ac; optind++) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (get_f)
      get(curl, av[optind], output_filename, force);
    else
      push(curl, av[optind], hardened, maxdl, expiration);
  }

  exit(EXIT_SUCCESS);
}
