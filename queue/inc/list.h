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

static inline Bool list_empty(CLIST *head)
{
	if(head == NULL) return true;
	return head->prev == head;
}

static inline size_t list_size(CLIST *head)
{
	if(head == NULL) return -1;
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

#define list_for_each(pos, head) \
	for(pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member) \
	for(pos = container_of((head)->next, typeof(*pos), member); &pos->member != (head); pos = container_of(pos->member.next, typeof(*pos), member))

#endif
