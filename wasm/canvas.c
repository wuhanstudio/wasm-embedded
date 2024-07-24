#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <emscripten.h>

#define NUM_CIRCLES 20

typedef struct Circle {
    int x;
    int y;
    int r;
    int cr;
    int cg;
    int cb;
    int vx;
    int vy;
} circle_t;

circle_t circles[NUM_CIRCLES];

int getRand (int max) {
    return (rand() % max);
}

// Init circle data and start render
int main() {
    printf("Init circles from C \n");
    
    srand(time(NULL));

    for (int i = 0; i < NUM_CIRCLES; i++)
    {
        int radius = getRand(50);
        int x = getRand(1000) + radius;
        int y = getRand(1000) + radius;

        circles[i].x = x;
        circles[i].y = y;
        circles[i].r = radius;

        circles[i].cr = getRand(255);
        circles[i].cg = getRand(255);
        circles[i].cb = getRand(255);

        circles[i].vx = getRand(10);
        circles[i].vy = getRand(10);
    }
    
    // EM_ASM({ render($0, $1); }, NUM_CIRCLES*8, 8);

    // emscripten_run_script("render()");
}

circle_t* getCircles(int canvasWidth, int canvasHeight) {
    for (int i = 0; i < NUM_CIRCLES; i++)
    {
        if ( circles[i].x + circles[i].r >= canvasWidth){
            if (circles[i].vx > 0)
                circles[i].vx = -circles[i].vx;
        }
        if ( circles[i].x - circles[i].r <= 0) {
            if (circles[i].vx < 0)
                circles[i].vx = -circles[i].vx;
        }
        if ( circles[i].y + circles[i].r >= canvasHeight) {
            if (circles[i].vy > 0)
                circles[i].vy = -circles[i].vy;
        }
        if ( circles[i].y - circles[i].r <= 0) {
            if (circles[i].vy < 0)
                circles[i].vy = -circles[i].vy;
        }

        circles[i].x += circles[i].vx;
        circles[i].y += circles[i].vy;
    }
    
    return circles;
}
