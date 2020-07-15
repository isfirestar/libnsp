#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "object.h"
#include "clist.h"
#include "avltree.h"

#define OBJ_HASHTABLE_SIZE   (397)  /* use a prime number as the hash key */

#define OBJSTAT_NORMAL    (0)
#define OBJSTAT_CLOSEWAIT   (1)

#if _WIN32 /* WIN32 */

typedef CRITICAL_SECTION MUTEX_T;

#define LOCK    EnterCriticalSection
#define UNLOCK  LeaveCriticalSection

#define INCREASEMENT(n)    InterlockedIncrement(n)

static void mutex_init(MUTEX_T *mutex)
{
    if (mutex) {
        InitializeCriticalSection(mutex);
    }
}

static void mutex_uninit(MUTEX_T *mutex)
{
    if (mutex) {
        DeleteCriticalSection(mutex);
    }
}

#else /* POSIX */

typedef pthread_mutex_t MUTEX_T;

#define LOCK    pthread_mutex_lock
#define UNLOCK  pthread_mutex_unlock

#define INCREASEMENT(n)    __sync_add_and_fetch(n, 1)

static void mutex_init(MUTEX_T *mutex)
{
    pthread_mutexattr_t attr;
    if (mutex) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(mutex, &attr);
    }
}

static void mutex_uninit(MUTEX_T *mutex)
{
    if (mutex) {
        pthread_mutex_destroy(mutex);
    }
}

#endif

typedef struct _object_t {
    struct avltree_node_t hash_clash_;
    objhld_t hld_;
    int stat_;
    int refcnt_;
    int objsizecb_;
    int user_size_;
    objinitfn_t initializer_;
    objuninitfn_t unloader_;
    unsigned char user_data_[0];
} object_t;

struct _object_manager {
    struct avltree_node_t *object_table_[OBJ_HASHTABLE_SIZE];
    objhld_t automatic_id_;
    MUTEX_T object_locker_;
};

static int avl_compare_routine(const void *left, const void *right)
{
    const object_t *lobj, *robj;

    assert(left && right);

    lobj = containing_record(left, object_t, hash_clash_);
    robj = containing_record(right, object_t, hash_clash_);

    if (lobj->hld_ > robj->hld_) {
        return 1;
    }

    if (lobj->hld_ < robj->hld_) {
        return -1;
    }

    return 0;
}

#if _WIN32 /* WIN32 */
static struct _object_manager g_objmgr;
#else
static struct _object_manager g_objmgr = {
    .object_table_ = { NULL }, 0, PTHREAD_MUTEX_INITIALIZER,
};
#endif

static
struct avltree_node_t **__hld2root(objhld_t hld)
{
    struct avltree_node_t **root;
    objhld_t idx;

    if (hld <= 0) {
        return NULL;
    }

    /* Exclude illegal hld input parameter */
    idx = hld % OBJ_HASHTABLE_SIZE;
    if (idx < 0 || idx >= OBJ_HASHTABLE_SIZE) {
        return NULL;
    }

    root = &g_objmgr.object_table_[idx];
    return root;
}

static
objhld_t __objtabinst(object_t *obj)
{
    objhld_t hld;
    struct avltree_node_t **root;

    if (!obj) {
        return INVALID_OBJHLD;
    }

    /* initial/reinitial the object's handle to INVALID */
    obj->hld_ = INVALID_OBJHLD;

    LOCK(&g_objmgr.object_locker_);

    do {
        /* automatic increase handle number */
        hld = ++g_objmgr.automatic_id_;

        /* map root pointer from table */
        root = __hld2root(hld);
        if (!root) {
            --g_objmgr.automatic_id_;
            break;
        }

        /* insert into hash list and using avl-binary-tree to handle the clash */
        obj->hld_ = hld;
        *root = avlinsert(*root, &obj->hash_clash_, &avl_compare_routine);
    } while (0);

    UNLOCK(&g_objmgr.object_locker_);
    return obj->hld_;
}

static
int __objtabrmve(objhld_t hld, object_t **removed)
{
	object_t node;
    struct avltree_node_t **root, *rmnode;

    root = __hld2root(hld);
    if (!root) {
        return -1;
    }

    rmnode = NULL;

    node.hld_ = hld;
    *root = avlremove(*root, &node.hash_clash_, &rmnode, &avl_compare_routine);
    if (rmnode && removed) {
        *removed = containing_record(rmnode, object_t, hash_clash_);
    }

    return ((NULL == rmnode) ? (-1) : (0));
}

static
object_t *__objtabsrch(const objhld_t hld)
{
    object_t node;
    struct avltree_node_t **root, *target;

    root = __hld2root(hld);
    if (!root) {
        return NULL;
    }

    node.hld_ = hld;
    target = avlsearch(*root, &node.hash_clash_, &avl_compare_routine);
    if (!target) {
        return NULL;
    }
    return containing_record(target, object_t, hash_clash_);
}

