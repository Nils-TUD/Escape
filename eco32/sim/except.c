/*
 * except.c -- exception handling
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "error.h"
#include "except.h"


#define MAX_ENV_NEST_DEPTH	10


static jmp_buf *environments[MAX_ENV_NEST_DEPTH];
static int currentEnvironment = -1;


void throwException(int exception) {
  if (currentEnvironment < 0) {
    error("exception %d thrown while no environment active", exception);
  }
  longjmp(*environments[currentEnvironment], exception);
}


void pushEnvironment(jmp_buf *environment) {
  if (currentEnvironment == MAX_ENV_NEST_DEPTH - 1) {
    error("too many environments active");
  }
  currentEnvironment++;
  environments[currentEnvironment] = environment;
}


void popEnvironment(void) {
  if (currentEnvironment < 0) {
    error("cannot pop environment - none active");
  }
  currentEnvironment--;
}
