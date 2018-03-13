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

int main()
{
    printf("Directional Rumble Test\n");

    int fd;
    struct ff_effect effects[5];
    struct input_event play;

    /* prepare effects */
    for (int i = 0; i < 5; i++) {
        effects[i].type = FF_RUMBLE;
        effects[i].id = -1; // set by ioctl
        effects[i].direction = FF_DIR_WEST + i * 0x2000;
        effects[i].u.rumble.strong_magnitude = 0xc000;
        effects[i].u.rumble.weak_magnitude = 0;
        effects[i].replay.length = 1950;
        effects[i].replay.delay = 0;
    }

    printf("Press ENTER to upload Rumble effects\n");
    getchar();


    /* uploading */
    fd = open("/dev/input/event15", O_RDWR);
    for (int i = 0; i < 5; i++) {
        if (ioctl(fd, EVIOCSFF, &effects[i]) == -1) {
            perror("Error\n");
            return 1;
        }
    }
       
       
    printf("Press ENTER to play rumble effects\n");
    getchar();
    
    for (int i = 0; i < 5; i++) {

        memset(&play, 0, sizeof(play));
        play.type = EV_FF;
        play.code = effects[i].id;
        play.value = 1;

        if (write(fd, (const void*) &play, sizeof(play)) == -1) {
            perror("Playing effect");
            return 1;
        } else {
            printf("Playing effect %d\n", i);
        }
        
        sleep(2);
    }

    return 0;
}

