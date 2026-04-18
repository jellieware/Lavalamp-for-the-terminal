#include <ncurses.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BLOBS 3

typedef struct {
    float x, y, dx, dy, r;
} Blob;

void init_lava_colors() {
    start_color();
    // Create a 256-color gradient: Black -> Red -> Orange -> White
    for (int i = 0; i < 256; i++) {
        short r = 0, g = 0, b = 0;
        if (i < 100) { // Black to Red
            g = (i * 1000) / 100;
        } else if (i < 150) { // Red to Orange
            g = 1000;
            b = ((i - 100) * 500) / 100;
            r=0;
         }
        else if (i > 149) {
             g = (1000 - (( i - 149) * 10 ));
             r=0;
             b=0;
        }
        init_color(i + 16, r, g, b); // Offset to avoid system colors
        init_pair(i + 16, i + 16, i + 16);
    }
}

int main() {
    initscr();
    noecho();
    curs_set(0);
    timeout(0);
    
    init_lava_colors();

    int w, h;
    getmaxyx(stdscr, h, w);
    w /= 2; // Each "pixel" is 2 spaces wide to stay square

    Blob blobs[BLOBS];
    for (int i = 0; i < BLOBS; i++) {
        blobs[i] = (Blob){rand() % w, rand() % h, 
                   (rand() % 10 - 5) / 10.0, (rand() % 10 - 5) / 10.0, 
                   10.0 + (rand() % 5)};
    }

    while (getch() == ERR) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float sum = 0;
                for (int i = 0; i < BLOBS; i++) {
                    float dx = x - blobs[i].x;
                    float dy = y - blobs[i].y;
                    float d2 = dx*dx + dy*dy;
                    //if (d2 > -1) sum += (blobs[i].r * blobs[i].r) / d2;
 		    if (d2 > -1) sum += (blobs[i].r * blobs[i].r) / d2;
                }
                
                int color_idx = (int)(sum * 50);
                if (color_idx > 254) color_idx = 254;
                if (color_idx < 0) color_idx = 254;

                attron(COLOR_PAIR(color_idx + 1));
                mvaddstr(y, x * 2, "  "); // Double solid block (two spaces)
                attroff(COLOR_PAIR(color_idx + 1));
            }
        }

        for (int i = 0; i < BLOBS; i++) {
            blobs[i].x += blobs[i].dx;
            blobs[i].y += blobs[i].dy;
            if (blobs[i].x < 0 || blobs[i].x >= w) blobs[i].dx *= -1;
            if (blobs[i].y < 0 || blobs[i].y >= h) blobs[i].dy *= -1;
        }
        refresh();
        usleep(90000);
    }

    endwin();
    return 0;
}
