#ifndef OD_SERVER_H
#define OD_SERVER_H

/*
 * Odissey.
 *
 * Advanced PostgreSQL connection pooler.
*/

typedef struct od_serverstat od_serverstat_t;
typedef struct od_server     od_server_t;

typedef enum
{
	OD_SUNDEF,
	OD_SIDLE,
	OD_SACTIVE,
	OD_SEXPIRE
} od_serverstate_t;

struct od_serverstat
{
	od_atomic_u64_t count_request;
	od_atomic_u64_t recv_server;
	od_atomic_u64_t recv_client;
	od_atomic_u64_t query_time;
	uint64_t        query_time_start;
	uint64_t        count_error;
};

struct od_server
{
	od_serverstate_t      state;
	od_id_t               id;
	shapito_stream_t      stream;
	shapito_parameters_t  params;
	machine_io_t         *io;
	machine_tls_t        *tls;
	int                   is_allocated;
	int                   is_transaction;
	int                   is_copy;
	od_serverstat_t       stats;
	uint64_t              sync_request;
	uint64_t              sync_reply;
	int                   idle_time;
	shapito_key_t         key;
	shapito_key_t         key_client;
	od_id_t               last_client_id;
	void                 *client;
	void                 *route;
	od_system_t          *system;
	od_list_t             link;
};

static inline void
od_server_init(od_server_t *server)
{
	server->state          = OD_SUNDEF;
	server->route          = NULL;
	server->client         = NULL;
	server->system         = NULL;
	server->io             = NULL;
	server->tls            = NULL;
	server->idle_time      = 0;
	server->is_allocated   = 0;
	server->is_transaction = 0;
	server->is_copy        = 0;
	server->sync_request   = 0;
	server->sync_reply     = 0;
	memset(&server->stats, 0, sizeof(server->stats));
	shapito_key_init(&server->key);
	shapito_key_init(&server->key_client);
	shapito_stream_init(&server->stream);
	shapito_parameters_init(&server->params);
	od_list_init(&server->link);
	memset(&server->id, 0, sizeof(server->id));
	memset(&server->last_client_id, 0, sizeof(server->last_client_id));
}

static inline od_server_t*
od_server_allocate(void)
{
	od_server_t *server = malloc(sizeof(*server));
	if (server == NULL)
		return NULL;
	od_server_init(server);
	server->is_allocated = 1;
	return server;
}

static inline void
od_server_free(od_server_t *server)
{
	shapito_stream_free(&server->stream);
	shapito_parameters_free(&server->params);
	if (server->is_allocated)
		free(server);
}

static inline int
od_server_sync_is(od_server_t *server)
{
	return server->sync_request == server->sync_reply;
}

static inline void
od_server_sync_request(od_server_t *server)
{
	server->sync_request++;
}

static inline void
od_server_sync_reply(od_server_t *server)
{
	server->sync_reply++;
}

static inline void
od_server_stat_request(od_server_t *server)
{
	server->stats.query_time_start = machine_time();
	od_atomic_u64_inc(&server->stats.count_request);
}

static inline uint64_t
od_server_stat_reply(od_server_t *server)
{
	uint64_t diff = machine_time() - server->stats.query_time_start;
	od_atomic_u64_add(&server->stats.query_time, diff);
	return diff;
}

static inline void
od_server_stat_recv_server(od_server_t *server, uint64_t bytes)
{
	od_atomic_u64_add(&server->stats.recv_server, bytes);
}

static inline void
od_server_stat_recv_client(od_server_t *server, uint64_t bytes)
{
	od_atomic_u64_add(&server->stats.recv_client, bytes);
}

static inline void
od_server_stat_error(od_server_t *server)
{
	server->stats.count_error++;
}

#endif /* OD_SERVER_H */
