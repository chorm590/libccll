#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "def.h"
#include "log.h"
#include "_timer.h"
#include "timer.h"
#include "list.h"
#include "alloc.h"
#include "wait.h"

TAG = "timer";

#define SRV_SKT_PATH "/tmp/l89ixdrb7cyu7c99l00l1a"

typedef struct {
	Bool srv;
	int fd;
	uint32_t repeats;
	cl_timer_cb cb;
	CLIST list;
} Timer;

static CRE_LIST_HEAD(g_li_sock);
static int srvfd = -1;
static int clifd = -1;
static int epofd = -1;
static pthread_mutex_t g_mtx_sock; // To ensure the safety of 'g_li_sock'
static pthread_t g_thr_socks;

static void _timer_destroy();

static int _cre_srv_sock()
{
	struct sockaddr_un server_addr;

    int srvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srvfd == -1) {
		CLOGE("create socket failed, err: %d", errno);
		return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SRV_SKT_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SRV_SKT_PATH);

    if (bind(srvfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		CLOGE("bind socket failed, err: %d", errno);
        close(srvfd);
		return -1;
    }

    if (listen(srvfd, 1) == -1) {
		CLOGE("listen socket failed, err: %d", errno);
        close(srvfd);
		return -1;
    }

	return srvfd;
}

static int _cre_epoll()
{
	int fd = epoll_create1(0);
	if(fd == -1)
	{
		CLOGE("create epoll failed, err: %d", errno);
		return -1;
	}

	return fd;
}

static Timer * _cre_node(int fd, cl_timer_cb cb)
{
	Timer *node = (Timer *) MALLOC(sizeof(Timer));
	node->srv = false;
	node->fd = fd;
	node->repeats = 0;
	node->cb = cb;
	init_list_node(&node->list);

	return node;
}

static int _conn_srv()
{
	struct sockaddr_un srv_addr;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd == -1)
	{
		CLOGE("create socket to conn failed, err: %d", errno);
		return -1;
	}

	memset(&srv_addr, 0, sizeof(struct sockaddr_un));
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, SRV_SKT_PATH);

	SLEEP_MS(100);
	if(connect(fd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)))
	{
		CLOGE("connect to timer-sys failed, err: %d", errno);
		close(fd);
		return -1;
	}

	return fd;
}

