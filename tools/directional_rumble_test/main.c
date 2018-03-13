#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

enum {
    FF_DIR_WEST       = 0x4000,
    FF_DIR_WEST_NORTH = 0x6000,
    FF_DIR_NORTH      = 0x8000,
    FF_DIR_NORTH_EAST = 0xA000,
    FF_DIR_EAST       = 0xC000
};

int main()
{
    printf("Directional Rumble Test\n");

    int fd;
    struct ff_effect effect;
    struct input_event play;

    /* prepare effect */
    effect.type = FF_RUMBLE;
    effect.id = -1;
    effect.direction = FF_DIR_EAST;
    effect.u.rumble.strong_magnitude = 0xc000;
    effect.u.rumble.weak_magnitude = 0;
    effect.replay.length = 5000;
    effect.replay.delay = 1000;


    printf("Press ENTER to upload Rumble effect\n");
    getchar();


    /* uploading */
    fd = open("/dev/input/event15", O_RDWR);

    if (ioctl(fd, EVIOCSFF, &effect) == -1) {
        perror("Error\n");
        return 1;
    }

    printf("Press ENTER to play Rumble effect\n");
    getchar();
    fflush(stdout);

    memset(&play,0,sizeof(play));
    play.type = EV_FF;
    play.code = effect.id;
    play.value = 1;

    if (write(fd, (const void*) &play, sizeof(play)) == -1) {
        perror("Playing effect");
        return 1;
    }

    printf("Press ENTER to stop all effect and loop\n");
    getchar();

    return 0;
}

