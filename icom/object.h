#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#include "compiler.h"

typedef int64_t objhld_t;

#define INVALID_OBJHLD		(~((objhld_t)(0)))

typedef int( *objinitfn_t)(void *udata, const void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(objhld_t hld, void *udata);

PORTABLEAPI(void) objinit(); /* not necessary for Linux/Unix */
PORTABLEAPI(void) objuninit();	/* object module life cycle tobe the same with process is recommend  */
PORTABLEAPI(objhld_t) objallo(int user_size, objinitfn_t initializer, objuninitfn_t unloader, const void *initctx, unsigned int cbctx);
/* simple way to allocate a object, calling thread can use @objreff to final reference and unloaded the object user data segment  */
PORTABLEAPI(objhld_t) objallo2(int user_size);
PORTABLEAPI(void *) objrefr(objhld_t hld);	/* object reference */
PORTABLEAPI(void) objdefr(objhld_t hld);		/* object deference */
PORTABLEAPI(void *) objreff(objhld_t hld);	/* object reference final */
PORTABLEAPI(void) objclos(objhld_t hld);		/* object mark close */

#endif
