#include <glib.h>
#include <canberra.h>

int
 main(int argc, char ** argv)
{
  ca_context * hello;
  ca_context_create (&hello);
  ca_context_play (hello, 0,
      /* CA_PROP_EVENT_ID, "phone-incoming-call", */
      CA_PROP_EVENT_ID, argv[1],
      CA_PROP_EVENT_DESCRIPTION, "hello world",
      NULL);
  g_usleep (2000000);

  return 0;
}
