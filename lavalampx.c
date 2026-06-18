#include <ncurses.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <locale.h> // Needed for printing Unicode solid blocks

#define BLOBS 5

typedef struct {
    float base_x;       
    float x, y;
    float r;
    float pulse_speed;  
    float phase_shift;  
    float sway_speed;   
    float phase;
    float time_pos;        
    float total_path_time; 
    float travel_time;     
    float hold_time;       
} Blob;

void init_lava_colors() {
    start_color();
    for (int i = 0; i < 256; i++) {
        short r = 0, g = 0, b = 0;
        
        if (i < 40) {
            r = 0; g = 0; b = 0;
        } else if (i < 70) {
            r = 0;
            g = ((i - 40) * 40) / 30;   
            b = ((i - 40) * 450) / 30;  
        } else if (i < 145) {
            r = 0;
            g = 40 + ((i - 70) * 80) / 75;    
            b = 450 + ((i - 70) * 350) / 75;  
        } else if (i < 220) {
            r = 0;
            g = 120 - ((i - 145) * 80) / 75; 
            b = 800 - ((i - 145) * 350) / 75; 
        } else if (i < 245) {
            r = 0;
            g = 40 - ((i - 220) * 40) / 25;
            b = 450 - ((i - 220) * 450) / 25;
        } else {
            r = 0; g = 0; b = 0;
        }
        
        init_color(i + 16, r, g, b); 
        init_pair(i + 16, i + 16, i + 16);
    }
}

int main() {
    setlocale(LC_ALL, ""); // Initialize wide character support for solid blocks
    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color.\n");
        return 1;
    }
    
    init_lava_colors();

    int w, h;
    getmaxyx(stdscr, h, w);
    if (w <= 0) w = 80;
    if (h <= 0) h = 24;

    // FIX: Because each pixel is now 2 characters wide, we halve our working width bounds
    int render_w = w / 2; 

    Blob blobs[BLOBS];
    srand(time(NULL));
    for (int i = 0; i < BLOBS; i++) {
        if (render_w > 16) {
            blobs[i].base_x = 8 + (rand() % (render_w - 16));
        } else {
            blobs[i].base_x = render_w / 2;
        }
        blobs[i].x = blobs[i].base_x;
        blobs[i].y = rand() % h;
        
        // Balanced sizing radius for square rendering math
        blobs[i].r = 10.5f + (rand() % 2);
        
        blobs[i].pulse_speed = 0.05f + ((rand() % 100) / 100.0f) * 0.1f; 
        blobs[i].phase_shift = (rand() % 360) * (3.14159f / 180.0f);
        blobs[i].sway_speed = 0.5f + ((rand() % 100) / 100.0f) * 1.0f;

        blobs[i].travel_time = 4.0f + (rand() % 3);       
        blobs[i].hold_time = 5.0f + ((rand() % 50) / 10.0f); 
        
        blobs[i].total_path_time = (blobs[i].travel_time * 2.0f) + (blobs[i].hold_time * 2.0f);
        blobs[i].time_pos = ((rand() % 1000) / 1000.0f) * blobs[i].total_path_time; 
    }

    float t = 0;
    float dt = 0.033f; 
    float c = 0.0;
    while (getch() == ERR) {
        c += 20.0f;
        t += 0.005f; 

        // Render pass over the square screen matrix
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < render_w; x++) {
                float sum = 0;
                
                for (int i = 0; i < BLOBS; i++) {
                    float dx = x - blobs[i].x;
                    float dy = y - blobs[i].y;
                    
                    // FIXED: Removed the squash factors entirely. Normal 1:1 circle distance.
                    float d2 = (dx * dx) + (dy * dy) + 0.1f;
                    float ripple = sinf(atan2f(dy, dx) * 4.0 + c + blobs[i].phase) * 0.3;
                    float fluctuation = 1.5f + sinf((t * blobs[i].pulse_speed * 10.0f) + blobs[i].phase_shift) * 1.0f;
                    float dynamic_r = blobs[i].r + fluctuation + ripple;
                    
                    sum += (dynamic_r * dynamic_r) / d2;
                }

                int color_idx = (int)(sum * 45);
                if (color_idx > 254) color_idx = 254;
                if (color_idx < 0) color_idx = 0;

                // FIX: Print double solid blocks to form perfectly square pixel clusters
                attron(COLOR_PAIR(color_idx + 16));
                mvaddstr(y, x * 2, "  ");
                attroff(COLOR_PAIR(color_idx + 16));
            }
        }

        // Timeline position math
        for (int i = 0; i < BLOBS; i++) {
            float sway_offset = sinf((t * blobs[i].sway_speed) + blobs[i].phase_shift) * 2.0f;
            blobs[i].x = blobs[i].base_x + sway_offset;

            blobs[i].time_pos += dt;
            if (blobs[i].time_pos >= blobs[i].total_path_time) {
                blobs[i].time_pos -= blobs[i].total_path_time;
            }

            // FIX: Push targets completely off screen boundaries based on max blob sizes
            float top_y = -8.0f; 
            float bottom_y = h + 8.0f;
            float progress = 0.0f;

            if (blobs[i].time_pos < blobs[i].travel_time) {
                // Moving Down
                float linear_t = blobs[i].time_pos / blobs[i].travel_time;
                progress = (1.0f - cosf(linear_t * 3.14159f)) / 2.0f; 
                blobs[i].y = top_y + progress * (bottom_y - top_y);
            } 
            else if (blobs[i].time_pos < (blobs[i].travel_time + blobs[i].hold_time)) {
                // Stuck completely hidden past the bottom edge
                blobs[i].y = bottom_y;
            } 
            else if (blobs[i].time_pos < (blobs[i].travel_time * 2.0f + blobs[i].hold_time)) {
                // Moving Up
                float current_phase_time = blobs[i].time_pos - (blobs[i].travel_time + blobs[i].hold_time);
                float linear_t = current_phase_time / blobs[i].travel_time;
                progress = (1.0f - cosf(linear_t * 3.14159f)) / 2.0f;
                blobs[i].y = bottom_y - progress * (bottom_y - top_y);
            } 
            else {
                // Stuck completely hidden past the top edge
                blobs[i].y = top_y;
            }
        }

        refresh();
        usleep(66000); 
    }

    endwin();
    return 0;
}
