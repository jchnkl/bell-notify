#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pty.h>
#include <poll.h>

int do_exit = 0;

void
sig_handler(int sig)
{
  do_exit = 1;
}

void wait_for_empty_fd(int fd)
{
  struct pollfd fds[] = {
    { .fd = fd, .events = POLLIN }
  };

  while (poll(fds, 1, 100) == 1);
}

int
main(int argc, char ** argv)
{
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sig_handler;
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

  pid_t child = fork();

  /* ioctl(master, TIOCSCTTY, 0); */

  if (child < 0) {
    fprintf(stderr, "could not fork\n");
    return EXIT_FAILURE;

  } else if (child == 0) {
    close(master);

    setsid();
    ioctl(slave, TIOCSCTTY, 0);

    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(slave, STDERR_FILENO);

    close(slave);

    /* execvp(argv[1], NULL); */
    execvp(argv[1], argv + 1);
    /* execlp("/bin/sh", "sh", (void*)0); */
    /* execvp(argv[1], NULL); */

    fprintf(stderr, "child exited\n");

  } else {

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

    while (! do_exit) {

      int ns = poll(fds, 2, -1);

      if (ns < 0) {
        perror("poll failed\n");

      } else {
        for (int n = 0; n < 2 && ns > 0; ++n) {
          if (fds[n].revents & POLLIN) {
            --ns;
            if (n == 0) {
              int nread = read(master, buf, BUFSIZ);
              write(STDOUT_FILENO, buf, nread);
            } else {
              int nread = read(STDIN_FILENO, buf, BUFSIZ);
              write(master, buf, nread);
            }
          }
        }
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

  }

  /* close(master); */
  fprintf(stderr, "\ntcsetattr before kill\n");
  /* tcsetattr(STDIN_FILENO, TCSADRAIN, &tt); */
  /* tcsetattr(STDIN_FILENO, TCSANOW, &tt); */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);

  kill(0, SIGTERM);
  /* kill(child, SIGTERM); */

  return EXIT_SUCCESS;
}
