#ifndef __CL_LIST_H__
#define __CL_LIST_H__

struct list_head {
	struct list_head *next, *prev;
};
typedef struct list_head LIST_HEAD;

#define CRE_LIST_HEAD(name) \
	LIST_HEAD name = { &(name), &(name) }

static inline void init_list_head(LIST_HEAD *list)
{
	list->prev = list;
	list->next = list;
}

static inline void __list_add(LIST_HEAD *prev, LIST_HEAD *new, LIST_HEAD *next)
{
	prev->next = new;
	new->prev = prev;
	new->next = next;
	next->prev = new;
}

static inline void list_add_head(LIST_HEAD *new, LIST_HEAD *head)
{
	__list_add(head, new, head->next);
}

static inline void list_add(LIST_HEAD *new, LIST_HEAD *head)
{
	__list_add(head->prev, new, head);
}

static inline void list_del(LIST_HEAD *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = NULL;
	node->next = NULL;
}

static inline Bool list_empty(LIST_HEAD *head)
{
	return head->prev == head;
}

#define list_for_each(pos, head) \
	for(pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member) \
	for(pos = container_of((head)->next, typeof(*pos), member); &pos->member != (head); pos = container_of(pos->member.next, typeof(*pos), member))

#endif