static
void __objtagfree(object_t *target)
{
    /* release the object context and free target memory when object removed from table
        call the unload routine if not null */
    if ( target ) {
        if ( target->unloader_ ) {
            target->unloader_( target->hld_, (void *)target->user_data_ );
        }
        free( target );
    }
}

void objinit()
{
    static long inited = 0;
    if ( 1 == INCREASEMENT(&inited)) {
        memset(g_objmgr.object_table_, 0, sizeof ( g_objmgr.object_table_));
        g_objmgr.automatic_id_ = 0;
        mutex_init(&g_objmgr.object_locker_);
    }
}

void objuninit()
{
    mutex_uninit(&g_objmgr.object_locker_);
}

objhld_t objallo(int user_size, objinitfn_t initializer, objuninitfn_t unloader, const void *initctx, unsigned int cbctx)
{
    object_t *obj;

    if (user_size <= 0) {
        return INVALID_OBJHLD;
    }

#if _WIN32
	objinit();
#endif

    obj = (object_t *) malloc(user_size + sizeof(object_t));
    if (!obj) {
        return INVALID_OBJHLD;
    }

    obj->stat_ = OBJSTAT_NORMAL;
    obj->refcnt_ = 0;
    obj->objsizecb_ = user_size + sizeof ( object_t);
    obj->user_size_ = user_size;
    memset(&obj->hash_clash_, 0, sizeof(obj->hash_clash_));
    obj->initializer_ = initializer;
    obj->unloader_ = unloader;
    memset(obj->user_data_, 0, obj->user_size_);

    if (obj->initializer_) {
        if (obj->initializer_((void *)obj->user_data_, initctx, cbctx) < 0) {
            obj->unloader_(-1, (void *)obj->user_data_);
            free(obj);
            return -1;
        }
    }

    if (INVALID_OBJHLD == __objtabinst(obj)) {
        free(obj);
        return INVALID_OBJHLD;
    }

    return obj->hld_;
}

objhld_t objallo2(int user_size)
{
    return objallo(user_size, NULL, NULL, NULL, 0);
}

void *objrefr(objhld_t hld)
{
    object_t *obj;
    unsigned char *user_data;

    obj = NULL;
    user_data = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = __objtabsrch(hld);
    if (obj) {
		/* object status CLOSE_WAIT will be ignore for @objrefr operation */
        if (OBJSTAT_NORMAL == obj->stat_) {
            ++obj->refcnt_;
            user_data = obj->user_data_;
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    return (void *)user_data;
}

void *objreff(objhld_t hld)
{
    object_t *obj;
    unsigned char *user_data;

    obj = NULL;
    user_data = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = __objtabsrch(hld);
    if (obj) {
        /* object status CLOSE_WAIT will be ignore for @objrefr operation */
        if (OBJSTAT_NORMAL == obj->stat_) {
            ++obj->refcnt_;
            user_data = obj->user_data_;

            /* change the object states to CLOSEWAIT immediately,
                so, other reference request will fail, object will be close when ref-count decrease equal to zero. */
            obj->stat_ = OBJSTAT_CLOSEWAIT;
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    return (void *)user_data;
}

void objdefr(objhld_t hld)
{
    object_t *obj, *removed;

    obj = NULL;
    removed = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = __objtabsrch(hld);
    if (obj) {
		/* in normal, ref-count must be greater than zero. otherwise, we will throw a assert fail*/
		assert( obj->refcnt_ > 0 );
		if (obj->refcnt_ > 0 ) {

            /* decrease the ref-count */
            --obj->refcnt_;

           /* if this object is waitting for close and ref-count decrease equal to zero,
				close it */
			if ( ( 0 == obj->refcnt_ ) && ( OBJSTAT_CLOSEWAIT == obj->stat_ ) ) {
                __objtabrmve(obj->hld_, &removed);
			}
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    if (removed) {
        __objtagfree(removed);
    }
}

void objclos(objhld_t hld)
{
    object_t *removed, *obj;

	removed = NULL;

    LOCK(&g_objmgr.object_locker_);
	obj = __objtabsrch(hld);
    if (obj) {
        /* if this object is already in CLOSE_WAIT status, maybe trying an "double close" operation, do nothing.
           if ref-count large than zero, do nothing during this close operation, actual close will take place when the last count dereference.
           if ref-count equal to zero, close canbe finish immediately */
        if ((0 == obj->refcnt_) && (OBJSTAT_NORMAL == obj->stat_)){
            __objtabrmve(obj->hld_, &removed);
        } else {
            obj->stat_ = OBJSTAT_CLOSEWAIT;
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    if (removed) {
        __objtagfree(removed);
    }
}

void objregs()
{
    ;
}

