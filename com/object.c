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

#define OBJ_HASHTABLE_SIZE   (599)

#define OBJSTAT_NORMAL    (0)
#define OBJSTAT_CLOSEWAIT   (1)

#if _WIN32 /* WIN32 */

typedef CRITICAL_SECTION MUTEX_T;

#define LOCK    EnterCriticalSection
#define UNLOCK  LeaveCriticalSection

#define INCREASEMENT(n)    InterlockedIncrement(n)

static void mutex_init(MUTEX_T *mutex) {
    if (mutex) {
        InitializeCriticalSection(mutex);
    }
}

static void mutex_uninit(MUTEX_T *mutex) {
    if (mutex) {
        DeleteCriticalSection(mutex);
    }
}

#else /* POSIX */

typedef pthread_mutex_t MUTEX_T;

#define LOCK    pthread_mutex_lock
#define UNLOCK  pthread_mutex_unlock

#define INCREASEMENT(n)    __sync_add_and_fetch(n, 1)

static void mutex_init(MUTEX_T *mutex) {
    pthread_mutexattr_t attr;
    if (mutex) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(mutex, &attr);
    }
}

static void mutex_uninit(MUTEX_T *mutex) {
    if (mutex) {
        pthread_mutex_destroy(mutex);
    }
}

#endif

typedef struct _object_t {
    objhld_t hld_;
    int stat_;
    int refcnt_;
    int objsizecb_;
    int user_size_;
    struct list_head hash_clash_;
    objinitfn_t initializer_;
    objuninitfn_t unloader_;
    unsigned char user_data_[0];
} object_t;

struct _object_manager {
    struct list_head object_table_[OBJ_HASHTABLE_SIZE];
    objhld_t automatic_id_;
    MUTEX_T object_locker_;
};

#if _WIN32 /* WIN32 */
static struct _object_manager g_objmgr;
#else
static struct _object_manager g_objmgr = {
    .object_table_ = { { NULL } }, 0, PTHREAD_MUTEX_INITIALIZER,
};
#endif

static struct list_head *hld2root(objhld_t hld) {
    struct list_head *root;
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
    if (!root->next || !root->prev) {
        INIT_LIST_HEAD(root);
    }

    return root;
}

static objhld_t objtabinst(object_t *obj) {
    objhld_t hld;
    struct list_head *root;

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
        root = hld2root(hld);
        if (!root) {
            break;
        }

        /* insert into hash list */
        list_add_tail(&obj->hash_clash_, root);
       
        /* give the handle to object ptr */
        obj->hld_ = hld;
    } while (0);
    
    UNLOCK(&g_objmgr.object_locker_);
    return obj->hld_;
}

static int objtabrmve(objhld_t hld, object_t **removed) {
	object_t *target, *cursor;
    struct list_head *root, *pos, *n;

    target = NULL;

    do {
        root = hld2root(hld);
        if (!root) {
            return -1;
        }

        list_for_each_safe(pos, n, root) {
            cursor = containing_record(pos, object_t, hash_clash_);
            assert(cursor);
            if (cursor->hld_ == hld) {
                target = cursor;
                list_del(pos);
                INIT_LIST_HEAD(pos);
                break;
            }
        }
    } while ( 0 );

    if (removed) {
        *removed = target;
    }

    return ((NULL == target) ? (-1) : (0));
}

static object_t *objtabsrch(const objhld_t hld) {
    object_t *target, *cursor;
    struct list_head *root, *pos, *n;

    target = NULL;

    do {
        root = hld2root(hld);
        if (!root) {
            return NULL;
        }

        list_for_each_safe(pos, n, root) {
            cursor = containing_record(pos, object_t, hash_clash_);
            assert(cursor);
            if (cursor->hld_ == hld) {
                target = cursor;
                break;
            }
        }
    }while(0);

    return target;
}

static void objtagfree(object_t *taget) {
    /* release the object context and free target memory when object removed from table
        call the unload routine if not null */
    if ( taget ) {
        if ( taget->unloader_ ) {
            taget->unloader_( taget->hld_, (void *)taget->user_data_ );
        }
        free( taget );
    }
}

void objinit() {
    static long inited = 0;
    if ( 1 == INCREASEMENT(&inited)) {
        memset(g_objmgr.object_table_, 0, sizeof ( g_objmgr.object_table_));
        g_objmgr.automatic_id_ = 0;
        mutex_init(&g_objmgr.object_locker_);
    }
}

void objuninit() {
    mutex_uninit(&g_objmgr.object_locker_);
}

objhld_t objallo(int user_size, objinitfn_t initializer, objuninitfn_t unloader, const void *initctx, unsigned int cbctx) {
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
    INIT_LIST_HEAD(&obj->hash_clash_);
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

    if (INVALID_OBJHLD == objtabinst(obj)) {
        free(obj);
        return INVALID_OBJHLD;
    }

    return obj->hld_;
}

objhld_t objallo2(int user_size) {
    return objallo(user_size, NULL, NULL, NULL, 0);
}

void *objrefr(objhld_t hld) {
    object_t *obj;
    unsigned char *user_data;

    obj = NULL;
    user_data = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = objtabsrch(hld);
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

void *objreff(objhld_t hld) {
    object_t *obj;
    unsigned char *user_data;

    obj = NULL;
    user_data = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = objtabsrch(hld);
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

void objdefr(objhld_t hld) {
    object_t *obj, *removed;

    obj = NULL;
    removed = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = objtabsrch(hld);
    if (obj) {
		/* in normal, ref-count must be greater than zero. otherwise, we will throw a assert fail*/
		assert( obj->refcnt_ > 0 );
		if (obj->refcnt_ > 0 ) {

            /* decrease the ref-count */
            --obj->refcnt_;

           /* if this object is waitting for close and ref-count decrease equal to zero, 
				close it */
			if ( ( 0 == obj->refcnt_ ) && ( OBJSTAT_CLOSEWAIT == obj->stat_ ) ) {
                objtabrmve(obj->hld_, &removed);
			}
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    if (removed) {
        objtagfree(removed);
    }
}

void objclos(objhld_t hld) {
    object_t *obj, *removed;

    obj = NULL;
    removed = NULL;

    LOCK(&g_objmgr.object_locker_);
    obj = objtabsrch(hld);
    if (obj) {
        /* if this object is already in CLOSE_WAIT status, maybe trying an "double close" operation, do nothing.
           if ref-count large than zero, do nothing during this close operation, actual close will take place when the last count dereference. 
           if ref-count equal to zero, close canbe finish immediately */
        if ((0 == obj->refcnt_) && (OBJSTAT_NORMAL == obj->stat_)){
            objtabrmve(obj->hld_, &removed);
        } else {
            obj->stat_ = OBJSTAT_CLOSEWAIT;
        }
    }
    UNLOCK(&g_objmgr.object_locker_);

    if (removed) {
        objtagfree(removed);
    }
}
