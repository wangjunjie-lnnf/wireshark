
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ftypes-int.h>
#include <string.h>
#include <ctype.h>
#include "resolv.h"

#define ETHER_LEN	6
#define IPv6_LEN	16

static void
ftype_from_tvbuff(field_info *fi, tvbuff_t *tvb, int start, int length,
	gboolean little_endian)
{
	/* XXX */
	g_assert_not_reached();
}


static void
bytes_fvalue_new(fvalue_t *fv)
{
	fv->value.bytes = NULL;
}

void
bytes_fvalue_free(fvalue_t *fv)
{
	if (fv->value.bytes) {
		g_byte_array_free(fv->value.bytes, TRUE);
	}
}


static void
bytes_fvalue_set(fvalue_t *fv, gpointer value, gboolean already_copied)
{
	g_assert(already_copied);
	fv->value.bytes = value;
}

static void
common_fvalue_set(fvalue_t *fv, guint8* data, guint len)
{
	fv->value.bytes = g_byte_array_new();
	g_byte_array_append(fv->value.bytes, data, len);
}

static void
ether_fvalue_set(fvalue_t *fv, gpointer value, gboolean already_copied)
{
	g_assert(!already_copied);
	common_fvalue_set(fv, value, ETHER_LEN);
}

static void
ipv6_fvalue_set(fvalue_t *fv, gpointer value, gboolean already_copied)
{
	g_assert(!already_copied);
	common_fvalue_set(fv, value, IPv6_LEN);
}

static gpointer
value_get(fvalue_t *fv)
{
	return fv->value.bytes->data;
}

static gboolean
is_byte_sep(guint8 c)
{
	return (c == '-' || c == ':' || c == '.');
}
	
static gboolean
val_from_string(fvalue_t *fv, char *s, LogFunc log)
{
	GByteArray	*bytes;
	guint8		val;
	char		*p, *q, *punct;
	char		two_digits[3];
	char		one_digit[2];
	gboolean	fail = FALSE;

	bytes = g_byte_array_new();

	p = s;
	while (*p) {
		q = p+1;
		if (*q && isxdigit(*p) && isxdigit(*q)) {
			two_digits[0] = *p;
			two_digits[1] = *q;
			two_digits[2] = '\0';

			val = (guint8) strtoul(two_digits, NULL, 16);
			g_byte_array_append(bytes, &val, 1);
			punct = q + 1;
			if (*punct) {
				if (is_byte_sep(*punct)) {
					p = punct + 1;
					continue;
				}
				else {
					fail = TRUE;
					break;
				}
			}
			else {
				p = punct;
				continue;
			}
		}
		else if (*q && isxdigit(*p) && is_byte_sep(*q)) {
			one_digit[0] = *p;
			one_digit[1] = '\0';

			val = (guint8) strtoul(one_digit, NULL, 16);
			g_byte_array_append(bytes, &val, 1);
			p = q + 1;
			continue;
		}
		else if (!*q && isxdigit(*p)) {
			one_digit[0] = *p;
			one_digit[1] = '\0';

			val = (guint8) strtoul(one_digit, NULL, 16);
			g_byte_array_append(bytes, &val, 1);
			p = q;
			continue;
		}
		else {
			fail = TRUE;
			break;
		}
	}

	if (fail) {
		g_byte_array_free(bytes, TRUE);
		return FALSE;
	}

	fv->value.bytes = bytes;


	return TRUE;
}

static gboolean
ether_from_string(fvalue_t *fv, char *s, LogFunc log)
{
	guint8	*mac;

	if (val_from_string(fv, s, log)) {
		return TRUE;
	}

	mac = get_ether_addr(s);
	if (!mac) {
		return FALSE;
	}

	ether_fvalue_set(fv, mac, FALSE);
	return TRUE;
}

static gboolean
ipv6_from_string(fvalue_t *fv, char *s, LogFunc log)
{
	guint8	buffer[16];

	if (!get_host_ipaddr6(s, (struct e_in6_addr*)buffer)) {
		return FALSE;
	}

	ipv6_fvalue_set(fv, buffer, FALSE);
	return TRUE;
}

static guint
len(fvalue_t *fv)
{
	return fv->value.bytes->len;
}

static void
slice(fvalue_t *fv, GByteArray *bytes, guint offset, guint length)
{
	guint8* data;

	data = fv->value.bytes->data + offset;

	g_byte_array_append(bytes, data, length);
}


static gboolean
cmp_eq(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len != b->len) {
		return FALSE;
	}

	return (memcmp(a->data, b->data, a->len) == 0);
}


static gboolean
cmp_ne(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len != b->len) {
		return FALSE;
	}

	return (memcmp(a->data, b->data, a->len) != 0);
}


static gboolean
cmp_gt(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len > b->len) {
		return TRUE;
	}

	if (a->len < b->len) {
		return FALSE;
	}
	
	return (memcmp(a->data, b->data, a->len) > 0);
}

static gboolean
cmp_ge(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len > b->len) {
		return TRUE;
	}

	if (a->len < b->len) {
		return FALSE;
	}
	
	return (memcmp(a->data, b->data, a->len) >= 0);
}

static gboolean
cmp_lt(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len < b->len) {
		return TRUE;
	}

	if (a->len > b->len) {
		return FALSE;
	}
	
	return (memcmp(a->data, b->data, a->len) < 0);
}

static gboolean
cmp_le(fvalue_t *fv_a, fvalue_t *fv_b)
{
	GByteArray	*a = fv_a->value.bytes;
	GByteArray	*b = fv_b->value.bytes;

	if (a->len < b->len) {
		return TRUE;
	}

	if (a->len > b->len) {
		return FALSE;
	}
	
	return (memcmp(a->data, b->data, a->len) <= 0);
}

void
ftype_register_bytes(void)
{

	static ftype_t bytes_type = {
		"FT_BYTES",
		"sequence of bytes",
		0,
		bytes_fvalue_new,
		bytes_fvalue_free,
		ftype_from_tvbuff,
		val_from_string,

		bytes_fvalue_set,
		NULL,
		NULL,

		value_get,
		NULL,
		NULL,

		cmp_eq,
		cmp_ne,
		cmp_gt,
		cmp_ge,
		cmp_lt,
		cmp_le,

		len,
		slice,
	};

	static ftype_t ether_type = {
		"FT_ETHER",
		"Ethernet or other MAC address",
		ETHER_LEN,
		bytes_fvalue_new,
		bytes_fvalue_free,
		ftype_from_tvbuff,
		ether_from_string,

		ether_fvalue_set,
		NULL,
		NULL,

		value_get,
		NULL,
		NULL,

		cmp_eq,
		cmp_ne,
		cmp_gt,
		cmp_ge,
		cmp_lt,
		cmp_le,

		len,
		slice,
	};

	static ftype_t ipv6_type = {
		"FT_IPv6",
		"IPv6 address",
		IPv6_LEN,
		bytes_fvalue_new,
		bytes_fvalue_free,
		ftype_from_tvbuff,
		ipv6_from_string,

		ipv6_fvalue_set,
		NULL,
		NULL,

		value_get,
		NULL,
		NULL,

		cmp_eq,
		cmp_ne,
		cmp_gt,
		cmp_ge,
		cmp_lt,
		cmp_le,

		len,
		slice,
	};

	ftype_register(FT_BYTES, &bytes_type);
	ftype_register(FT_ETHER, &ether_type);
	ftype_register(FT_IPv6, &ipv6_type);
}
