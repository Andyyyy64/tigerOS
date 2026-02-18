#include <stdint.h>

#include "keyboard.h"
#include "keyboard_dispatch.h"
#include "wm_focus.h"
#include "wm_layers.h"

typedef struct keyboard_endpoint {
  const wm_window_t *window;
  keyboard_text_handler_t text_handler;
  keyboard_control_handler_t control_handler;
  void *ctx;
} keyboard_endpoint_t;

static keyboard_endpoint_t g_endpoints[WM_MAX_WINDOWS];

static int keyboard_event_is_control(keyboard_event_type_t type) {
  return type == KEYBOARD_EVENT_BACKSPACE || type == KEYBOARD_EVENT_ENTER || type == KEYBOARD_EVENT_TAB ||
         type == KEYBOARD_EVENT_ESCAPE;
}

static int keyboard_endpoint_find_index(const wm_window_t *window) {
  uint32_t i;

  if (window == (const wm_window_t *)0) {
    return -1;
  }

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    if (g_endpoints[i].window == window) {
      return (int)i;
    }
  }

  return -1;
}

static int keyboard_endpoint_allocate_index(void) {
  uint32_t i;

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    if (g_endpoints[i].window == (const wm_window_t *)0) {
      return (int)i;
    }
  }

  return -1;
}

void keyboard_dispatch_reset(void) {
  uint32_t i;

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    g_endpoints[i].window = (const wm_window_t *)0;
    g_endpoints[i].text_handler = (keyboard_text_handler_t)0;
    g_endpoints[i].control_handler = (keyboard_control_handler_t)0;
    g_endpoints[i].ctx = (void *)0;
  }

  keyboard_input_reset();
  wm_focus_reset();
}

int keyboard_dispatch_register_endpoint(const wm_window_t *window, keyboard_text_handler_t text_handler,
                                        keyboard_control_handler_t control_handler, void *ctx) {
  int index;

  if (window == (const wm_window_t *)0) {
    return -1;
  }

  index = keyboard_endpoint_find_index(window);
  if (index < 0) {
    index = keyboard_endpoint_allocate_index();
  }

  if (index < 0) {
    return -1;
  }

  g_endpoints[(uint32_t)index].window = window;
  g_endpoints[(uint32_t)index].text_handler = text_handler;
  g_endpoints[(uint32_t)index].control_handler = control_handler;
  g_endpoints[(uint32_t)index].ctx = ctx;
  return 0;
}

int keyboard_dispatch_unregister_endpoint(const wm_window_t *window) {
  int index = keyboard_endpoint_find_index(window);

  if (index < 0) {
    return -1;
  }

  g_endpoints[(uint32_t)index].window = (const wm_window_t *)0;
  g_endpoints[(uint32_t)index].text_handler = (keyboard_text_handler_t)0;
  g_endpoints[(uint32_t)index].control_handler = (keyboard_control_handler_t)0;
  g_endpoints[(uint32_t)index].ctx = (void *)0;
  (void)wm_focus_clear_if_active(window);
  return 0;
}

int keyboard_dispatch_route_event(const keyboard_event_t *event) {
  const wm_window_t *focused_window;
  int index;
  keyboard_endpoint_t *endpoint;

  if (event == (const keyboard_event_t *)0) {
    return -1;
  }

  focused_window = wm_focus_active_window();
  if (focused_window == (const wm_window_t *)0) {
    return 0;
  }

  index = keyboard_endpoint_find_index(focused_window);
  if (index < 0) {
    return 0;
  }

  endpoint = &g_endpoints[(uint32_t)index];
  if (wm_focus_is_active_window(endpoint->window) == 0) {
    return 0;
  }

  if (event->type == KEYBOARD_EVENT_TEXT) {
    if (endpoint->text_handler == (keyboard_text_handler_t)0 || event->text == '\0') {
      return 0;
    }

    endpoint->text_handler(endpoint->window, event->text, endpoint->ctx);
    return 1;
  }

  if (keyboard_event_is_control(event->type) == 0) {
    return 0;
  }

  if (endpoint->control_handler == (keyboard_control_handler_t)0) {
    return 0;
  }

  endpoint->control_handler(endpoint->window, event->type, endpoint->ctx);
  return 1;
}

int keyboard_dispatch_route_input_byte(uint8_t byte) {
  keyboard_event_t event;

  if (keyboard_event_from_byte(byte, &event) == 0) {
    return 0;
  }

  return keyboard_dispatch_route_event(&event);
}

int keyboard_dispatch_poll_and_route(void) {
  keyboard_event_t event;

  if (keyboard_poll_event(&event) == 0) {
    return 0;
  }

  return keyboard_dispatch_route_event(&event);
}
