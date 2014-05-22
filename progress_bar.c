#include "pushtf-client.h"

typedef struct {
  double	b;
  char		*unit;
} human_unit_t;

static void	bytes_to_unit(human_unit_t *h, off_t bytes)
{
  char *units[] = {"B", "KB", "MB", "GB", "TB"};
  char p = 0;

  h->b = bytes;

  if (bytes)
    while (h->b/1024 > 1 && p < 4) {
      h->b /= 1024;
      p++;
    }

  h->unit = units[(unsigned char)p];
}

static void print_eta(time_t sec)
{
  long hr, min, t;
  hr = sec/3600;
  t = sec%3600;
  min = t/60;
  sec = t%60;

  if (hr) {
    printf("%02ld:%02ld.%02ld", hr, min, sec);
  } else if (min) {
    printf("00:%02ld.%02ld", min, sec);
  } else if (sec) {
    printf("%2ld sec", sec);
  }
}

static void print_progress_bar(int percent, off_t pos, unsigned long speed, time_t eta)
{
  human_unit_t h;
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  int i;
  
  int progress_bar_length = w.ws_col - 7 - 38;
  int prgbar_stars = (int)((percent * progress_bar_length) / 100);

  printf("\033[2K\r%3d %% [", percent);

  for (i = 0; i++ < prgbar_stars; printf("*"));
  for (; i++ < progress_bar_length; printf(" "));
  
  if (percent == 100) {
    printf("] %lu bytes", pos);
  } else {
    
    bytes_to_unit(&h, pos);
    printf("] %.3f %s", h.b, h.unit);
    
    if (speed) {
      bytes_to_unit(&h, speed);
      printf(" - %.0f %s/s", h.b, h.unit);
    }
    
    if (eta) {
      printf(" ETA ");
      print_eta(eta);
    }
  }
}

void	progress_bar(sfile_t *f)
{
  time_t		tick;
  unsigned long		speed;
  int			percent;
  human_unit_t		h;
  off_t			filesize = 0;

  if (g_quiet)
    return;

  if (f->fd)
    filesize = f->st.st_size ? f->st.st_size : f->h->content_length;
  
  tick = time(NULL);

  if (f->fh_cur_pos == filesize) {
    print_progress_bar(100, f->fh_cur_pos, 0, 0);
  } 
  else 
    if (f->speedometer == 0 || tick >= f->pb_timer + 1)
      {
	f->pb_timer = time(NULL);

	speed = f->fh_cur_pos - f->speedometer;
	f->speedometer = f->fh_cur_pos;

	if (filesize) {
	  percent = f->fh_cur_pos * 100 / filesize;
	  
	  print_progress_bar(percent, f->fh_cur_pos, speed,
			     (filesize - f->fh_cur_pos) / speed);
	} else {
	  bytes_to_unit(&h, speed);
	  printf("\033[2K\r%lu bytes - %.0f %s/s", f->fh_cur_pos, h.b, h.unit);
	}
      }
}

