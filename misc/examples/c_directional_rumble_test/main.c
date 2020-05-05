#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>


enum {
    FF_DIR_SOUTH      = 0x0000,
    FF_DIR_WEST       = 0x4000,
    FF_DIR_WEST_NORTH = 0x6000,
    FF_DIR_NORTH      = 0x8000,
    FF_DIR_NORTH_EAST = 0xA000,
    FF_DIR_EAST       = 0xC000
};

#define ioctl_sff(fd, effect) ioctl(fd, EVIOCSFF, effect)
#define ioctl_rmff(fd, id)     ioctl(fd, EVIOCRMFF, id)

#include<unistd.h>

struct ff_effect effect = {
    .id = -1, // set by ioctl
    .type = FF_RUMBLE,
    .replay.length = 50,
    .replay.delay = 0,
};

int fd = -1;

void signal_handler(int signo)
{
    switch (signo) {
        case SIGINT: {
            if (fd >= 0) {
                if (effect.id >= 0)
                    ioctl_rmff(fd, effect.id);
                close(fd);
            }
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Error installing handler\n");
        return 1;
    }

    /* Read in arguments */
    if ((argc < 2 || argc > 3))
    {
        printf("./directional_rumble_rest <eventnumber> [<magnitude, 0 to 65535>]\n ");
        return 0;
    }

    char device[80];
    snprintf(device, 80, "/dev/input/event%d", atoi(argv[1]));


    int magnitude = -1;
    if (argc == 3) {
        magnitude = atoi(argv[2]);
    }
    if (magnitude < 0 || magnitude > 65535)
        magnitude = 0xC000;

    printf("Directional Rumble Test\n");

    /* prepare the effect */
    long int direction = FF_DIR_SOUTH;
    effect.direction = direction;
    effect.u.rumble.strong_magnitude = magnitude;

    /* uploading */
    fd = open(device, O_RDWR);
    if (ioctl_sff(fd, &effect) == -1) {
        perror("Error while uploading\n");
        return 1;
    }
    printf("Press ENTER to play rumble effects\n");
    getchar();

    /* Start */
    struct input_event play;
    memset(&play, 0, sizeof(play));
    play.type = EV_FF;
    play.code = effect.id;
    play.value = 1;

    for (int i = 0; i < 3000; i++) {
        printf("direction: 0x%04lX\n", direction);

        effect.direction = direction;
        if (ioctl_sff(fd, &effect) == -1) {
            perror("Error while uploading\n");
            goto error_and_rm_effect;
            return 1;
        }

        play.value = 1;
        if (write(fd, (const void*) &play, sizeof(play)) < 0) {
            perror("Error while playing\n");
            goto error_and_rm_effect;
        }

        direction = (direction + 1024) % 65536;

        struct timespec req = {0};
        req.tv_sec = 0;
        req.tv_nsec = 50 * 1000000L;
        while (nanosleep(&req, (struct timespec *)NULL) == -1) {
            if (errno != EINTR) {
                perror("End...\n");
                goto error_and_rm_effect;
            }
        }
    }

    printf("End...\n");

    ioctl_rmff(fd, effect.id);
    close(fd);
    return 0;

error_and_rm_effect:
    ioctl_rmff(fd, effect.id);
    close(fd);
    return 1;
}
