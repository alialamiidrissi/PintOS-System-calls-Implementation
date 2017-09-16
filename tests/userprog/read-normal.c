/* Try reading a file in the most normal way. */

//#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
   char sample[] = {
  "\"Amazing Electronic Fact: If you scuffed your feet long enough without\n"
  " touching anything, you would build up so many electrons that your\n"
  " finger would explode!  But this is nothing to worry about unless you\n"
  " have carpeting.\" --Dave Barry\n" 
};
  check_file ("sample.txt", sample, sizeof sample - 1);
}
