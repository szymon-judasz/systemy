#ifndef _UTILS_H_
#define _UTILS_H_

#include "siparse.h"

void printcommand(command *, int);
void printpipeline(pipeline, int);
void printparsedline(line *);

command * pickfirstcommand(line *);

int cstringtoint(char* str, int* result);



#endif /* !_UTILS_H_ */
