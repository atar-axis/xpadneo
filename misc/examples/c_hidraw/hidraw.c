/* hidraw test, bypassing the driver for rumble control
 * for Xbox compatible devices only, use at your own risk
 */

#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned char u8;

static struct ff_pack_t {
	char cmd;
	struct {
		u8 weak:1;
		u8 strong:1;
		u8 right:1;
		u8 left:1;
	} enable;
	struct {
		u8 left;
		u8 right;
		u8 strong;
		u8 weak;
	} strength;
	struct {
		u8 sustain_10ms;
		u8 release_10ms;
		u8 loop_count;
	} pulse;
} ff_pack;

WINDOW *input, *output;

static void print_pack(WINDOW * window)
{
	if (window == output) {
		scroll(output);
		wmove(output, LINES - 6, 0);
	}
	wprintw(window,
		"%02X  %d   %u   %u   %u   0   0   0   0  %03d %03d %03d %03d  %03d %03d  %03d",
		ff_pack.cmd,
		ff_pack.enable.strong,
		ff_pack.enable.weak,
		ff_pack.enable.left,
		ff_pack.enable.right,
		ff_pack.strength.strong,
		ff_pack.strength.weak,
		ff_pack.strength.left,
		ff_pack.strength.right,
		ff_pack.pulse.sustain_10ms, ff_pack.pulse.release_10ms, ff_pack.pulse.loop_count);
}

static void print_input()
{
	wmove(input, 0, 0);
	wprintw(input, "--[  MotorEnable  ]---------------[ Strength in %% ][  10ms ][cnt]--\n");
	print_pack(input);
	wprintw(input,
		"\n"
		"    |   |   |   |                   |   |   |   |    |   |    |\n"
		"   STR WEA LTR RTR  -   -   -   -  STR WEA LTR RTR  SUS REL  LOP\n"
		"   [1] [2] [3] [4]                 q/a w/s e/d r/f  h/j k/l  u/i "
		"   <-- keys, or Enter = SEND, Ctrl+C = ABORT");
	wrefresh(input);
}

int hidraw = -1;

int main(int argc, char **argv)
{
	char ch;
	ff_pack.cmd = 0x03;

	ff_pack.enable.strong = 1;
	ff_pack.enable.weak = 1;
	ff_pack.enable.left = 1;
	ff_pack.enable.right = 1;

	ff_pack.strength.strong = 40;
	ff_pack.strength.weak = 30;
	ff_pack.strength.left = 20;
	ff_pack.strength.right = 20;

	ff_pack.pulse.sustain_10ms = 5;
	ff_pack.pulse.release_10ms = 5;
	ff_pack.pulse.loop_count = 3;

	if (argc == 2) {
		hidraw = open(argv[1], O_WRONLY);
	} else {
		fprintf(stderr, "usage: %s /dev/hidraw##\n", argv[0]);
		exit(1);
	}

	if (hidraw < 0) {
		fprintf(stderr, "%s: error %d opening '%s': %s\n", argv[0], errno, argv[1],
			strerror(errno));
		exit(1);
	}

	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();

	output = newwin(LINES - 5, COLS, 0, 0);
	input = newwin(5, COLS, LINES - 5, 0);

	print_input();
	scrollok(output, TRUE);

	while ((ch = wgetch(input)) != 3) {
		switch (ch) {
		case 10:
			print_pack(output);
			write(hidraw, &ff_pack, sizeof(ff_pack));
			wrefresh(output);
			break;
		case '1':
			ff_pack.enable.strong ^= 1;
			break;
		case '2':
			ff_pack.enable.weak ^= 1;
			break;
		case '3':
			ff_pack.enable.left ^= 1;
			break;
		case '4':
			ff_pack.enable.right ^= 1;
			break;
		case 'q':
			if (ff_pack.strength.strong < 100)
				ff_pack.strength.strong++;
			break;
		case 'a':
			if (ff_pack.strength.strong > 0)
				ff_pack.strength.strong--;
			break;
		case 'w':
			if (ff_pack.strength.weak < 100)
				ff_pack.strength.weak++;
			break;
		case 's':
			if (ff_pack.strength.weak > 0)
				ff_pack.strength.weak--;
			break;
		case 'e':
			if (ff_pack.strength.left < 100)
				ff_pack.strength.left++;
			break;
		case 'd':
			if (ff_pack.strength.left > 0)
				ff_pack.strength.left--;
			break;
		case 'r':
			if (ff_pack.strength.right < 100)
				ff_pack.strength.right++;
			break;
		case 'f':
			if (ff_pack.strength.right > 0)
				ff_pack.strength.right--;
			break;
		case 'h':
			if (ff_pack.pulse.sustain_10ms > 0)
				ff_pack.pulse.sustain_10ms--;
			break;
		case 'j':
			if (ff_pack.pulse.sustain_10ms < 255)
				ff_pack.pulse.sustain_10ms++;
			break;
		case 'k':
			if (ff_pack.pulse.release_10ms > 0)
				ff_pack.pulse.release_10ms--;
			break;
		case 'l':
			if (ff_pack.pulse.release_10ms < 255)
				ff_pack.pulse.release_10ms++;
			break;
		case 'u':
			if (ff_pack.pulse.loop_count > 0)
				ff_pack.pulse.loop_count--;
			break;
		case 'i':
			if (ff_pack.pulse.loop_count < 255)
				ff_pack.pulse.loop_count++;
			break;
		}
		print_input();
	}

	delwin(input);
	delwin(output);

	endwin();

	close(hidraw);
	exit(0);
}
