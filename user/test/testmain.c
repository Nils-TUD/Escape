/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <stdlib.h>
#include <bufio.h>
#include <string.h>

/*int main(void) {
	div_t res = div(123,12);
	printf("quot=%d, rem=%d\n",res.quot,res.rem);
	ldiv_t res2 = ldiv(1231273123,12454);
	printf("quot=%d, rem=%d\n",res2.quot,res2.rem);
}
*/
int main ()
{
  char str[] ="- This, a sample string.";
  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str," ,.-");
  while (pch != NULL)
  {
    printf ("'%s'\n",pch);
    pch = strtok (NULL, " ,.-");
  }
  return 0;
}
