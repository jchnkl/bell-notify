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

  /* pid_t child = forkpty(&master, NULL, NULL, NULL); */
  openpty(&master, &slave, NULL, &tt, &win);

  my_child_proc = fork();

  /* ioctl(master, TIOCSCTTY, 0); */

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

    /* execvp(argv[1], NULL); */
    execvp(argv[2], argv + 2);
    /* execlp("/bin/sh", "sh", (void*)0); */
    /* execvp(argv[1], NULL); */


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

    /* sleep(1); */

    /* int flags = 0; */
    /* flags = fcntl(master, F_GETFL, 0); */
    /* fcntl(master, F_SETFL, flags | O_NONBLOCK); */

    /* const char * cmd = "echo hello\n"; */
    /* write(master, cmd, strlen(cmd)); */

    /* struct termios tios; */
    /* tcgetattr(master, &tios); */
    /* tios.c_lflag &= ~(ECHO | ECHONL); */
    /* tcsetattr(master, TCSAFLUSH, &tios); */

    char buf[BUFSIZ];

    struct pollfd fds[] =
      { { .fd = master, .events = POLLIN }
      , { .fd = STDIN_FILENO, .events = POLLIN }
      };


    /* } while (read_cnt > 0 && errno != EAGAIN && errno != EWOULDBLOCK); */

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

      /* int nread = read(master, buf, BUFSIZ); */
      /*  */
      /* if (nread < 0) { */
      /*   // try again */
      /*   continue; */
      /*  */
      /* } else { */
      /*  */
      /*   char * bell = strchr(buf, 0x7); */
      /*  */
      /*   #<{(| if (bell) { |)}># */
      /*   #<{(|   *bell = 0; |)}># */
      /*   #<{(|   fprintf(stderr, "BELL\n"); |)}># */
      /*   #<{(| } |)}># */
      /*  */
      /*   #<{(| buf[nread] = '\0'; |)}># */
      /*   #<{(| fprintf(STDOUT_FILENO, "%s", buf); |)}># */
      /*   #<{(| fprintf(stderr, "read: \"%s\"\n", buf); |)}># */
      /*   #<{(| fprintf(stderr, "read\n"); |)}># */
      /*   #<{(| for (int i = 0; i < nread; i++) { |)}># */
      /*   #<{(|   putchar(buf[i]); |)}># */
      /*   #<{(| } |)}># */
      /*  */
      /*   write(STDOUT_FILENO, buf, nread); */
      /*  */
      /* } */
      /*  */
      /* nread = read(STDIN_FILENO, buf, BUFSIZ); */
      /* write(master, buf, nread); */

      /* if (nread > -1) { */
      /*   #<{(| buf[nread] = '\0'; |)}># */
      /*   #<{(| fprintf(stderr, "from stdin: %s", buf); |)}># */
      /*   write(master, buf, nread); */
      /* } */

      /* sleep(1); */
      /* break; */
    }

    ca_proplist_destroy(p);
    ca_context_destroy(ca_ctx);

  }

  /* close(master); */
  /* tcsetattr(STDIN_FILENO, TCSADRAIN, &tt); */
  /* tcsetattr(STDIN_FILENO, TCSANOW, &tt); */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);

  kill(0, SIGTERM);
  /* kill(child, SIGTERM); */

  return EXIT_SUCCESS;
}
