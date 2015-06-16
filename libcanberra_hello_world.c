#include <stdio.h>
#include <glib.h>
#include <canberra.h>

void
ca_callback(ca_context * c, uint32_t id, int error_code, void * playing)
{
  *(int *)playing = 0;
  fprintf(stderr, __PRETTY_FUNCTION__);
}

int
 main(int argc, char ** argv)
{

  /* ca_context * hello; */
  /* ca_context_create (&hello); */
  /* ca_context_play (hello, 0, */
  /*     #<{(| CA_PROP_EVENT_ID, "phone-incoming-call", |)}># */
  /*     CA_PROP_EVENT_ID, argv[1], */
  /*     CA_PROP_EVENT_DESCRIPTION, "hello world", */
  /*     NULL); */
  /* g_usleep (2000000); */

  ca_context * ca_bell;
  ca_context_create(&ca_bell);

  /* int ca_r = ca_context_cache(ca_bell, */
  /*     CA_PROP_EVENT_ID, argv[1], */
  /*     CA_PROP_EVENT_DESCRIPTION, argv[1], */
  /*     NULL); */
  /*  */
  /*   if (ca_r < 0) { */
  /*     perror("ca_context_cache() failed\n"); */
  /*   } */

  /* ca_context_play (ca_bell, 0, */
  /*     CA_PROP_EVENT_ID, argv[1], */
  /*     NULL); */

  ca_proplist * p;
  ca_proplist_create(&p);

  ca_proplist_sets(p, CA_PROP_EVENT_ID, argv[1]);

  int playing = 1;

  int r = ca_context_play_full(ca_bell, 1, p, ca_callback, (void *)&playing);

  if (r < 0) {
    perror("ca_context_play_full() failed\n");
  }

  while (playing);

  ca_proplist_destroy(p);

  /* int playing = 1; */
  /* while (playing) { */
  /*   ca_context_playing(ca_bell, 1, &playing); */
  /* } */

  /* g_usleep (100000); */


  return 0;
}
