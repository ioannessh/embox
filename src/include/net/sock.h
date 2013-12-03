/**
 * @file
 * @brief
 *
 * @date 17.03.09
 * @author Anton Bondarev
 * @author Ilia Vaprol
 */

#ifndef NET_SOCK_H_
#define NET_SOCK_H_

#include <assert.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <stddef.h>
#include <stdbool.h>

#include <util/list.h>

#include <fs/idesc.h>
//#include <kernel/task/io_sync.h>
#include <net/skbuff.h>


struct proto_sock; //TODO What does it mean
struct sock_family_ops;
struct sock_proto_ops;
struct net_pack_out_ops;
struct pool;
struct indexator;

enum sock_state {
	SS_UNKNOWN,
	SS_UNCONNECTED,
	SS_BOUND,
	SS_LISTENING,
	SS_CONNECTING,
	SS_CONNECTED,
	SS_ESTABLISHED,
	SS_DISCONNECTING,
	SS_CLOSED
};

struct sock_opt {
	int so_acceptconn;
	struct net_device *so_bindtodevice;
	int so_broadcast;
	int so_domain;
	int so_dontroute;
	int so_error;
	struct linger so_linger;
	int so_oobinline;
	int so_protocol;
	int so_rcvbuf;
#define SOCK_OPT_DEFAULT_RCVBUF   16384
	int so_rcvlowat;
#define SOCK_OPT_DEFAULT_RCVLOWAT 1
	struct timeval so_rcvtimeo;
#define SOCK_OPT_DEFAULT_RCVTIMEO { .tv_sec = 0, .tv_usec = 0 }
	int so_sndbuf;
#define SOCK_OPT_DEFAULT_SNDBUF   16384
	int so_sndlowat;
#define SOCK_OPT_DEFAULT_SNDLOWAT 1
	struct timeval so_sndtimeo;
#define SOCK_OPT_DEFAULT_SNDTIMEO { .tv_sec = 0, .tv_usec = 0 }
	int so_type;
};

/* Base class for family sockets */
struct sock {
	struct idesc idesc;
	struct list_link lnk;
	enum sock_state state;
	struct sock_opt opt;
	struct sk_buff_head rx_queue;
	struct sk_buff_head tx_queue;
	unsigned int rx_data_len;
	//unsigned int tx_data_len;
	unsigned char shutdown_flag; /* FIXME */
	struct proto_sock *p_sk;
	const struct sock_family_ops *f_ops;
	const struct sock_proto_ops *p_ops;
	const struct net_pack_out_ops *o_ops;
};

struct sock_family_ops {
	int (*init)(struct sock *sk);
	int (*close)(struct sock *sk);
	int (*bind)(struct sock *sk, const struct sockaddr *addr,
			socklen_t addrlen);
	int (*bind_local)(struct sock *sk);
	int (*connect)(struct sock *sk, const struct sockaddr *addr,
			socklen_t addrlen, int flags);
	int (*listen)(struct sock *sk, int len);
	int (*accept)(struct sock *sk, struct sockaddr *addr,
			socklen_t *addrlen, int flags, struct sock **out_sk);
	int (*sendmsg)(struct sock *sk, struct msghdr *msg,
			int flags);
	int (*recvmsg)(struct sock *sk, struct msghdr *msg,
			int flags);
	int (*getsockname)(struct sock *sk, struct sockaddr *addr,
			socklen_t *addrlen);
	int (*getpeername)(struct sock *sk, struct sockaddr *addr,
			socklen_t *addrlen);
	int (*getsockopt)(struct sock *sk, int level, int optname,
			void *optval, socklen_t *optlen);
	int (*setsockopt)(struct sock *sk, int level, int optname,
			const void *optval, socklen_t optlen);
	int (*shutdown)(struct sock *sk, int how);
	struct pool *sock_pool;
};

struct sock_proto_ops {
	int (*init)(struct sock *sk);
	int (*close)(struct sock *sk);
	int (*connect)(struct sock *sk, const struct sockaddr *addr,
			socklen_t addrlen, int flags);
	int (*listen)(struct sock *sk, int backlog);
	int (*accept)(struct sock *sk, struct sockaddr *addr,
			socklen_t *addrlen, int flags, struct sock **out_sk);
	int (*sendmsg)(struct sock *sk, struct msghdr *msg, int flags);
	int (*recvmsg)(struct sock *sk, struct msghdr *msg, int flags);
	int (*getsockopt)(struct sock *sk, int level, int optname,
			void *optval, socklen_t *optlen);
	int (*setsockopt)(struct sock *sk, int level, int optname,
			const void *optval, socklen_t optlen);
	int (*shutdown)(struct sock *sk, int how);
	struct pool *sock_pool;
	struct indexator *sock_port;
	struct list *sock_list;
};

/* Base class for protocol sockets */
struct proto_sock {
	struct sock *sk;
};

/* Conversion to base family socket.
 * @arg p_sk - derived of proto_sock
 *             (proto_sock MUST BE FIRST field in derived socket type)
 */
static inline struct sock * to_sock(const void *p_sk) {
	assert(p_sk != NULL);
	return ((const struct proto_sock *)p_sk)->sk;
}


extern struct sock * sock_create(int family, int type, int protocol);

extern void sock_release(struct sock *sk);

extern void sock_hash(struct sock *sk);
extern void sock_unhash(struct sock *sk);


extern void sock_rcv(struct sock *sk, struct sk_buff *skb,
		unsigned char *p_data, size_t size);

extern int sock_close(struct sock *sk);

extern int sock_common_recvmsg(struct sock *sk, struct msghdr *msg,
		int flags, int stream_mode);

static inline int sock_nonstream_recvmsg(struct sock *sk,
		struct msghdr *msg, int flags) {
	return sock_common_recvmsg(sk, msg, flags, 0);
}

static inline int sock_stream_recvmsg(struct sock *sk,
		struct msghdr *msg, int flags) {
	return sock_common_recvmsg(sk, msg, flags, 1);
}

#include <net/sock_state.h>

static inline void sock_set_so_error(struct sock *sk, int error) {
	assert(sk != NULL);
	sk->opt.so_error = error;
}



typedef int (*sock_lookup_tester_ft)(const struct sock *sk,
		const struct sk_buff *skb);

extern struct sock * sock_iter(const struct sock_proto_ops *p_ops);
extern struct sock * sock_next(const struct sock *sk);
extern struct sock * sock_lookup(const struct sock *sk,
		const struct sock_proto_ops *p_ops,
		sock_lookup_tester_ft tester,
		const struct sk_buff *skb);

#define sock_foreach(sk, p_ops) \
	list_foreach(sk, p_ops->sock_list, lnk)

/**
 * AF_INET/AF_INET6 socket functions
 */
#include <netinet/in.h>
extern in_port_t sock_inet_get_src_port(const struct sock *sk);
extern in_port_t sock_inet_get_dst_port(const struct sock *sk);

#endif /* NET_SOCK_H_ */
