#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>



enum {
    FF_DIR_WEST       = 0x4000,
    FF_DIR_WEST_NORTH = 0x6000,
    FF_DIR_NORTH      = 0x8000,
    FF_DIR_NORTH_EAST = 0xA000,
    FF_DIR_EAST       = 0xC000
};

int main(int argc, char *argv[])
{

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



    /* Start */

    printf("Directional Rumble Test\n");

    int fd;
    struct ff_effect effects[10];
    struct input_event play;

    /* prepare effects */
    for (int i = 0; i < 10; i++) {
        effects[i].type = FF_RUMBLE;
        effects[i].id = -1; // set by ioctl
        effects[i].direction = FF_DIR_WEST + i * 0x1000;
        effects[i].u.rumble.strong_magnitude = magnitude;
        effects[i].u.rumble.weak_magnitude = magnitude;
        effects[i].replay.length = 1950;
        effects[i].replay.delay = 0;
    }

    printf("Press ENTER to upload Rumble effects\n");
    getchar();


    /* uploading */
    fd = open(device, O_RDWR);
    for (int i = 0; i < 10; i++) {
        if (ioctl(fd, EVIOCSFF, &effects[i]) == -1) {
            perror("Error while uploading\n");
            return 1;
        }
    }


    printf("Press ENTER to play rumble effects\n");
    getchar();

    for (int i = 0; i < 10; i++) {
        memset(&play, 0, sizeof(play));
        play.type = EV_FF;
        play.code = effects[i].id;
        play.value = 1;

        if (write(fd, (const void*) &play, sizeof(play)) == -1) {
            perror("Error while playing");
            return 1;
        } else {
            printf("effect %d, direction: %04x\n", i, effects[i].direction);
        }

        sleep(1);
    }

    return 0;
}

