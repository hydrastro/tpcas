#ifndef CAS_H
#define CAS_H
#endif /* CAS_H */

#include <stdlib.h>
#include "repl.h"

/*
example of what the input should look like:
 integrate(
    e^(x/2 * cos(x^2 ) ) * cos(e^2x) / sin(x^2 -2x + sqrt( -x) ) * x^(1/3) + k + f(x) + g(x,y,z),
    x
    [, start, end ]
)
*/

int main(int argc, char *argv[]) {
    compile_command_regex_maps();
    shell_loop();
	return EXIT_SUCCESS;
}
