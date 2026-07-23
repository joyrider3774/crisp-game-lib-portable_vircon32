#include "../cglp.h"

char* barollTitle = "BAROLL";
char* barollDescription = "[Tap]  Jump\n[Hold] Slow down";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] barollCharacters = {
    {   // Barrel 'a'
        " llll ",
        "l    l",
        "l ll l",
        "llllll",
        "llllll",
        " llll ",
    },
    {   // Runner frame 1 'b'
        "  l   ",
        "  l   ",
        " lll  ",
        " l l  ",
        "  l   ",
        " l    ",
    },
    {   // Runner frame 2 'c'
        "    l ",
        "   l  ",
        " lllll",
        "l ll  ",
        " l  l ",
        "l   l ",
    }
};
int barollCharactersCount = 3;

Options barollOptions = {200, 100, 9, false};

enum BarollBarrelMode {
    BAROLL_FALL,
    BAROLL_ROLL
};

struct Barrel {
    Vector pos;
    float vy;
    float speed;
    BarollBarrelMode mode;
    float angle;
    bool isAlive;
};

enum BarollPlayerMode {
    BAROLL_RUN,
    BAROLL_JUMPING
};

#define MAX_BARREL_COUNT 80
Barrel[MAX_BARREL_COUNT] barollBarrels;
int barollBarrelIndex;
float barollBarrelAddingTicks;
Vector barollPos;
Vector barollVel;
BarollPlayerMode barollMode;
float barollBx;
float barollAnim;

void barollUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(barollBarrels);
        barollBarrelIndex = 0;
        barollBarrelAddingTicks = 0;
        barollPos.x = 9;
        barollPos.y = 86;
        barollVel.x = 0;
        barollVel.y = 0;
        barollMode = BAROLL_RUN;
        barollBx = 0;
        barollAnim = 0;
    }

    rect(0, 90, 200, 9, &scratch);
    float df = sqrt(difficulty);
    barollBarrelAddingTicks -= df;

    if (barollBarrelAddingTicks < 0) {
        play(LASER);
        ASSIGN_ARRAY_ITEM(barollBarrels, barollBarrelIndex, Barrel, b);
        float minX;
        if (barollMode == BAROLL_RUN) {
            minX = 10;
        } else {
            minX = 100;
        }
        b->pos.x = rnd(minX, 200);
        b->pos.y = -5;
        b->vy = 0;
        b->speed = rnd(1, df);
        b->mode = BAROLL_FALL;
        b->angle = rnd(0, 99);
        b->isAlive = true;
        barollBarrelIndex = cgl_wrap(barollBarrelIndex + 1, 0, MAX_BARREL_COUNT);
        barollBarrelAddingTicks += rndi(30, 90);
    }

    if (input.isPressed) {
        barollVel.x = df * 1;
    } else {
        barollVel.x = df * 2;
    }
    score += barollVel.x - df;

    FOR_EACH(barollBarrels, i) {
        ASSIGN_ARRAY_ITEM(barollBarrels, i, Barrel, b);
        SKIP_IS_NOT_ALIVE(b);

        if (b->mode == BAROLL_FALL) {
            b->vy += b->speed * 0.2;
            b->vy *= 0.92;
            b->pos.y += b->vy * sqrt(df);
            if (b->pos.y > 85) {
                play(SELECT);
                b->pos.y = 86;
                b->mode = BAROLL_ROLL;
            }
        } else {
            b->pos.x -= b->speed * df;
            b->angle += b->speed * df * 0.2;
        }
        b->pos.x -= barollVel.x;

        characterOptions.rotation = 3 - ((int)b->angle % 4);
        character("a", b->pos.x, b->pos.y, &scratch);

        if (b->pos.x <= -5) {
            b->isAlive = false;
        }
    }

    if (barollMode == BAROLL_RUN) {
        if (input.isJustPressed) {
            play(JUMP);
            barollMode = BAROLL_JUMPING;
            barollVel.y = -3.6;
        }
    } else {
        barollPos.y += barollVel.y;
        if (input.isPressed) {
            barollVel.y += 0.1;
        } else {
            barollVel.y += 0.2;
        }
        if (barollPos.y > 85) {
            barollPos.y = 86;
            if (input.isPressed) {
                play(JUMP);
                barollVel.y = -3;
            } else {
                barollMode = BAROLL_RUN;
            }
        }
    }

    // Player animation and collision
    float animStep;
    if (input.isPressed) {
        animStep = 0.1;
    } else {
        animStep = 0.2;
    }
    float modeFactor;
    if (barollMode == BAROLL_RUN) {
        modeFactor = 1;
    } else {
        modeFactor = 0.5;
    }
    barollAnim += df * animStep * modeFactor;
    characterOptions.rotation = 0;
    int[2] playerChar;
    playerChar[0] = 'b' + ((int)floor(barollAnim) % 2);
    playerChar[1] = 0;
    character(playerChar, barollPos.x, barollPos.y, &scratch);
    if (scratch.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
    }

    barollBx -= barollVel.x;
    if (barollBx < -9) {
        barollBx += 200;
    }
    color = LIGHT_BLACK;
    rect(barollBx, 90, 3, 9, &scratch);
    color = BLACK;
}

void addGameBaroll() {
    addGame(barollTitle, barollDescription, barollCharacters, barollCharactersCount,
            &barollOptions, false, &barollUpdate);
}
