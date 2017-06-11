#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "object.h"

#define OBJ_HASHTABLE_SIZE   (339)

#define OBJSTAT_NORMAL    (0)
#define OBJSTAT_CLOSEWAIT   (1)

typedef struct _baseobject {
    objhld_t hld_;
    int stat_;
    int refcnt_;
    int objsizecb_;
    int user_size_;
    struct _baseobject *next_hash_clash_;
    objinitfn_t initializer_;
    objuninitfn_t unloader_;
    char user_data_[0];
} object_t;

#if _WIN32

static
struct {
    object_t *object_hash_table_[OBJ_HASHTABLE_SIZE];
    objhld_t automatic_id_;
    CRITICAL_SECTION locker_;
} g_objmgr;

#define LOCK    EnterCriticalSection
#define UNLOCK  LeaveCriticalSection

#else

static
struct {
    object_t *object_hash_table_[OBJ_HASHTABLE_SIZE];
    objhld_t automatic_id_;
    pthread_mutex_t locker_;
} g_objmgr = {
    { NULL}, 0, PTHREAD_MUTEX_INITIALIZER
};

#define LOCK    pthread_mutex_lock
#define UNLOCK  pthread_mutex_unlock

#endif

#define HLD_ROOT(hld)    ((object_t *)(g_objmgr.object_hash_table_[hld % OBJ_HASHTABLE_SIZE]))

static
int objtabinst(object_t *obj) {
    object_t * target;

    if (!obj || obj->next_hash_clash_) {
        return -1;
    }

    LOCK(&g_objmgr.locker_);
    obj->hld_ = ++g_objmgr.automatic_id_;
    target = HLD_ROOT(obj->hld_);
    if (!target) {
        g_objmgr.object_hash_table_[obj->hld_ % OBJ_HASHTABLE_SIZE] = obj;
    } else {
        while (target->next_hash_clash_) {
            target = target->next_hash_clash_;
        }
        target->next_hash_clash_ = obj;
    }
    UNLOCK(&g_objmgr.locker_);
    return obj->hld_;
}

static
void objtabrmve(object_t *obj) {
    object_t *found;

    if (!obj) {
        return;
    }

    LOCK(&g_objmgr.locker_);
    found = HLD_ROOT(obj->hld_);
    if (found) {
        if (found == obj) {
            g_objmgr.object_hash_table_[obj->hld_ % OBJ_HASHTABLE_SIZE] = obj->next_hash_clash_;
        } else {
            while (found->next_hash_clash_) {
                if (found->next_hash_clash_ == obj) {
                    found->next_hash_clash_ = obj->next_hash_clash_;
                    break;
                }
                found = found->next_hash_clash_;
            }
        }
    }
    UNLOCK(&g_objmgr.locker_);

    if (obj->unloader_) {
        obj->unloader_(obj->hld_, obj->user_data_);
    }

    free(obj);
}

static
object_t *objtabsrch(const objhld_t hld) {
    object_t *loop, *found;

    loop = found = NULL;

    loop = HLD_ROOT(hld);
    while (loop) {
        if (loop->hld_ == hld) {
            found = loop;
            break;
        }
        loop = loop->next_hash_clash_;
    }

    return found;
}

void objinit() {
    static long inited = 0;
#if _WIN32
    if (1 == InterlockedIncrement(&inited)) {
        memset(g_objmgr.object_hash_table_, 0, sizeof ( g_objmgr.object_hash_table_));
        g_objmgr.automatic_id_ = 0;
        InitializeCriticalSection(&g_objmgr.locker_);
    }
#else
    pthread_mutexattr_t attr;
    if (1 == __sync_add_and_fetch(&inited, 1)) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&g_objmgr.locker_, &attr);
    }
#endif
}

void objuninit() {
#if _WIN32
    DeleteCriticalSection(&g_objmgr.locker_);
#else
    pthread_mutex_destroy(&g_objmgr.locker_);
#endif
}

objhld_t objallo(int user_size, objinitfn_t initializer, objuninitfn_t unloader, void *initctx, unsigned int cbctx) {
    object_t *obj;

    if (0 == user_size || !initializer || !unloader) {
        return -1;
    }

    obj = (object_t *) malloc(user_size + sizeof ( object_t));
    if (!obj) {
        return -1;
    }

    obj->stat_ = OBJSTAT_NORMAL;
    obj->refcnt_ = 0;
    obj->objsizecb_ = user_size + sizeof ( object_t);
    obj->user_size_ = user_size;
    obj->next_hash_clash_ = NULL;
    obj->initializer_ = initializer;
    obj->unloader_ = unloader;
    memset(obj->user_data_, 0, obj->user_size_);

    if (obj->initializer_(obj->user_data_, initctx, cbctx) < 0) {
        obj->unloader_(-1, obj->user_data_);
        free(obj);
        return -1;
    }

    return objtabinst(obj);
}

void *objrefr(objhld_t hld) {
    object_t *obj = NULL;
    char *user_data = NULL;

    LOCK(&g_objmgr.locker_);
    obj = objtabsrch(hld);
    if (obj) {
        if (OBJSTAT_NORMAL == obj->stat_) {
            obj->refcnt_++;
            user_data = (char *) obj->user_data_;
        }
    }
    UNLOCK(&g_objmgr.locker_);
    return user_data;
}

void objdefr(objhld_t hld) {
    object_t *obj = NULL;
    int rmv = 0;

    LOCK(&g_objmgr.locker_);
    obj = objtabsrch(hld);
    if (obj) {
        if (0 == obj->refcnt_) {
            abort();
        } else {
            obj->refcnt_--;
        }
        rmv = ((OBJSTAT_CLOSEWAIT == obj->stat_ && 0 == obj->refcnt_) ? 1 : 0);
    }
    UNLOCK(&g_objmgr.locker_);

    if (rmv && obj) {
        objtabrmve(obj);
    }
}

void objclos(objhld_t hld) {
    object_t *obj = NULL;
    int rmv = 0;

    LOCK(&g_objmgr.locker_);
    obj = objtabsrch(hld);
    if (obj) {
        if (obj->refcnt_ > 0) {
            obj->stat_ = OBJSTAT_CLOSEWAIT;
        } else {
            rmv = 1;
        }
    }
    UNLOCK(&g_objmgr.locker_);

    if (rmv && obj) {
        objtabrmve(obj);
    }
}

int objentry(void *udata, void *ctx, int ctxcb) {
    return 0;
}

void objunload(int hld, void *udata) {

}