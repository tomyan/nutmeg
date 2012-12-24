
#define dlist_first_field(prefix) \
    prefix##_first

#define dlist_last_field(prefix) \
    prefix##_last

#define dlist_first(prefix) \
    self->dlist_first_field(prefix)

#define dlist_last(prefix) \
    self->dlist_last_field(prefix)

#define dlist_container_fields(prefix, type) \
    type* dlist_first_field(prefix); \
    type* dlist_last_field(prefix);

#define dlist_container_init(prefix) \
    dlist_first(prefix) = NULL; \
    dlist_last(prefix) = NULL;

#define dlist_unshift(prefix, value) \
    value->next = dlist_first(prefix); \
    if (value->next == NULL) { \
        dilist_last(prefix) = value; \
    } \
    else { \
        value->next->previous = value; \
    } \
    dlist_first(prefix) = value; \
    value->previous = NULL;

#define dlist_push(prefix, value) \
    value->previous = dlist_last(prefix); \
    if (value->previous == NULL) { \
        dlist_first(prefix) = value; \
    } \
    else { \
        value->previous->next = value; \
    } \
    dlist_last(prefix) = value; \
    value->next = NULL;

#define dlist_insert_before(prefix, task, after) \
    assert(after != NULL); \
    task->previous = after->previous; \
    task->next = after; \
    if (after->previous == NULL) { \
        dlist_first(prefix) = task; \
    } \
    else { \
        after->previous->next = task; \
    } \
    after->previous = task;

#define dlist_detach_first(prefix) \
    if (dlist_first(prefix)->next == NULL) { \
        dlist_last(prefix) = NULL; \
    } \
    else { \
        dlist_first(prefix)->next->previous = NULL; \
    } \
    dlist_first(prefix) = dlist_first(prefix)->next;


#define dlist_has_first(prefix) \
    (dlist_first(prefix) != NULL)

#define dlist_shift_existing(prefix, var) \
    var = dlist_first(prefix); \
    dlist_detach_first(prefix);

#define dlist_shift(prefix, var) \
    var = dlist_first(prefix); \
    if (var != NULL) { \
        dlist_detach_first(prefix); \
    }

#define dlist_fields(type) \
    type* previous; \
    type* next;

#define dlist_init() \
    self->next = NULL; \
    self->previous = NULL;


