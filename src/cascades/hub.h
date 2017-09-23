#ifndef HUB_H_
#define HUB_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC bool notify_instantpreview(const char *message);

#endif
