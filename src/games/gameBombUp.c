#include "../cglp.h"

char* bombupTitle = "BOMB UP";
char* bombupDescription = "[Tap]\n Throw\n Blast";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bombupCharacters = {
    {   // Player standing 'a'
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {   // Player jumping 'b'
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
    {   // Bomb 'c'
        "  r   ",
        " r    ",
        " ll   ",
        "llll  ",
        "llll  ",
        " ll   ",
    }
};
int bombupCharactersCount = 3;

Options bombupOptions = {100, 100, 90, true};

struct BombupMobileEntity {
    Vector pos;
    Vector vel;
};

struct BombupRock {
    Vector pos;
    Vector vel;
    bool isAlive;
};

struct BombupExplosion {
    Vector pos;
    float ticks;
    bool exists;
};

#define MAX_ROCKS 80
BombupMobileEntity bombupPlayer;
BombupMobileEntity bombupBomb;
bool bombupHasBomb;
BombupExplosion bombupExplosion;
BombupRock[MAX_ROCKS] bombupRocks;
int bombupRockIndex;
float bombupNextRockTicks;
float bombupNextRockX;
float bombupNextRockVelX;
float bombupWallY;
int bombupMultiplier;

void bombupUpdate() {
    Collision scratch;
    if (!ticks) {
        vectorSet(&bombupPlayer.pos, 50, 50);
        vectorSet(&bombupPlayer.vel, 0, 0);

        bombupHasBomb = false;
        bombupExplosion.exists = false;
        INIT_UNALIVED_ARRAY_FAST(bombupRocks);
        bombupRockIndex = 0;
        bombupNextRockTicks = 0;
        bombupNextRockX = 80;
        bombupNextRockVelX = 0;
        bombupWallY = 0;
        bombupMultiplier = 1;
    }

    float scr = 0;
    if (bombupPlayer.pos.y < 40) {
        scr = (40 - bombupPlayer.pos.y) * 0.3;
    } else if (bombupPlayer.pos.y > 60) {
        scr = (60 - bombupPlayer.pos.y) * 0.2;
    }

    // Update player
    bombupPlayer.vel.y += 0.02;
    if ((bombupPlayer.pos.x < 7 && bombupPlayer.vel.x < 0) ||
        (bombupPlayer.pos.x > 93 && bombupPlayer.vel.x > 0)) {
        bombupPlayer.vel.x *= -0.7;
        bombupPlayer.pos.x = clamp(bombupPlayer.pos.x, 7, 93);
    }
    vectorMul(&bombupPlayer.vel, 0.99);
    vectorAdd(&bombupPlayer.pos, bombupPlayer.vel.x, bombupPlayer.vel.y);
    bombupPlayer.pos.y += scr;

    // Draw player
    color = BLACK;
    characterOptions.isMirrorX = bombupPlayer.vel.x < 0;
    int[2] playerChar;
    if (bombupPlayer.vel.y < 0) {
        playerChar[0] = 'a';
    } else {
        playerChar[0] = 'b';
    }
    playerChar[1] = 0;
    character(playerChar, bombupPlayer.pos.x, bombupPlayer.pos.y, &scratch);

    float sideOffset;
    if (bombupPlayer.vel.x < 0) {
        sideOffset = -2;
    } else {
        sideOffset = 2;
    }
    if (!bombupHasBomb) {
        character("c", bombupPlayer.pos.x + sideOffset, bombupPlayer.pos.y, &scratch);
    }

    // Handle input
    if (input.isJustPressed) {
        if (!bombupHasBomb) {
            play(SELECT);
            bombupHasBomb = true;
            float bombX = bombupPlayer.pos.x + sideOffset;
            vectorSet(&bombupBomb.pos, bombX, bombupPlayer.pos.y);
            float velXOffset;
            if (bombupPlayer.vel.x < 0) {
                velXOffset = -0.5;
            } else {
                velXOffset = 0.5;
            }
            bombupBomb.vel.x = bombupPlayer.vel.x + velXOffset;
            bombupBomb.vel.y = bombupPlayer.vel.y;
            bombupMultiplier = 1;
        } else {
            play(POWER_UP);
            bombupExplosion.exists = true;
            vectorSet(&bombupExplosion.pos, bombupBomb.pos.x, bombupBomb.pos.y);
            bombupExplosion.ticks = 0;
            color = LIGHT_RED;
            particle(bombupBomb.pos.x, bombupBomb.pos.y, 20, 3, 0, M_PI * 2);
            float d = distanceTo(&bombupBomb.pos, bombupPlayer.pos.x, bombupPlayer.pos.y);
            if (d > 0) {
                float a = angleTo(&bombupBomb.pos, bombupPlayer.pos.x, bombupPlayer.pos.y);
                addWithAngle(&bombupPlayer.vel, a, 20 / d);
            }
            bombupHasBomb = false;
        }
    }

    // Update bomb
    if (bombupHasBomb) {
        if ((bombupBomb.pos.x < 7 && bombupBomb.vel.x < 0) ||
            (bombupBomb.pos.x > 93 && bombupBomb.vel.x > 0)) {
            bombupBomb.vel.x *= -0.7;
        }
        bombupBomb.vel.y += 0.15;
        vectorMul(&bombupBomb.vel, 0.98);
        vectorAdd(&bombupBomb.pos, bombupBomb.vel.x, bombupBomb.vel.y);
        bombupBomb.pos.y += scr;
        character("c", bombupBomb.pos.x, bombupBomb.pos.y, &scratch);
        if (bombupBomb.pos.y > 103) {
            bombupHasBomb = false;
        }
    }

    // Update explosion
    if (bombupExplosion.exists) {
        bombupExplosion.ticks++;
        float r = sin(bombupExplosion.ticks * 0.15) * 25;
        if (r < 0) {
            bombupExplosion.exists = false;
        } else {
            color = LIGHT_RED;
            arc(bombupExplosion.pos.x, bombupExplosion.pos.y, r, 0, M_PI * 2, &scratch);
        }
    }

    // Update rocks
    bombupNextRockTicks--;
    bombupNextRockVelX += rnd(-0.1, 0.1) * sqrt(difficulty);
    bombupNextRockVelX *= 0.99;
    bombupNextRockX += bombupNextRockVelX;
    if ((bombupNextRockX < 7 && bombupNextRockVelX < 0) ||
        (bombupNextRockX > 93 && bombupNextRockVelX > 0)) {
        bombupNextRockVelX *= -0.7;
        bombupNextRockX = clamp(bombupNextRockX, 7, 93);
    }

    if (bombupNextRockTicks < 0) {
        float isBottomThreshold;
        if (bombupPlayer.pos.y > 30) {
            isBottomThreshold = 0.05;
        } else {
            isBottomThreshold = 0.9;
        }
        bool isBottom = rnd(0, 1) > isBottomThreshold;
        ASSIGN_ARRAY_ITEM(bombupRocks, bombupRockIndex, BombupRock, r);
        float startY;
        if (isBottom) {
            startY = 103;
        } else {
            startY = -3;
        }
        vectorSet(&r->pos, bombupNextRockX, startY);
        float yvel;
        if (isBottom) {
            yvel = -rnd(0.5, sqrt(difficulty));
        } else {
            yvel = bombupPlayer.vel.y / 2;
        }
        vectorSet(&r->vel, 0, yvel);
        r->isAlive = true;
        bombupRockIndex = cgl_wrap(bombupRockIndex + 1, 0, MAX_ROCKS);
        bombupNextRockTicks = rnd(8, 10) / difficulty;
    }

    // Update and draw rocks
    color = RED;
    FOR_EACH(bombupRocks, i) {
        ASSIGN_ARRAY_ITEM(bombupRocks, i, BombupRock, r);
        SKIP_IS_NOT_ALIVE(r);

        vectorAdd(&r->pos, r->vel.x, r->vel.y);
        r->vel.y += 0.01;
        vectorMul(&r->vel, 0.99);
        vectorAdd(&r->pos, r->vel.x, r->vel.y);
        r->pos.y += scr;

        float angle = vectorAngle(&r->vel);
        particle(r->pos.x, r->pos.y, 0.1, vectorLength(&r->vel) * -0.5,
                angle, angle);

        Collision c;
        box(r->pos.x, r->pos.y, 5, 5, &c);
        if (c.isColliding.rect[LIGHT_RED]) {
            play(COIN);
            addScore(bombupMultiplier, r->pos.x, r->pos.y);
            bombupMultiplier++;
            particle(r->pos.x, r->pos.y, 9, 1, 0, M_PI * 2);
            r->isAlive = false;
            continue;
        } else if (c.isColliding.character['a'] || c.isColliding.character['b']) {
            play(EXPLOSION);
            gameOver();
            return;
        }

        if (r->pos.y < -50 || r->pos.y > 150) {
            r->isAlive = false;
        }
    }

    // Draw walls
    bombupWallY += scr;
    color = LIGHT_BLACK;
    for (int i = 0; i < 18; i++) {
        box(3, i * 6 + ((int)bombupWallY % 6), 4, 4, &scratch);
        box(97, i * 6 + ((int)bombupWallY % 6), 4, 4, &scratch);
    }
}

void addGameBombup() {
    addGame(bombupTitle, bombupDescription, bombupCharacters, bombupCharactersCount,
            &bombupOptions, false, &bombupUpdate);
}
