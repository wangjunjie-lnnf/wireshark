/* syntax-tree.c
 *
 * $Id: syntax-tree.c,v 1.1 2001/02/01 20:21:18 gram Exp $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "syntax-tree.h"

/* Keep track of sttype_t's via their sttype_id_t number */
static sttype_t* type_list[STTYPE_NUM_TYPES];

/* These are the sttype_t registration function prototypes. */
void sttype_register_pointer(void);
void sttype_register_range(void);
void sttype_register_string(void);
void sttype_register_test(void);


#define STNODE_MAGIC	0xe9b00b9e


void
sttype_init(void)
{
	sttype_register_pointer();
	sttype_register_range();
	sttype_register_string();
	sttype_register_test();
}

void
sttype_cleanup(void)
{
	/* nothing to do */
}


void
sttype_register(sttype_t *type)
{
	sttype_id_t	type_id;

	type_id = type->id;

	/* Check input */
	g_assert(type_id >= 0);
	g_assert(type_id < STTYPE_NUM_TYPES);

        /* Don't re-register. */
        g_assert(type_list[type_id] == NULL);

        type_list[type_id] = type;
}

static sttype_t*
sttype_lookup(sttype_id_t type_id)
{
	sttype_t	*result;

	/* Check input */
	g_assert(type_id >= 0);
	g_assert(type_id < STTYPE_NUM_TYPES);

	result = type_list[type_id];

	/* Check output. */
        g_assert(result != NULL);

        return result;
}


stnode_t*
stnode_new(sttype_id_t type_id, gpointer data)
{
	sttype_t	*type;
	stnode_t	*node;

	node = g_new(stnode_t, 1);
	node->magic = STNODE_MAGIC;

	if (type_id == STTYPE_UNINITIALIZED) {
		node->type = NULL;
		node->data = NULL;
	}
	else {
		type = sttype_lookup(type_id);
		g_assert(type);
		node->type = type;
		if (type->func_new) {
			node->data = type->func_new(data);
		}
		else {
			node->data = data;
		}

	}

	return node;
}

void
stnode_init(stnode_t *node, sttype_id_t type_id, gpointer data)
{
	sttype_t	*type;

	assert_magic(node, STNODE_MAGIC);
	g_assert(!node->type);
	g_assert(!node->data);

	type = sttype_lookup(type_id);
	g_assert(type);
	node->type = type;
	if (type->func_new) {
		node->data = type->func_new(data);
	}
	else {
		node->data = data;
	}
}

void
stnode_free(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	if (node->type) {
		if (node->type->func_free) {
			node->type->func_free(node->data);
		}
	}
	else {
		g_assert(!node->data);
	}
	g_free(node);
}

const char*
stnode_type_name(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	if (node->type)
		return node->type->name;
	else
		return "UNINITIALIZED";
}

sttype_id_t
stnode_type_id(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	if (node->type)
		return node->type->id;
	else
		return STTYPE_UNINITIALIZED;
}

gpointer
stnode_data(stnode_t *node)
{
	assert_magic(node, STNODE_MAGIC);
	if (node)
		return node->data;
	else
		return NULL;
}
