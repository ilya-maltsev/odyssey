
/*
 * Shapito.
 *
 * Protocol-level PostgreSQL client library.
*/

#include "shapito.h"

SHAPITO_API int
shapito_fe_read_ready(int *status, char *data, uint32_t size)
{
	shapito_header_t *header = (shapito_header_t*)data;
	uint32_t len;
	int rc = shapito_read(&len, &data, &size);
	if (shapito_unlikely(rc != 0))
		return -1;
	if (shapito_unlikely(header->type != 'Z' || len != 1))
		return -1;
	*status = header->data[0];
	return 0;
}

SHAPITO_API int
shapito_fe_read_key(shapito_key_t *key, char *data, uint32_t size)
{
	shapito_header_t *header = (shapito_header_t*)data;
	uint32_t len;
	int rc = shapito_read(&len, &data, &size);
	if (shapito_unlikely(rc != 0))
		return -1;
	if (shapito_unlikely(header->type != 'K' || len != 8))
		return -1;
	uint32_t pos_size = len;
	char *pos = header->data;
	rc = shapito_stream_read32(&key->key_pid, &pos, &pos_size);
	if (shapito_unlikely(rc == -1))
		return -1;
	rc = shapito_stream_read32(&key->key, &pos, &pos_size);
	if (shapito_unlikely(rc == -1))
		return -1;
	return 0;
}

SHAPITO_API int
shapito_fe_read_auth(uint32_t *type, char salt[4], char *data, uint32_t size)
{
	shapito_header_t *header = (shapito_header_t*)data;
	uint32_t len;
	int rc = shapito_read(&len, &data, &size);
	if (shapito_unlikely(rc != 0))
		return -1;
	if (shapito_unlikely(header->type != 'R'))
		return -1;
	uint32_t pos_size = len;
	char *pos = header->data;
	rc = shapito_stream_read32(type, &pos, &pos_size);
	if (shapito_unlikely(rc == -1))
		return -1;
	switch (*type) {
	/* AuthenticationOk */
	case 0:
		return 0;
	/* AuthenticationCleartextPassword */
	case 3:
		return 0;
	/* AuthenticationMD5Password */
	case 5:
		if (pos_size != 4)
			return -1;
		memcpy(salt, pos, 4);
		return 0;
	}
	/* unsupported */
	return -1;
}

SHAPITO_API int
shapito_fe_read_parameter(shapito_parameters_t *params, char *data, uint32_t size)
{
	shapito_header_t *header = (shapito_header_t*)data;
	uint32_t len;
	int rc = shapito_read(&len, &data, &size);
	if (shapito_unlikely(rc != 0))
		return -1;
	if (shapito_unlikely(header->type != 'S'))
		return -1;
	/* name */
	uint32_t name_len = 0;
	char *name;
	name = data;
	rc = shapito_stream_readsz(&data, &name_len);
	if (shapito_unlikely(rc == -1))
		return -1;
	name_len = data - name;
	/* value */
	uint32_t value_len = 0;
	char *value;
	value = data;
	rc = shapito_stream_readsz(&data, &value_len);
	if (shapito_unlikely(rc == -1))
		return -1;
	value_len = data - value;
	rc = shapito_parameters_add(params, name, name_len, value, value_len);
	if (shapito_unlikely(rc == -1))
		return -1;
	return 0;
}

SHAPITO_API int
shapito_fe_read_error(shapito_fe_error_t *error, char *data, uint32_t size)
{
	shapito_header_t *header = (shapito_header_t*)data;
	uint32_t len;
	int rc = shapito_read(&len, &data, &size);
	if (shapito_unlikely(rc != 0))
		return -1;
	if (shapito_unlikely(header->type != 'E'))
		return -1;
	memset(error, 0, sizeof(*error));
	uint32_t pos_size = len;
	char *pos = header->data;
	for (;;)
	{
		char type;
		int rc;
		rc = shapito_stream_read8(&type, &pos, &pos_size);
		if (shapito_unlikely(rc == -1))
			return -1;
		switch (type) {
		/* severity */
		case 'S':
			error->severity = pos;
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		/* sqlstate */
		case 'C':
			error->code = pos;
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		/* message */
		case 'M':
			error->message = pos;
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		/* detail */
		case 'D':
			error->detail = pos;
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		/* hint */
		case 'H':
			error->hint = pos;
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		/* end */
		case 0:
			return 0;
		default:
			rc = shapito_stream_readsz(&pos, &pos_size);
			if (shapito_unlikely(rc == -1))
				return -1;
			break;
		}
	}
	return 0;
}
