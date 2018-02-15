#ifndef _ASSERT_H_
#define _ASSERT_H_

void _assert(int assertion, char * assertionStr, int line, char * file);

#define assert(x) _assert(x, #x, __LINE__, __FILE__)

#endif
