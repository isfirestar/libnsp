#ifndef POSIX_WAIT_H
#define POSIX_WAIT_H

#include "posix_thread.h"

typedef struct __waitable_handle {
    int sync_; /* as boolean check */
#if _WIN32
    HANDLE cond_;
#else
    pthread_cond_t cond_;
    pthread_condattr_t condattr_;
    int pass_;
    posix__pthread_mutex_t mutex_;
#endif
} posix__waitable_handle_t;

__extern__
int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter);

/*
 * ��һ���ȴ�����ִ�еȴ����� 
 * @waiter  �ȴ�����
 * @tsc     �ȴ���ʱ�ĺ��뼶����
 * 
 * ���ض���:
 * ��� tsc <= 0 ����:
 * 0: �¼�����
 * -1: ϵͳ����ʧ��
 * 
 * ��� tsc > 0 ���У�
 * 0: �¼�����
 * ETIMEOUT: �ȴ���ʱ
 * -1: ϵͳ����ʧ�� 
 */
__extern__
int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, uint32_t tsc/*ms*/);

/*
 * ���� @waiter ����
 * ��� @waiter ��ͬ������ ��ʹ�� @waiter �����ڵȴ��е��̼߳����ڣ� ������һ���̱߳�����
 * ��� @waiter ��ͨ����� ��ʹ�� @waiter �����ڵȴ��е��̼߳����ڣ� �����߳̾�������
 * ��� @waiter ��ͨ����� �������ʽ���� posix__block_waitable_handle �����ö�����������
 */
__extern__
int posix__sig_waitable_handle(posix__waitable_handle_t *waiter);

/*
 * (ͨ�����)��������ͨ�����
 * �����ڲ�û�жԶ������ͽ����жϣ� ͬ��������ô˹��̵���Ϊ����֤��ȷ��
 */
__extern__
void posix__block_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
void posix__reset_waitable_handle(posix__waitable_handle_t *waiter);

#define DECLARE_SYNC_WAITER(name)   \
    struct __waitable_handle name; \
    posix__init_synchronous_waitable_handle(&name)

#define DECLARE_NOTI_WATIER(name) \
    struct __waitable_handle name; \
    posix__init_notification_waitable_handle(&name)

/* ����ǰ�߳�(����״̬) */
__extern__
void posix__hang();

/* �߾��ȵĹ̶���ʱִ�� */
__extern__
int posix__delay_execution(uint64_t us);

#endif /* POSIX_WAIT_H */

