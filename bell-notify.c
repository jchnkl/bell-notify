#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pty.h>
#include <poll.h>
#include <canberra.h>

int do_resize = 0;
int do_terminate = 0;
pid_t my_child_proc = 0;

void
sig_resize(int sig)
{
  do_resize = 1;
}

void
sig_terminate(int sig, siginfo_t * siginfo, void * ucontext)
{
  if (siginfo->si_pid == my_child_proc) {
    do_terminate = 1;
  }
}

void
ca_callback(ca_context * c, uint32_t id, int error_code, void * playing)
{
  *(int *)playing = 0;
}

int
main(int argc, char ** argv)
{
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sa.sa_handler = sig_resize;
  sigaction(SIGWINCH, &sa, NULL);

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sig_terminate;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGCHLD, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  struct termios tt;
  struct winsize win;

  tcgetattr(STDIN_FILENO, &tt);
  ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &win);

  int master;
  int slave;

  openpty(&master, &slave, NULL, &tt, &win);

  my_child_proc = fork();

  if (my_child_proc < 0) {
    return EXIT_FAILURE;

  } else if (my_child_proc == 0) {
    close(master);

    setsid();
    ioctl(slave, TIOCSCTTY, 0);

    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(slave, STDERR_FILENO);

    close(slave);

    execvp(argv[2], argv + 2);

  } else {

    ca_context * ca_ctx;
    ca_context_create(&ca_ctx);

    ca_proplist * p;
    ca_proplist_create(&p);

    ca_proplist_sets(p, CA_PROP_EVENT_ID, argv[1]);

    int playing = 1;

    struct termios rtt;
    rtt = tt;
    cfmakeraw(&rtt);
    rtt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &rtt);

    char buf[BUFSIZ];

    struct pollfd fds[] =
      { { .fd = master, .events = POLLIN }
      , { .fd = STDIN_FILENO, .events = POLLIN }
      };

    char * bell_chr = NULL;

    while (! do_terminate) {

      if (do_resize) {
        do_resize = 0;
        ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&win);
        ioctl(slave, TIOCSWINSZ, (char *)&win);
      }

      int ns = poll(fds, 2, -1);

      if (ns < 0) {
        continue;

      } else {
        for (int n = 0; n < 2 && ns > 0; ++n) {
          if (fds[n].revents & POLLIN) {
            --ns;
            if (fds[n].fd == master) {
              int nread = read(master, buf, BUFSIZ);
              write(STDOUT_FILENO, buf, nread);
              bell_chr = memchr(buf, 0x7, nread);

            } else {
              int nread = read(STDIN_FILENO, buf, BUFSIZ);
              write(master, buf, nread);
            }
          }
        }
      }

      if (bell_chr) {
        bell_chr = NULL;
        ca_context_play_full(ca_ctx, 1, p, ca_callback, (void *)&playing);
        while (playing);
      }

    }

    ca_proplist_destroy(p);
    ca_context_destroy(ca_ctx);

  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);

  kill(0, SIGTERM);

  return EXIT_SUCCESS;
}
