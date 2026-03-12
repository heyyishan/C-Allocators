#include <stdbool.h>
#include <stddef.h>

typedef void (*event_cb)(int event, const void *payload, void *user);

struct listener {
  event_cb fn;
  void *user;
};

struct callback_list {
  struct listener *data;
  size_t len;
  size_t cap;
};

bool callback_list_init(struct callback_list *c);
void callback_list_deinit(struct callback_list *c);
bool callback_list_add(struct callback_list *c, event_cb fn, void *user);
void callback_list_emit(const struct callback_list *c, int event,
                        const void *payload);
