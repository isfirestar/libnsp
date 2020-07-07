#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#include "compiler.h"

typedef int64_t objhld_t;

#define INVALID_OBJHLD		(~((objhld_t)(0)))

typedef int( *objinitfn_t)(void *udata, const void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(objhld_t hld, void *udata);

__interface__
void objinit(); /* not necessary for Linux/Unix */
__interface__
void objuninit();	/* object module life cycle tobe the same with process is recommend  */
__interface__
objhld_t objallo(int user_size, objinitfn_t initializer, objuninitfn_t unloader, const void *initctx, unsigned int cbctx);
__interface__
objhld_t objallo2(int user_size); /* simple way to allocate a object, calling thread can use @objreff to final reference and unloaded the object user data segment  */
__interface__
void *objrefr(objhld_t hld);	/* object reference */
__interface__
void objdefr(objhld_t hld);		/* object deference */
__interface__
void *objreff(objhld_t hld);	/* object reference final */
__interface__
void objclos(objhld_t hld);		/* object mark close */
__interface__
void objregs();	/* clean up the current object set and make object manager regress to initial status,
					all objects will be going to try to close,
					Be careful! this operation will cause object manager global locked,
					unload method for each object are all maybe running inside the global locker  */

#endif
