#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#if !defined OBJHLD_TYPEDEF
#define OBJHLD_TYPEDEF
typedef long objhld_t;
#endif

#include "compiler.h"

typedef int( *objinitfn_t)(void *udata, void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(int hld, void *udata);

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
__extern__
int objentry(void *udata, void *ctx, int ctxcb);
__extern__
void objunload(int hld, void *udata);

#endif