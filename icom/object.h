#if !defined BASEOBJECT_H
#define BASEOBJECT_H

#include "compiler.h"

typedef int objhld_t;

#define INVALID_OBJHLD		(~((objhld_t)(0)))

typedef int( *objinitfn_t)(void *udata, const void *ctx, int ctxcb);
typedef void( *objuninitfn_t)(objhld_t hld, void *udata);

extern
void objinit(); /* not necessary for Linux/Unix */
extern
void objuninit();	/* object module life cycle tobe the same with process is recommend  */
extern
objhld_t objallo(int user_data_size, objinitfn_t initializer, objuninitfn_t unloader, const void *initctx, unsigned int cbctx);
extern
objhld_t objallo2(int user_data_size); /* simple way to allocate a object, calling thread can use @objreff to final reference and unloaded the object user data segment  */
extern
void *objrefr(objhld_t hld);	/* object reference */
extern
void objdefr(objhld_t hld);		/* object deference */
extern
void *objreff(objhld_t hld);	/* object reference final */
extern
void objclos(objhld_t hld);		/* object mark close */
	
#endif