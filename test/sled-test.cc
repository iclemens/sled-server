
#include <sled.h>

int main(int argc, char *argv[])
{
  sled_t *sled = sled_create();
  sled_destroy(&sled);
  
  return 0;
}
