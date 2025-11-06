#ifndef __CL_LIST_H__
#define __CL_LIST_H__

typedef struct list_head LIST_HEAD;
struct list_head {
	LIST_HEAD *next, *prev;
};

#define CRE_LIST_HEAD(name) \
	LIST_HEAD name = { &(name), &(name) }

static inline void init_list_head(LIST_HEAD *list)
{
	if(list == NULL) return;
	list->prev = list;
	list->next = list;
}

static inline void __list_add(LIST_HEAD *prev, LIST_HEAD *new, LIST_HEAD *next)
{
	if(prev == NULL || new == NULL || next == NULL) return;
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
	if(node == NULL) return;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = NULL;
	node->next = NULL;
}

static inline Bool list_empty(LIST_HEAD *head)
{
	if(head == NULL) return true;
	return head->prev == head;
}

static inline size_t list_size(LIST_HEAD *head)
{
	if(head == NULL) return -1;
	size_t cnt = 0;
	LIST_HEAD *a = head;
	int i = 0;
	while(true)
	{
		i++;
		LIST_HEAD *b = a->next;
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
