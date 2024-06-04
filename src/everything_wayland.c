#include "config.h"
#include "env.h"
#include "hotreload.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <linux/input-event-codes.h>
#include "xdg-shell-protocol.h"

// Wayland state
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_surface *surface;
struct wl_buffer *buffer;
struct wl_shm *shm;
struct wl_seat *seat;
struct wl_keyboard *keyboard;
struct wl_pointer *pointer;
struct wl_callback_listener cb_listener;

// XDG Shell stuff
struct xdg_wm_base *shell;
struct xdg_toplevel *window;

void *pixel_data;
int width = INIT_WIDTH;
int height = INIT_HEIGHT;
double last_frame_time = 0.0;
bool window_should_close = false;
bool input_used = false;

Env env = {0};
AppModule module = {0};

double get_time(void) {
    struct timeval now;
    gettimeofday(&now, NULL);

    double now_ms = now.tv_sec*1000.0+now.tv_usec*(1.0/1000.0);
    return now_ms;
}

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

void reset_input() {
    if (!input_used) return;

    env.key_down = false;
    env.mouse_left_down = false;
    env.mouse_right_down = false;
    env.mouse_moved = false;
}

void render_frame(void) {
    double current_frame_time = get_time();
    double dt = (current_frame_time- last_frame_time) / 1000.0;

    env.delta_time = dt;
    env.buffer = pixel_data;
    env.window_width = width;
    env.window_height = height;

    module.app_update(&env);
    input_used = true;

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, width, height);
    wl_surface_commit(surface);

    reset_input();
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

static void pointer_enter(void *data,
                          struct wl_pointer *wl_pointer,
                          uint32_t serial,
                          struct wl_surface *surface,
                          wl_fixed_t surface_x,
                          wl_fixed_t surface_y) {
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
    (void)surface_x;
    (void)surface_y;

}

static void pointer_leave(void *data,
                          struct wl_pointer *wl_pointer,
                          uint32_t serial,
                          struct wl_surface *surface) {
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
}

static void pointer_motion(void *data,
                           struct wl_pointer *wl_pointer,
                           uint32_t time,
                           wl_fixed_t surface_x,
                           wl_fixed_t surface_y) {
    (void)data;
    (void)wl_pointer;
    (void)time;
    env.mouse_x = surface_x;
    env.mouse_y = surface_y;
}

static void pointer_button(void *data,
                           struct wl_pointer *wl_pointer,
                           uint32_t serial,
                           uint32_t time,
                           uint32_t button,
                           uint32_t state) {
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)time;
    (void)state;

    env.mouse_left_down = button == BTN_LEFT;
    env.mouse_right_down = button == BTN_RIGHT;
}

static void pointer_axis(void *data,
                         struct wl_pointer *wl_pointer,
                         uint32_t time,
                         uint32_t axis,
                         wl_fixed_t value) {
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
    (void)value;
}

static void pointer_frame(void *data,
                          struct wl_pointer *wl_pointer) {
    (void)data;
    (void)wl_pointer;
}

static void pointer_axis_source(void *data,
                                struct wl_pointer *wl_pointer,
                                uint32_t axis_source) {
    (void)data;
    (void)wl_pointer;
    (void)axis_source;
}

static void pointer_axis_stop(void *data,
                              struct wl_pointer *wl_pointer,
                              uint32_t time,
                              uint32_t axis) {
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
}

static void pointer_axis_discrete(void *data,
                                  struct wl_pointer *wl_pointer,
                                  uint32_t axis,
                                  int32_t discrete) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)discrete;
}

static void pointer_axis_value120(void *data,
                                  struct wl_pointer *wl_pointer,
                                  uint32_t axis,
                                  int32_t value120) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)value120;
}

static void pointer_axis_relative_direction(void *data,
        struct wl_pointer *wl_pointer,
        uint32_t axis,
        uint32_t direction) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)direction;
}

struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .button  = pointer_button,
    .motion = pointer_motion,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete,
    .axis_value120 = pointer_axis_value120,
    .axis_relative_direction = pointer_axis_relative_direction,
};

void seat_capabilities_handler(void *data, struct wl_seat *seat,
                               uint32_t capabilities) {
    (void)data;

    bool has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    //bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (has_pointer) {
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer,
                                &pointer_listener, NULL);
    } else if (!has_pointer && pointer != NULL) {
        wl_pointer_release(pointer);
        pointer = NULL;
    } else {
        fprintf(stderr, "ERROR: Device does not have a pointer\n");
    }

    /*
        if (has_keyboard) {
            keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(keyboard,
            	&keyboard_listener, NULL);
        } else {
            fprintf(stderr, "ERROR: Device does not have a keyboard\n");
        }
    */
}

void seat_name_handler(void *data, struct wl_seat *seat, const char *name) {
    (void)data;
    (void)seat;
    (void)name;
}

struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities_handler,
    .name = seat_name_handler,
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 7);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    }
}

static void registry_remover(void *data, struct wl_registry *registry,
                             uint32_t id) {
    (void)data;
    (void)registry;
    (void)id;
}

static const struct wl_registry_listener registry_listener = {registry_handler,
           registry_remover
};

int main(void) {
    last_frame_time = get_time();

    // Loading the app module
    load_module(&module, "./everything.so");
    module.app_init();

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

