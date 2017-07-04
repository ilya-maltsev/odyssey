#ifndef SO_FEWRITE_H_
#define SO_FEWRITE_H_

/*
 * SHAPITO.
 *
 * Protocol-level PostgreSQL client library.
*/

typedef struct so_fearg so_fearg_t;

struct so_fearg
{
	char *name;
	int len;
};

static inline int
so_fewrite_startup_message(so_stream_t *stream, int argc, so_fearg_t *argv)
{
	int size = sizeof(uint32_t) + /* len */
	           sizeof(uint32_t) + /* version */
	           sizeof(uint8_t);   /* last '\0' */
	int i = 0;
	for (; i < argc; i++)
		size += argv[i].len;
	int rc = so_stream_ensure(stream, size);
	if (so_unlikely(rc == -1))
		return -1;
	/* len */
	so_stream_write32(stream, size);
	/* version */
	so_stream_write32(stream, 196608);
	/* arguments */
	for (i = 0; i < argc; i++)
		so_stream_write(stream, argv[i].name, argv[i].len);
	/* eof */
	so_stream_write8(stream, 0);
	return 0;
}

static inline int
so_fewrite_cancel(so_stream_t *stream, uint32_t pid, uint32_t key)
{
	int size = sizeof(uint32_t) + /* len */
	           sizeof(uint32_t) + /* special */
	           sizeof(uint32_t) + /* pid */
	           sizeof(uint32_t);  /* key */
	int rc = so_stream_ensure(stream, size);
	if (so_unlikely(rc == -1))
		return -1;
	/* len */
	so_stream_write32(stream, size);
	/* special */
	so_stream_write32(stream, 80877102);
	/* pid */
	so_stream_write32(stream, pid);
	/* key */
	so_stream_write32(stream, key);
	return 0;
}

static inline int
so_fewrite_ssl_request(so_stream_t *stream)
{
	int size = sizeof(uint32_t) + /* len */
	           sizeof(uint32_t);  /* special */
	int rc = so_stream_ensure(stream, size);
	if (so_unlikely(rc == -1))
		return -1;
	/* len */
	so_stream_write32(stream, size);
	/* special */
	so_stream_write32(stream, 80877103);
	return 0;
}

static inline int
so_fewrite_terminate(so_stream_t *stream)
{
	int rc = so_stream_ensure(stream, sizeof(so_header_t));
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'X');
	so_stream_write32(stream, sizeof(uint32_t));
	return 0;
}

static inline int
so_fewrite_password(so_stream_t *stream, char *password, int len)
{
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + len);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'p');
	so_stream_write32(stream, sizeof(uint32_t) + len);
	so_stream_write(stream, password, len);
	return 0;
}

static inline int
so_fewrite_query(so_stream_t *stream, char *query, int len)
{
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + len);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'Q');
	so_stream_write32(stream, sizeof(uint32_t) + len);
	so_stream_write(stream, query, len);
	return 0;
}

static inline int
so_fewrite_parse(so_stream_t *stream,
                 char *operator_name, int operator_len,
                 char *query, int query_len,
                 uint16_t typec, int *typev)
{
	uint32_t size = operator_len + query_len + sizeof(uint16_t) +
	                typec * sizeof(uint32_t);
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + size);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'P');
	so_stream_write32(stream, sizeof(uint32_t) + size);
	so_stream_write(stream, operator_name, operator_len);
	so_stream_write(stream, query, query_len);
	so_stream_write16(stream, typec);
	int i = 0;
	for (; i < typec; i++)
		so_stream_write32(stream, typev[i]);
	return 0;
}

static inline int
so_fewrite_bind(so_stream_t *stream,
                char *portal_name, int portal_len,
                char *operator_name, int operator_len,
                int argc_call_types, int call_types[],
                int argc_result_types, int result_types[],
                int argc, int *argv_len, char **argv)
{
	int size = portal_len + operator_len +
	           sizeof(uint16_t) +                     /* argc_call_types */
	           sizeof(uint16_t) * argc_call_types +   /* call_types */
	           sizeof(uint16_t) +                     /* argc_result_types */
	           sizeof(uint16_t) * argc_result_types + /* result_types */
	           sizeof(uint16_t);                      /* argc */
	int i = 0;
	for (; i < argc; i++) {
		size += sizeof(uint32_t);
		if (argv_len[i] == -1)
			continue;
		size += argv_len[i];
	}
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + size);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'B');
	so_stream_write32(stream, sizeof(uint32_t) + size);
	so_stream_write(stream, portal_name, portal_len);
	so_stream_write(stream, operator_name, operator_len);

	so_stream_write16(stream, argc_call_types);
	for (i = 0; i < argc_call_types; i++)
		so_stream_write16(stream, call_types[i]);

	so_stream_write16(stream, argc);
	for (i = 0; i < argc; i++) {
		so_stream_write32(stream, argv_len[i]);
		if (argv_len[i] == -1)
			continue;
		so_stream_write(stream, argv[i], argv_len[i]);
	}

	so_stream_write16(stream, argc_result_types);
	for (i = 0; i < argc_result_types; i++)
		so_stream_write16(stream, result_types[i]);
	return 0;
}

static inline int
so_fewrite_describe(so_stream_t *stream, uint8_t type, char *name, int name_len)
{
	int size = sizeof(type) + name_len;
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + size);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'D');
	so_stream_write32(stream, sizeof(uint32_t) + size);
	so_stream_write8(stream, type);
	so_stream_write(stream, name, name_len);
	return 0;
}

static inline int
so_fewrite_execute(so_stream_t *stream, char *portal, int portal_len, uint32_t limit)
{
	int size = portal_len + sizeof(limit);
	int rc = so_stream_ensure(stream, sizeof(so_header_t) + size);
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'E');
	so_stream_write32(stream, sizeof(uint32_t) + size);
	so_stream_write(stream, portal, portal_len);
	so_stream_write32(stream, limit);
	return 0;
}

static inline int
so_fewrite_sync(so_stream_t *stream)
{
	int rc = so_stream_ensure(stream, sizeof(so_header_t));
	if (so_unlikely(rc == -1))
		return -1;
	so_stream_write8(stream, 'S');
	so_stream_write32(stream, sizeof(uint32_t));
	return 0;
}

#endif
