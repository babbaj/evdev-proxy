#include <cstring>
#include <thread>

#include <linux/input.h>
#include <linux/uinput.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <fcntl.h>
#include <unistd.h>

int emit(int fd, int type, int code, int val)
{
    input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    return write(fd, &ie, sizeof(ie));
}

void press_combo(int fd) {

    // press down (possibly unnecessary)
    emit(fd, EV_KEY, KEY_LEFTCTRL, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, KEY_RIGHTCTRL, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    // press up
    emit(fd, EV_KEY, KEY_LEFTCTRL, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, KEY_RIGHTCTRL, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

struct Device {
    libevdev_uinput* uidev;
    int fd;
};

Device create_proxy(libevdev* kb_dev, int kb_fd) {
    /*constexpr uinput_setup usetup {
            .id = {
                    .bustype = BUS_USB,
                    .vendor = 0x1234,
                    .product = 0x666
            },
            .name{"cute device"}
    };*/

    const int ui_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ui_fd < 0) {
        printf("open error (%s)\n", strerror(errno));
        exit(1);
    }

    libevdev_uinput *uidev;
    if (int err = libevdev_uinput_create_from_device(kb_dev, ui_fd, &uidev); err != 0) {
        printf("libevdev_uinput_create_from_device error (%s)\n", strerror(-err));
        exit(1);
    }

    sleep(1);

    return Device {
        .uidev = uidev,
        .fd = ui_fd
    };
}

int main() {
    const int kb_fd = open("/dev/input/by-id/usb-Logitech_USB_Keyboard-event-kbd", O_RDONLY);
    if (kb_fd < 0) {
        printf("Failed to open keyboard device (%s)\n", strerror(errno));
        exit(1);
    }
    const int mouse_fd = open("/dev/input/by-id/usb-Logitech_G203_LIGHTSYNC_Gaming_Mouse_206B37804B42-if01-event-kbd", O_RDONLY);
    if (mouse_fd < 0) {
        printf("Failed to open mouse device (%s)\n", strerror(errno));
        exit(1);
    }


    libevdev *kb_dev = nullptr;
    if (int err = libevdev_new_from_fd(kb_fd , &kb_dev); err < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-err));
        exit(1);
    }

    libevdev *mouse_dev = nullptr;
    if (int err = libevdev_new_from_fd(mouse_fd , &mouse_dev); err < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-err));
        exit(1);
    }


    const Device proxy = create_proxy(kb_dev, kb_fd);
    const char* path = libevdev_uinput_get_devnode(proxy.uidev);
    printf("Path = %s\n", path);
    constexpr auto symlink_path = "/dev/input/by-id/cute-kbd";
    remove(symlink_path);
    if (symlink(path, symlink_path) < 0) {
        printf("Failed to create symlink (%d), (%s)\n", errno, strerror(errno));
        exit(1);
    }

    puts("grabbing keyboard");
    libevdev_grab(kb_dev, LIBEVDEV_GRAB);
    sleep(3);

    // pretty sure I don't need multiple threads with O_NONBLOCK
    std::thread kb_thread([&] {
        int err = 0;
        do {
            input_event ev{};
            err = libevdev_next_event(kb_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (err == 0) {
                err = libevdev_uinput_write_event(proxy.uidev, ev.type, ev.code, ev.value);
            }
        } while (err == 1 || err == 0 || err == -EAGAIN);
    });

    std::thread mouse_thread([&] {
        int err = 0;
        do {
            input_event ev{};
            err = libevdev_next_event(mouse_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (err == 0) {
                // that weird key to the right of space bar
                if (ev.type == EV_KEY && ev.code == KEY_COMPOSE) {
                    if (ev.value == 1) {
                        press_combo(proxy.fd);
                        //puts("sent combo");
                    }
                } else {
                    err = libevdev_uinput_write_event(proxy.uidev, ev.type, ev.code, ev.value);
                }
            }

        } while (err == 1 || err == 0 || err == -EAGAIN);
    });

    kb_thread.join();
    mouse_thread.join();

    // this gets done anyways when the process exits
    libevdev_uinput_destroy(proxy.uidev);
    libevdev_grab(kb_dev, LIBEVDEV_UNGRAB);
    close(kb_fd);
    close(mouse_fd);
    libevdev_free(mouse_dev);
    libevdev_free(kb_dev);
}
