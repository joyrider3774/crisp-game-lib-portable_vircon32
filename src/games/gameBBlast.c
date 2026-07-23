#include "../cglp.h"

char* bblastTitle = "B BLAST";
char* bblastDescription = "[Hold]\n Select area\n[Release]\n Erase balls";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bblastCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      "
}};
int bblastCharactersCount = 1;

Options bblastOptions = {100, 100, 5, true};

struct BblastBall {
    Vector pos;
    Vector vel;
    float radius;
    float targetRadius;
    bool isRed;
    bool isAlive;
};

#define MAX_BALL_COUNT 50
BblastBall[MAX_BALL_COUNT] bblastBalls;
int bblastBallIndex;
float bblastNextBallTicks;
float bblastNextRedBallTicks;
Vector bblastPos;
float bblastRadius;
float bblastRadiusSlowTicks;

void bblastUpdate() {
    Collision scratch;
    // Never reads a Collision result anywhere in this file - ball hits
    // are detected via distanceTo() instead (see the collision loop and
    // the tap-to-pop check below), so the engine's own O(n^2) hitbox
    // scan (see checkHitBox() in cglp.c) is pure waste here. Restored
    // automatically when the next real game starts, via resetDrawState()
    // in initInGame().
    hasCollision = false;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(bblastBalls);
        bblastBallIndex = 0;
        bblastNextRedBallTicks = 0;
        bblastNextBallTicks = 60;
        vectorSet(&bblastPos, 0, 0);
        bblastRadius = -1;
        bblastRadiusSlowTicks = 0;
    }

    bblastNextRedBallTicks--;
    if (bblastNextRedBallTicks < 0) {
        play(JUMP);
        ASSIGN_ARRAY_ITEM(bblastBalls, bblastBallIndex, BblastBall, b);
        vectorSet(&b->pos, rnd(19, 80), 10);
        vectorSet(&b->vel, 0, 0);
        b->radius = 0;
        b->targetRadius = 9;
        b->isRed = true;
        b->isAlive = true;
        bblastBallIndex = cgl_wrap(bblastBallIndex + 1, 0, MAX_BALL_COUNT);
        bblastNextRedBallTicks = 300 / difficulty;
    }

    bblastNextBallTicks--;
    if (bblastNextBallTicks < 0) {
        play(HIT);
        ASSIGN_ARRAY_ITEM(bblastBalls, bblastBallIndex, BblastBall, b);
        vectorSet(&b->pos, rnd(9, 90), rnd(20, 80));
        vectorSet(&b->vel, 0, 0);
        b->radius = 0;
        b->targetRadius = rnd(5, 10);
        b->isRed = false;
        b->isAlive = true;
        bblastBallIndex = cgl_wrap(bblastBallIndex + 1, 0, MAX_BALL_COUNT);
        bblastNextBallTicks = 30 / difficulty;
    }

    bool hasRed = false;
    FOR_EACH(bblastBalls, i) {
        ASSIGN_ARRAY_ITEM(bblastBalls, i, BblastBall, b);
        SKIP_IS_NOT_ALIVE(b);

        b->radius += (b->targetRadius - b->radius) * 0.05;

        // Ball collision
        FOR_EACH(bblastBalls, j) {
            ASSIGN_ARRAY_ITEM(bblastBalls, j, BblastBall, ob);
            SKIP_IS_NOT_ALIVE(ob);
            if (b == ob) continue;

            float d = distanceTo(&b->pos, ob->pos.x, ob->pos.y) - b->radius - ob->radius;
            if (d < 0) {
                float angle = angleTo(&ob->pos, b->pos.x, b->pos.y);
                addWithAngle(&b->vel, angle, -d / b->radius);
            }
        }

        // Wall collision
        if ((b->pos.x < b->radius && b->vel.x < 0) ||
            (b->pos.x > 99 - b->radius && b->vel.x > 0)) {
            b->vel.x *= -0.7;
        }
        if (b->pos.y > 110 - b->radius && b->vel.y > 0) {
            b->vel.y *= -0.7;
        }

        // Physics
        b->vel.y += 0.05;
        vectorMul(&b->vel, 0.95);
        float velLength = sqrt(b->vel.x * b->vel.x + b->vel.y * b->vel.y);
        if (velLength > 1) {
            vectorMul(&b->vel, 0.5);
        }
        vectorAdd(&b->pos, b->vel.x, b->vel.y);

        if (b->isRed) {
            hasRed = true;
            if (b->pos.y > 99 - b->radius) {
                play(COIN);
                addScore(10, b->pos.x, b->pos.y);
                b->isRed = false;
            }
        }

        if (b->isRed) {
            color = RED;
        } else {
            color = BLACK;
        }
        thickness = b->radius * 0.4;
        arc(b->pos.x, b->pos.y, b->radius, 0, M_PI * 2, &scratch);

        if (b->pos.y - b->radius < 0) {
            play(RANDOM);
            color = RED;
            text("X", b->pos.x, 7, &scratch);
            gameOver();
        }
    }

    if (!hasRed) {
        bblastNextRedBallTicks = 0;
    }

    if (input.isJustPressed) {
        play(SELECT);
        bblastPos.x = clamp(input.pos.x, 0, 99);
        bblastPos.y = clamp(input.pos.y, 0, 99);
        bblastRadius = 1;
    }

    if (bblastRadius >= 0 && input.isPressed) {
        bblastRadius += 0.5 * difficulty * (1 - bblastRadiusSlowTicks / 60);
        color = YELLOW;
        thickness = 2;
        arc(bblastPos.x, bblastPos.y, bblastRadius, 0, M_PI * 2, &scratch);
    }

    if (bblastRadius >= 0 && input.isJustReleased) {
        play(LASER);
        bblastRadiusSlowTicks = 60;
        int multiplier = 1;

        Vector[MAX_BALL_COUNT] redPositions;
        int redCount = 0;

        FOR_EACH(bblastBalls, i) {
            ASSIGN_ARRAY_ITEM(bblastBalls, i, BblastBall, b);
            SKIP_IS_NOT_ALIVE(b);

            if (distanceTo(&b->pos, bblastPos.x, bblastPos.y) - b->radius - bblastRadius < 0) {
                if (b->isRed) {
                    play(EXPLOSION);
                    redPositions[redCount].x = b->pos.x;
                    redPositions[redCount].y = b->pos.y;
                    redCount++;
                    color = RED;
                    particle(b->pos.x, b->pos.y, 30, 4, M_PI * 2, 0.4);
                } else {
                    play(COIN);
                    color = BLACK;
                    particle(b->pos.x, b->pos.y, b->radius, b->radius * 0.3, M_PI * 2, 0.4);
                }
                addScore(multiplier, b->pos.x, b->pos.y);
                multiplier++;
                b->isAlive = false;
            }
        }

        // Spawn new balls from red ball positions
        for (int i = 0; i < redCount; i++) {
            for (int j = 0; j < 9; j++) {
                ASSIGN_ARRAY_ITEM(bblastBalls, bblastBallIndex, BblastBall, b);
                vectorSet(&b->pos, redPositions[i].x + rnd(-3, 3),
                                  redPositions[i].y + rnd(-3, 3));
                vectorSet(&b->vel, 0, 0);
                b->radius = 0;
                b->targetRadius = rnd(12, 15);
                b->isRed = false;
                b->isAlive = true;
                bblastBallIndex = cgl_wrap(bblastBallIndex + 1, 0, MAX_BALL_COUNT);
            }
        }
    }

    if (bblastRadiusSlowTicks > 0) {
        bblastRadiusSlowTicks--;
    }
}

void addGameBBlast() {
    addGame(bblastTitle, bblastDescription, bblastCharacters, bblastCharactersCount,
            &bblastOptions, true, &bblastUpdate);
}
