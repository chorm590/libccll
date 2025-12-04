#ifndef __CL_LIST_H__
#define __CL_LIST_H__

typedef struct cl_list CLIST;
struct cl_list {
	CLIST *next, *prev;
};

#define CRE_LIST_HEAD(name) \
	CLIST name = { &(name), &(name) }

static inline void init_list_node(CLIST *list)
{
	if(list == NULL) return;
	list->prev = list;
	list->next = list;
}

static inline void __list_add(CLIST *prev, CLIST *new, CLIST *next)
{
	if(prev == NULL || new == NULL || next == NULL) return;
	prev->next = new;
	new->prev = prev;
	new->next = next;
	next->prev = new;
}

static inline void list_add_head(CLIST *new, CLIST *head)
{
	__list_add(head, new, head->next);
}

static inline void list_add(CLIST *new, CLIST *head)
{
	__list_add(head->prev, new, head);
}

static inline void list_del(CLIST *node)
{
	if(node == NULL) return;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = NULL;
	node->next = NULL;
}

static inline bool list_empty(CLIST *head)
{
	if(head == NULL) return true;
	return head->prev == head;
}

static inline size_t list_size(CLIST *head)
{
	if(head == NULL) return 0;
	size_t cnt = 0;
	CLIST *a = head;
	int i = 0;
	while(true)
	{
		i++;
		CLIST *b = a->next;
		if(b == NULL || head == b) break;
		cnt++;
		a = b;
	}

	return cnt;
}

#define list_for_each(ptr_pos, ptr_head) \
	for(ptr_pos = (ptr_head)->next; ptr_pos != (ptr_head); ptr_pos = ptr_pos->next)

#define list_for_each2(ptr_type_pos, ptr_head, member) \
	for(ptr_type_pos = container_of((ptr_head)->next, typeof(*ptr_type_pos), member); &ptr_type_pos->member != (ptr_head); ptr_type_pos = container_of(ptr_type_pos->member.next, typeof(*ptr_type_pos), member))

/*
 * Remove node from list while in interating. Must merge the 'list_for_each' or 'list_for_each2' to using.
 *
 * Example:
 * 	typedef struct {
 *		int a;
 *		CLIST list;
 * 	} Type;
 * 	CRE_LIST_HEAD(li_tp);
 *
 * 	// Usage 1:
 * 	CLIST *ls;
 *	list_for_each(ls, &li_tp)
 *	{
 *		CLIST *rmv;
 *		list_pop(ls, rmv);
 *		free(container_of(rmv, Type, list));
 *	}
 *
 *	// Usage 2:
 *	Type *tp;
 *	list_for_each2(tp, &li_tp, list)
 *	{
 *		Type *rmv;
 *		list_pop2(tp, rmv, Type, list);
 *		free(rmv);
 *	}
 * */
#define list_pop(ptr_node_for_rm, ptr_node_rm) \
	CLIST *_li4337_a = ptr_node_for_rm->prev; ptr_node_rm = ptr_node_for_rm; list_del(ptr_node_for_rm); ptr_node_for_rm = _li4337_a

#define list_pop2(ptr_node_for_rm, ptr_node_rm, type, member) \
	CLIST *_li4337_a = ptr_node_for_rm->list.prev; ptr_node_rm = ptr_node_for_rm; list_del(&ptr_node_for_rm->list); ptr_node_for_rm = container_of(_li4337_a, type, member)

#endif
