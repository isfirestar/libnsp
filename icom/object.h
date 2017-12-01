#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#include "compiler.h"

typedef int32_t objhld_t;

#define INVALID_OBJHLD		((objhld_t)(-1))

typedef int( *objinitfn_t)(void *udata, void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(objhld_t hld, void *udata);

__extern__
void objinit();
__extern__
void objuninit();
__extern__
objhld_t objallo(int user_data_size, objinitfn_t initializer, objuninitfn_t unloader, void *initctx, unsigned int cbctx);
__extern__
void *objrefr(objhld_t hld);
__extern__
void objdefr(objhld_t hld);
__extern__
void objclos(objhld_t hld);

#endif