static Ret _epoll_add(int fd)
{
	struct epoll_event eev = {
		.events = EPOLLIN,
		.data.fd = fd,
	};
	if(epoll_ctl(epofd, EPOLL_CTL_ADD, fd, &eev))
	{
		CLOGE("add fd to epoll list failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}

static Ret _epoll_del(int fd)
{
	if(epoll_ctl(epofd, EPOLL_CTL_DEL, fd, NULL))
	{
		CLOGW("delete timerfd from epoll failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}

static Ret _cre_cli_node(const int sec, const int ms, const uint32_t repeats, cl_timer_cb cb)
{
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if(fd == -1)
	{
		CLOGE("create timerfd failed, err: %d", errno);
		return FAIL;
	}

	// Enqueue to list
	Timer *new_cli = _cre_node(fd, cb);
	new_cli->repeats = repeats;
	list_add(&new_cli->list, &g_li_sock);

	// Add to epoll
	if(_epoll_add(fd) != SUCC)
	{
		CLOGE("add new timer to epoll failed");
		list_del(&new_cli->list);
		FREE(new_cli);
		close(fd);
		return FAIL;
	}

	// Run the timer on
	const long ns = ms * 1000000;
	struct itimerspec its = {
		.it_value.tv_sec = sec,
		.it_value.tv_nsec = ns,
		.it_interval.tv_sec = sec,
		.it_interval.tv_nsec = ns
	};

	if(timerfd_settime(fd, 0, &its, NULL))
	{
		list_del(&new_cli->list);
		FREE(new_cli);
		close(fd);
		CLOGE("set timerfd failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}

static void _add_timer(const int sec, const int ms, const uint32_t repeats, const cl_timer_cb cb)
{
	TRACE();
	if(pthread_mutex_lock(&g_mtx_sock))
	{
		LOG_LOCK_FAIL(new timer);
		return;
	}

	// Check whether exist
	Timer *tmr;
	list_for_each_entry(tmr, &g_li_sock, list)
	{
		if(tmr->cb == cb)
		{
			CLOGW("timer-cb registered");
			goto UNLOCK5534;
		}
	}

	// Check amount of timer in queue
	if(list_size(&g_li_sock) >/*To exclude the first 'srv-node'*/ MAX_TIMER_CNT)
	{
		CLOGE("Too many timer in queue");
		goto UNLOCK5534;
	}

	// Create new timer node
	if(_cre_cli_node(sec, ms, repeats, cb) != SUCC)
	{
		CLOGE("cre timer node failed");
		goto UNLOCK5534;
	}
	CLOGI("new timer on, %ds %dms", sec, ms);

UNLOCK5534:
	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(new timer);
		exit(-1);
	}
}

static void _cancel_timerfd(int fd)
{
	struct itimerspec its = {0};
	if(timerfd_settime(fd, 0, &its, NULL))
	{
		CLOGE("cancel timerfd failed, err: %d", errno);
	}
	close(fd);
}

static void _rmv_timer(const cl_timer_cb cb)
{
	TRACE();

	if(pthread_mutex_lock(&g_mtx_sock))
	{
		LOG_LOCK_FAIL(remove timer);
		return;
	}

	Timer *tmr;
	list_for_each_entry(tmr, &g_li_sock, list)
	{
		if(tmr->srv == false && tmr->cb == cb)
		{
			// Delete from epoll
			_epoll_del(tmr->fd);
			// Cancel timerfd
			_cancel_timerfd(tmr->fd);
			// Dequeue from list
			list_del(&tmr->list);
			close(tmr->fd);
			FREE(tmr);
			CLOGI("timer canceled");
			break;
		}
	}

	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(remove timer);
		exit(-1);
	}
}

/*
 * @return true
 * 				Requesting destroy the timer-sys
 * 		   false
 * 		   		Continue epoll_wait
 * */
static Bool _srv_timer_proc(int fd)
{
	/*
		receive(plaintext):
			+----------+-------+--------+-------+------+-----------+----------------+
			|  opcode  |  len  |  type  |  sec  |  ms  |  repeats  |  callback fun  |
			+----------+-------+--------+-------+------+-----------+----------------+
			|    1B    |   2B  |   1B   |   2B  |  2B  |     4B    |       8B       |
			+----------+-------+--------+-------+------+-----------+----------------+
			opcode:
				   1
			type:
				 1 -- New timer request
				 	  will be fail if 'callback fun' existed
				 2 -- Cancel timer
				 	  judge by 'callback fun'
				 3 -- Destroy the timer-sys
				 	  before exit the app
			sec:
				max 28800, 8h
				min 0
			ms:
				max 1000, 1s
				min 10, 10ms
			repeats:
					the times need to repeats

		reply:
			none
	 * */
#define SZ 20
#define ERR(msg) \
	CLOGE("srv-node fail cause %s, err: %s", msg, strerror(errno))

	uint8_t buf[SZ] = {0};
	if(read(fd, buf, SZ) != SZ)
	{
		ERR("unexpected rlen");
		exit(1);
		return false;
	}

	int idx = 0;
	if(buf[idx++] != 1)
	{
		ERR("mismatch opcode");
		return false;
	}

	const int len = *((uint16_t *) (buf + idx));
	idx += 2;
	if(len != 17)
	{
		ERR("unexpected len");
		return false;
	}

	const int type = buf[idx++];
	switch(type)
	{
		case 1: {
			const uint16_t sec = *((uint16_t *) (buf + idx));
			idx += 2;
			const uint16_t ms = *((uint16_t *) (buf + idx));
			idx += 2;
			const uint32_t rpe = *((uint32_t *) (buf + idx));
			idx += 4;
			cl_timer_cb cb_fun;
			memcpy(&cb_fun, buf + idx, 8);
			_add_timer(sec, ms, rpe, (cl_timer_cb) cb_fun);
		} break;
		case 2: {
			idx += 8;
			cl_timer_cb cb_fun;
			memcpy(&cb_fun, buf + idx, 8);
			_rmv_timer(cb_fun);
		} break;
		case 3:
			CLOGW("Destroying the timer-sys");
			return true;
		default:
			ERR("unsupported type");
			return false;
	}

#undef ERR
#undef SZ
	return false;
}

static void _cli_timer_proc(int fd)
{
	CLOGD("timer out: %d", fd);
	uint64_t val;
	static size_t sz = sizeof(uint64_t);
	if(read(fd, &val, sz) != sz)
	{
		CLOGE("read for timerfd %d failed, err: %d", errno);
		return;
	}

	if(pthread_mutex_lock(&g_mtx_sock))
	{
		LOG_LOCK_FAIL(timeout proc);
		return;
	}

	Timer *tmr;
	list_for_each_entry(tmr, &g_li_sock, list)
	{
		if(tmr->srv || tmr->fd != fd) continue;
		if(tmr->cb) tmr->cb();
		if(tmr->repeats > 1) tmr->repeats--;
		else if(tmr->repeats == 1)
		{
			_epoll_del(tmr->fd);
			_cancel_timerfd(tmr->fd);
			list_del(&tmr->list);
			FREE(tmr);
			CLOGI("timer %d used up", fd);
		}
		break;
	}

	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(timeout proc);
		exit(-1);
	}
}

static Ret _cre_srv_node(int fd)
{
	Timer *tmr;
	list_for_each_entry(tmr, &g_li_sock, list)
	{
		if(tmr->srv)
		{
			CLOGW("Already have srv-node");
			return FAIL;
		}
	}

	Timer *srv_node = _cre_node(fd, NULL);
	srv_node->srv = true;
	list_add(&srv_node->list, &g_li_sock);

	return SUCC;
}

static void _wait_self_connect()
{
	int fd = accept(srvfd, NULL, NULL);

	if(pthread_mutex_lock(&g_mtx_sock))
	{
		LOG_LOCK_FAIL(sock);
		exit(-1);
	}

	if(_cre_srv_node(fd) != SUCC)
	{
		CLOGE("create srv node failed");
		exit(-1);
	}

	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(sock);
		exit(-1);
	}

	if(_epoll_add(fd))
	{
		CLOGE("add srv-node to epoll failed");
		exit(-1);
	}

	// Set to non-blocking
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
	{
		CLOGE("get flags of fd failed, err: %d", errno);
		exit(-1);
	}
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void * _timer_thread(void *data)
{
	TRACE();
	struct epoll_event *ev_buf = (struct epoll_event *) MALLOC(10 * sizeof(struct epoll_event));

	_wait_self_connect();
	CLOGD("timer-sys is ready!");

	int i;
	while(true)
	{
		const int nrs = epoll_wait(epofd, ev_buf, 10, -1);
		CLOGD("epoll wait, nrs: %d", nrs);
		if(nrs < 0)
		{
			CLOGE("wait the epoll failed, err: %d", errno);
			break;
		}

		for(i = 0; i < nrs; i++)
		{
			const int fd = (ev_buf + i)->data.fd;
			Timer *tmr;
			list_for_each_entry(tmr, &g_li_sock, list)
			{
				if(tmr->fd == fd)
				{
					if(tmr->srv)
					{
						if(_srv_timer_proc(fd)) goto TMROUT1722;
					}
					else
					{
						_cli_timer_proc(fd);
						break;
					}
				}
			}
		}
	}

TMROUT1722:
	CLOGW("out of timer thread");
	const int cnt = list_size(&g_li_sock);
	for(i = 0; i < cnt; i++)
	{
		Timer *tmr;
		list_for_each_entry(tmr, &g_li_sock, list)
		{
			CLOGD("destroying fd: %d", tmr->fd);
			_epoll_del(tmr->fd);
			list_del(&tmr->list);
			close(tmr->fd);
			FREE(tmr);
			break;
		}
	}
	FREE(ev_buf);

	return NULL;
}

Ret cl_timer_init()
{
	TRACE();

	if(pthread_mutex_init(&g_mtx_sock, NULL))
	{
		LOG_INIT_FAIL(mutex);
		return FAIL;
	}

	srvfd = _cre_srv_sock();
	if(srvfd == -1)
	{
		CLOGE("create srv sock failed");
		return FAIL;
	}

	epofd = _cre_epoll();
	if(epofd == -1)
	{
		close(srvfd);
		srvfd = -1;
		CLOGE("create epoll failed");
		return FAIL;
	}

	if(pthread_create(&g_thr_socks, NULL, _timer_thread, NULL))
	{
		CLOGE("create sub-thread failed, err: %d", errno);
		close(srvfd);
		close(epofd);
		srvfd = -1;
		epofd = -1;
		return FAIL;
	}

	clifd = _conn_srv();
	if(clifd == -1)
	{
		CLOGE("connect to timer-sys failed");
		close(srvfd);
		close(epofd);
		srvfd = -1;
		epofd = -1;
		return FAIL;
	}

	return SUCC;
}

void cl_timer_deinit()
{
	TRACE();
	_timer_destroy();
	pthread_mutex_destroy(&g_mtx_sock);
	// TODO write srv-node to request deinit
	pthread_join(g_thr_socks, NULL);
    unlink(SRV_SKT_PATH);
	CLOGD("timer deinitialized");
}

#define SZ 20
static void _load_srv_buf(const int type, const int sec, const int ms, const uint32_t rpe, const cl_timer_cb cb, uint8_t *buf)
{
	int idx = 0;
	buf[idx++] = 1;
	*((uint16_t *) (buf + idx)) = (1 + 2 + 2 + 4 + 8);
	idx += 2;
	buf[idx++] = type;
	*((uint16_t *) (buf + idx)) = (uint16_t) sec;
	idx += 2;
	*((uint16_t *) (buf + idx)) = (uint16_t) ms;
	idx += 2;
	*((uint32_t *) (buf + idx)) = rpe;
	idx += 4;
	*((uint64_t *) (buf + idx)) = (uint64_t) cb;
}

static void _timer_destroy()
{
	TRACE();
	uint8_t buf[SZ];
	_load_srv_buf(3, 0, 0, 0, NULL, buf);
	if(write(clifd, buf, SZ) != SZ)
	{
		CLOGE("destroy timer failed, err: %d", errno);
	}
}

Ret cl_timer_set(const uint16_t sec, const uint16_t ms, const int repeats, cl_timer_cb cb)
{
	TRACE();
	uint8_t buf[SZ];
	_load_srv_buf(1, sec, ms, repeats, cb, buf);
	if(write(clifd, buf, SZ) != SZ)
	{
		CLOGE("set timer failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}

Ret cl_timer_cancel(cl_timer_cb cb)
{
	TRACE();
	uint8_t buf[SZ];
	_load_srv_buf(2, 0, 0, 0, cb, buf);
	if(write(clifd, buf, SZ) != SZ)
	{
		CLOGE("cancel the timer failed, err: %d", errno);
		return FAIL;
	}

	return SUCC;
}
#undef SZ

int cl_timer_count()
{
	TRACE();

	if(pthread_mutex_lock(&g_mtx_sock))
	{
		LOG_LOCK_FAIL(timeout proc);
		return;
	}

	const int cnt = list_size(&g_li_sock);

	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(timeout proc);
		exit(-1);
	}

	return cnt - 1;
}

