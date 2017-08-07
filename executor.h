#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "defs.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int execute(Command *);

#endif // EXECUTOR_H
