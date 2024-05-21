#include "config.h"
#include "env.h"
#include "hotreload.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "xdg-shell-protocol.h"

struct wl_display *display;
struct wl_compositor *compositor;
struct wl_surface *surface;
struct wl_buffer *buffer;
struct wl_shm *shm;
struct wl_callback_listener cb_listener;

// XDG Shell stuff
struct xdg_wm_base *shell;
struct xdg_toplevel *window;

void *pixel_data;
int width = INIT_WIDTH;
int height = INIT_HEIGHT;
bool window_should_close = false;

AppModule module = {0};

// Fills the buffer with random file_name
void rand_filename(char *buf, size_t n) {
  buf[0] = '/';
  for (size_t i = 1; i < n - 1; i++) {
    buf[i] = (rand() & ('z' - 'a' + 1)) + 'a';
  }
  buf[n - 1] = 0;
}

static int create_shm_file(size_t size) {
  char name[20];
  int retries = 100;
  do {
    rand_filename(name, 20);
    --retries;

    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      shm_unlink(name);
      ftruncate(fd, size);
      return fd;
    }
  } while (retries > 0);
  return -1;
}

void window_resize(void) {
  size_t stride = width * 4;
  size_t buffer_size = height * stride;
  size_t pool_length = 1;
  size_t pool_size = buffer_size * pool_length;

  int fd = create_shm_file(pool_size);
  if (fd < 0) {
    fprintf(stderr, "ERROR: Failed to create shm file.\n");
    return;
  }

  // Storing the buffer as a global array so we can
  // reuse it later
  pixel_data = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (pixel_data == NULL) {
    fprintf(stderr, "ERROR: Failed to mmap file.\n");
    return;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, pool_size);
  buffer = wl_shm_pool_create_buffer(
      pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);

  close(fd);
}

void render_frame(void) {
  Env env = {
      .buffer = pixel_data,
      .window_width = width,
      .window_height = height,
  };
  module.app_update(&env);

  wl_surface_attach(surface, buffer, 0, 0);
  wl_surface_damage(surface, 0, 0, width, height);
  wl_surface_commit(surface);
}

void update_frame(void* data, struct wl_callback* cb, uint32_t cb_data) {
    (void)data;
    (void)cb_data;

    wl_callback_destroy(cb);
    cb = wl_surface_frame(surface);
    wl_callback_add_listener(cb, &cb_listener, NULL);

    render_frame();
}

struct wl_callback_listener cb_listener = {
    .done = update_frame,
};

static void toplevel_configure(void* data, struct xdg_toplevel* toplevel,
	int32_t w, int32_t h, struct wl_array* states) {
    	(void)data;
    	(void)toplevel;
    	(void)states;

    	if (w == 0 || h == 0) {
        	return;
    	}

    	if (width != w || height != h) {
        	size_t old_buffer_size = width*height*4;
        	munmap(pixel_data, old_buffer_size);

        	width = w;
		height = h;
            window_resize();
    	}
}

static void toplevel_close(void* data, struct xdg_toplevel* toplevel) {
    	(void)data;
    	(void)toplevel;

    	window_should_close = true;
    	fprintf (stderr, "INFO: Closing\n");
}

static struct xdg_toplevel_listener toplevel_listener = {
    .configure = toplevel_configure,
    .close = toplevel_close,
};

static void xrfc_configure(void* data, struct xdg_surface* xrfc, uint32_t serial) {
    (void)data;

    xdg_surface_ack_configure(xrfc, serial);
    if (!pixel_data) {
        window_resize();
    }

    render_frame();
}

static struct xdg_surface_listener xrfc_listener = {
    .configure = xrfc_configure,
};

static void shell_ping(void* data, struct xdg_wm_base* wm, uint32_t serial) {
    (void)data;
    (void)wm;
    (void)serial;
}

static struct xdg_wm_base_listener shell_listener = {
    .ping = shell_ping,
};


static void registry_handler(void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface,
                             uint32_t version) {
  (void)version;
  (void)data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    shell = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(shell, &shell_listener, NULL);
  }
}

static void registry_remover(void *data, struct wl_registry *registry,
                             uint32_t id) {
  (void)data;
  (void)registry;
  (void)id;
}

static const struct wl_registry_listener registry_listener = {registry_handler,
                                                              registry_remover};

int main(void) {
  // Loading the app module
  load_module(&module, "./everything.so");
    
  // Connect to the Wayland display server
  display = wl_display_connect(NULL);
  if (display == NULL) {
    fprintf(stderr, "ERROR: Failed to connect to the Wayland display.\n");
    exit(1);
  }

  // Get the registry and add the listener
  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, NULL);
  wl_display_roundtrip(display);

  if (compositor == NULL || shm == NULL) {
    fprintf(stderr, "ERROR: Failed to get the wayland interfaces.\n");
    exit(1);
  }

  // Create a surface
  surface = wl_compositor_create_surface(compositor);
  if (surface == NULL) {
    fprintf(stderr, "ERROR: Failed to create a Wayland surface.\n");
    exit(1);
  }
  struct wl_callback* cb = wl_surface_frame(surface);
  wl_callback_add_listener(cb, &cb_listener, NULL);

  struct xdg_surface* xrfc = xdg_wm_base_get_xdg_surface(shell, surface);
  xdg_surface_add_listener(xrfc, &xrfc_listener, NULL);
  
  window = xdg_surface_get_toplevel(xrfc); 
  xdg_toplevel_add_listener(window, &toplevel_listener, NULL); 
  xdg_toplevel_set_title(window, "Wayland Client");

  wl_surface_commit(surface);
  
  // Main event loop
  while (wl_display_dispatch(display) != -1) {
      if (window_should_close) {
          break;
      }
  }

  // Clean up
  if(buffer) wl_buffer_destroy(buffer);
  xdg_toplevel_destroy(window);
  xdg_surface_destroy(xrfc);
  wl_surface_destroy(surface);
  wl_display_disconnect(display);

  return 0;
}

