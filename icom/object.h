#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#include "compiler.h"

typedef int32_t objhld_t;

#define INVALID_OBJHLD		((objhld_t)(-1))

typedef int( *objinitfn_t)(void *udata, void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(objhld_t hld, void *udata);

extern
void objinit();
extern
void objuninit();
extern
objhld_t objallo(int user_data_size, objinitfn_t initializer, objuninitfn_t unloader, void *initctx, unsigned int cbctx);
extern
void *objrefr(objhld_t hld);	/* object reference */
extern
void objdefr(objhld_t hld);		/* object deference */
extern
void *objreff(objhld_t hld);	/* object reference final */
extern
void objclos(objhld_t hld);		/* object mark close */
	
#endif