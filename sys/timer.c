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

TAG = "timer";

#define SRV_SKT_PATH "/tmp/l89ixdrb7cyu7c99l00l1a"

typedef struct {
	Bool srv;
	int fd;
	cl_timer_cb cb;
	CLIST list;
} Timer;

static CRE_LIST_HEAD(g_li_sock);
static int epofd = -1;
static pthread_mutex_t g_mtx_sock; // To ensure the safety of 'g_li_sock'
static pthread_t g_thr_socks;

static void * _timer_thread(void *data);
static Ret _epoll_add(int fd);

static int _cre_srv_sock()
{
	struct sockaddr_un server_addr;

    int srvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srvfd == -1) {
		CLOGE("create socket failed, err: %d", errno);
		return -1;
    }

	int flgs = fcntl(srvfd, F_GETFL, 0);
	fcntl(srvfd, F_SETFL, flgs | O_NONBLOCK);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SRV_SKT_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SRV_SKT_PATH);

    if (bind(srvfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		CLOGE("bind socket failed, err: %d", errno);
        close(srvfd);
		return -1;
    }

    if (listen(srvfd, 5) == -1) {
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
	node->cb = cb;
	init_list_node(&node->list);

	return node;
}

static Ret _cre_srv_node(int srvfd)
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

	Timer *srv_node = _cre_node(srvfd, NULL);
	srv_node->srv = true;
	list_add(&srv_node->list, &g_li_sock);

	return SUCC;
}

static Ret _cre_cli_node(const int sec, const int ms, cl_timer_cb cb)
{
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if(fd == -1)
	{
		CLOGE("create timerfd failed, err: %d", errno);
		return FAIL;
	}

	// Enqueue to list
	Timer *new_cli = _cre_node(fd, cb);
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

Ret cl_timer_init()
{
	TRACE();

	if(pthread_mutex_init(&g_mtx_sock, NULL))
	{
		LOG_INIT_FAIL(mutex);
		return FAIL;
	}

	int sock = _cre_srv_sock();
	if(sock == -1)
	{
		CLOGE("create srv sock failed");
		return FAIL;
	}

	int epoll = _cre_epoll();
	if(epoll == -1)
	{
		close(sock);
		CLOGE("create epoll failed");
		return FAIL;
	}

	if(pthread_mutex_lock(&g_mtx_sock))
	{
		close(sock);
		close(epoll);
		LOG_LOCK_FAIL(sock);
		return FAIL;
	}

	if(_cre_srv_node(sock) != SUCC)
	{
		close(sock);
		close(epoll);
		CLOGE("create srv node failed");
		return FAIL;
	}

	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		close(sock);
		close(epoll);
		Timer *srv_node = container_of(g_li_sock.next, Timer, list);
		FREE(srv_node);
		LOG_UNLOCK_FAIL(sock);

		return FAIL;
	}

	epofd = epoll;

	if(_epoll_add(sock))
	{
		CLOGE("add srv-node to epoll failed");
		close(sock);
		close(epoll);
		epofd = -1;
		Timer *srv_node = container_of(g_li_sock.next, Timer, list);
		FREE(srv_node);
		return FAIL;
	}

	if(pthread_create(&g_thr_socks, NULL, _timer_thread, NULL))
	{
		CLOGE("create sub-thread failed, err: %d", errno);
		close(sock);
		close(epoll);
		epofd = -1;
		Timer *srv_node = container_of(g_li_sock.next, Timer, list);
		FREE(srv_node);
		return FAIL;
	}

	return SUCC;
}

void cl_timer_deinit()
{
	TRACE();
	pthread_mutex_destroy(&g_mtx_sock);
	// TODO write srv-node to request deinit
	pthread_join(g_thr_socks, NULL);
}

static void _new_timer(const int sec, const int ms, const cl_timer_cb cb)
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
	if(_cre_cli_node(sec, ms, cb) != SUCC)
	{
		CLOGE("cre timer node failed");
		goto UNLOCK5534;
	}
	CLOGI("new timer on, %dS %dMS", sec, ms);

UNLOCK5534:
	if(pthread_mutex_unlock(&g_mtx_sock))
	{
		LOG_UNLOCK_FAIL(new timer);
		exit(-1);
	}
}

static void _rm_timer(const cl_timer_cb cb)
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
			close(tmr->fd);
			// Dequeue from list
			list_del(&tmr->list);
			FREE(tmr);
			CLOGI("timer canceled");
			break;
		}
	}
}

static Bool _srv_timer_proc(int fd)
{
	/*
		receive(plaintext):
			+----------+-------+--------+-------+------+----------------+
			|  opcode  |  len  |  type  |  sec  |  ms  |  callback fun  |
			+----------+-------+--------+-------+------+----------------+
			|    1B    |   2B  |   1B   |   2B  |  2B  |       8B       |
			+----------+-------+--------+-------+------+----------------+
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

		reply:
			none
	 * */
#define SZ 16
#define ERR(msg) \
	CLOGE("msg from srv-node failed cause %s", msg)

	uint8_t buf[SZ] = {0};
	if(read(fd, buf, SZ) != SZ)
	{
		ERR("unexpected rlen");
		return false;
	}

	int idx = 0;
	if(buf[idx++] != 1)
	{
		ERR("mismatch opcode");
		return false;
	}

	const int len = (buf[idx] << 8) | buf[idx + 1];
	idx += 2;
	if(len != 13)
	{
		ERR("unexpected len");
		return false;
	}

	const int type = buf[idx++];
	switch(type)
	{
		case 1: {
			const uint16_t sec = (buf[idx] << 8) | buf[idx + 1];
			const uint16_t ms = (buf[idx + 2] << 8) | buf[idx + 3];
			idx += 4;
			void *cb_fun = (void *) (buf + idx);
			_new_timer(sec, ms, cb_fun);
		} break;
		case 2: {
			idx += 4;
			void *cb_fun = (void *) (buf + idx);
			_rm_timer(cb_fun);
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

}

static void * _timer_thread(void *data)
{
	TRACE();
	struct epoll_event *ev_buf = (struct epoll_event *) MALLOC(10 * sizeof(struct epoll_event));

	int i;
	while(true)
	{
		const int nrs = epoll_wait(epofd, ev_buf, 10, -1);
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
					else _cli_timer_proc(fd);
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
			CLOGD("destroying %d", tmr->fd);
			_epoll_del(tmr->fd);
			list_del(&tmr->list);
			FREE(tmr);
			break;
		}
	}
	FREE(ev_buf);

	return NULL;
}

