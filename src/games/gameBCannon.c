#include "../cglp.h"

char* bcannonTitle = "B CANNON";
char* bcannonDescription = "[Tap]  Turn\n[Hold] Fire";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bcannonCharacters = {
    {   // Cannon frame 1 'a'
        " ll   ",
        "ll    ",
        " l    ",
        " ll   ",
        "l     ",
        "      ",
    },
    {   // Cannon frame 2 'b'
        " ll   ",
        "ll    ",
        " l    ",
        "ll    ",
        "  l   ",
        "      ",
    },
    {   // Target 'c'
        "ll ll ",
        "l l l ",
        " lll  ",
        "l l l ",
        "ll ll ",
        "      ",
    }
};
int bcannonCharactersCount = 3;

Options bcannonOptions = {100, 100, 2, true};

struct BcannonBall {
    Vector pos;
    Vector vel;
    float radius;
    float invincibleTicks;
    bool isAlive;
};

struct BcannonRope {
    Vector from;
    float angle;
    float length;
};

struct BcannonPlayer {
    float x;
    float angle;
    int vx;
    float stopTicks;
    Vector pos;
};

#define MAX_BALL_COUNT 50
BcannonBall[MAX_BALL_COUNT] bcannonBalls;
int bcannonBallIndex;
int bcannonNextBallCount;
int bcannonNextBallCountMax;
float bcannonNextBallTicks;
float bcannonBallSpeed;
BcannonRope bcannonRope;
BcannonPlayer bcannonPlayer;
Vector bcannonTargetPos;

void bcannonUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(bcannonBalls);
        bcannonBallIndex = 0;
        bcannonNextBallCount = 0;
        bcannonNextBallCountMax = 2;
        bcannonNextBallTicks = 40;
        bcannonBallSpeed = 1;
        vectorSet(&bcannonRope.from, 0, 0);
        bcannonRope.angle = 0;
        bcannonRope.length = 0;
        bcannonPlayer.x = 40;
        vectorSet(&bcannonPlayer.pos, 40, 92);
        bcannonPlayer.angle = 0;
        bcannonPlayer.vx = 1;
        bcannonPlayer.stopTicks = 0;
        vectorSet(&bcannonTargetPos, 50, 50);
        barCenterPosRatio = 0;
    }

    color = LIGHT_BLACK;
    rect(0, 0, 100, 5, &scratch);
    rect(0, 95, 100, 5, &scratch);
    rect(0, 5, 5, 90, &scratch);
    rect(95, 5, 5, 90, &scratch);

    color = BLACK;
    if (bcannonRope.length > 0) {
        Collision ropeCl;
        bar(bcannonRope.from.x, bcannonRope.from.y, bcannonRope.length, bcannonRope.angle, &ropeCl);
        if (!ropeCl.isColliding.rect[LIGHT_BLACK]) {
            bcannonRope.length += sqrt(difficulty) * 2;
        } else {
            bcannonRope.length = 0;
        }
    }

    COUNT_IS_ALIVE(bcannonBalls, ballsAlive);
    if (ballsAlive == 0) {  // No active balls
        bcannonNextBallTicks++;
        if (bcannonNextBallTicks > 60) {
            play(POWER_UP);
            bcannonNextBallTicks = 0;
            bcannonNextBallCount++;
            if (bcannonNextBallCount > bcannonNextBallCountMax) {
                bcannonNextBallCount = 1;
                bcannonBallSpeed += 0.2;
                bcannonNextBallCountMax = clamp(bcannonNextBallCountMax + 1, 1, 4);
            }

            float x = 50 - (bcannonNextBallCount - 1) * 10 - 20;
            for (int i = 0; i < bcannonNextBallCount; i++) {
                ASSIGN_ARRAY_ITEM(bcannonBalls, bcannonBallIndex, BcannonBall, b);
                x += 20;
                vectorSet(&b->pos, x, 30);
                vectorSet(&b->vel, 0, bcannonBallSpeed);
                b->radius = 9;
                b->invincibleTicks = 0;
                b->isAlive = true;
                bcannonBallIndex = cgl_wrap(bcannonBallIndex + 1, 0, MAX_BALL_COUNT);
            }
        }
    }

    float nearestDist = 200;
    color = YELLOW;
    BcannonBall* nearestBall = NULL;
    FOR_EACH(bcannonBalls, i) {
        ASSIGN_ARRAY_ITEM(bcannonBalls, i, BcannonBall, b);
        SKIP_IS_NOT_ALIVE(b);

        vectorAdd(&b->pos, b->vel.x, b->vel.y);

        if ((b->pos.x < 5 + b->radius && b->vel.x < 0) ||
            (b->pos.x > 95 - b->radius && b->vel.x > 0)) {
            play(HIT);
            b->vel.x *= -1;
        }
        if ((b->pos.y < 5 + b->radius && b->vel.y < 0) ||
            (b->pos.y > 95 - b->radius && b->vel.y > 0)) {
            play(HIT);
            b->vel.y *= -1;
        }

        b->invincibleTicks--;
        Collision ballCl;
        arc(b->pos.x, b->pos.y, b->radius, 0, M_PI * 2, &ballCl);
        if (ballCl.isColliding.rect[BLACK] &&
            b->invincibleTicks < 0) {
            float radius = b->radius * 0.7;
            if (radius < 3) {
                play(JUMP);
                addScore(bcannonBallIndex, b->pos.x, b->pos.y);
                particle(b->pos.x, b->pos.y, 9, 1, 0, M_PI * 2);
                b->isAlive = false;
                continue;
            }

            play(COIN);
            float s = sqrt(b->vel.x * b->vel.x + b->vel.y * b->vel.y);
            float invincibleTicks = 30 / sqrt(difficulty);
            float oa = cgl_wrap(vectorAngle(&b->vel) - bcannonRope.angle - M_PI, -M_PI, M_PI);

            if (fabs(oa) > M_PI / 2) {
                oa = M_PI - fabs(oa);
            }
            if (fabs(oa) < M_PI / 5) {
                oa = M_PI / 5;
            }

            // Split ball into two
            ASSIGN_ARRAY_ITEM(bcannonBalls, bcannonBallIndex, BcannonBall, newBall1);
            vectorSet(&newBall1->pos, b->pos.x, b->pos.y);
            vectorSet(&newBall1->vel, s, 0);
            rotate(&newBall1->vel, bcannonRope.angle + oa);
            newBall1->radius = radius;
            newBall1->invincibleTicks = invincibleTicks;
            newBall1->isAlive = true;
            bcannonBallIndex = cgl_wrap(bcannonBallIndex + 1, 0, MAX_BALL_COUNT);

            ASSIGN_ARRAY_ITEM(bcannonBalls, bcannonBallIndex, BcannonBall, newBall2);
            vectorSet(&newBall2->pos, b->pos.x, b->pos.y);
            vectorSet(&newBall2->vel, s, 0);
            rotate(&newBall2->vel, bcannonRope.angle - oa);
            newBall2->radius = radius;
            newBall2->invincibleTicks = invincibleTicks;
            newBall2->isAlive = true;
            bcannonBallIndex = cgl_wrap(bcannonBallIndex + 1, 0, MAX_BALL_COUNT);

            b->isAlive = false;
            continue;
        }
        float d = distanceTo(&b->pos, bcannonPlayer.pos.x, bcannonPlayer.pos.y);
        if (d < nearestDist) {
            nearestDist = d;
            nearestBall = b;
        }
    }

    if (nearestBall != NULL) {
        Vector delta;
        vectorSet(&delta, nearestBall->vel.x, nearestBall->vel.y);
        vectorMul(&delta, 8 + bcannonBallSpeed);
        vectorAdd(&delta, nearestBall->pos.x, nearestBall->pos.y);
        vectorAdd(&delta, -bcannonTargetPos.x, -bcannonTargetPos.y);
        vectorMul(&delta, 0.1 * sqrt(difficulty));
        vectorAdd(&bcannonTargetPos, delta.x, delta.y);
    }

    if (input.isJustPressed) {
        play(SELECT);
        bcannonPlayer.vx *= -1;
    }

    if (input.isPressed) {
        if (nearestBall != NULL) {
            bcannonPlayer.stopTicks++;
        }
    } else {
        bcannonPlayer.stopTicks = 0;
        bcannonPlayer.x += bcannonPlayer.vx * sqrt(difficulty);
        if (bcannonPlayer.x < 0) {
            bcannonPlayer.angle++;
            bcannonPlayer.x = 84 + bcannonPlayer.x;
        } else if (bcannonPlayer.x >= 84) {
            bcannonPlayer.angle--;
            bcannonPlayer.x = 84 - bcannonPlayer.x;
        }
        bcannonPlayer.angle = cgl_wrap(bcannonPlayer.angle, 0, 4);
    }

    int angleCase = (int)bcannonPlayer.angle;
    if (angleCase == 0) {
        vectorSet(&bcannonPlayer.pos, bcannonPlayer.x + 8, 92);
    } else if (angleCase == 1) {
        vectorSet(&bcannonPlayer.pos, 8, bcannonPlayer.x + 8);
    } else if (angleCase == 2) {
        vectorSet(&bcannonPlayer.pos, 92 - bcannonPlayer.x, 8);
    } else if (angleCase == 3) {
        vectorSet(&bcannonPlayer.pos, 92, 92 - bcannonPlayer.x);
    }

    color = BLACK;
    character("c", bcannonTargetPos.x, bcannonTargetPos.y, &scratch);

    if (bcannonPlayer.stopTicks == 10) {
        play(LASER);
        bcannonRope.angle = angleTo(&(bcannonPlayer.pos), bcannonTargetPos.x, bcannonTargetPos.y);
        vectorSet(&bcannonRope.from, bcannonPlayer.pos.x, bcannonPlayer.pos.y);
        bcannonRope.length = 1;
    }

    characterOptions.isMirrorX = bcannonPlayer.vx < 0;
    characterOptions.rotation = (int)bcannonPlayer.angle;
    int[2] playerChar;
    playerChar[0] = 'a' + ((int)(ticks / 20) % 2);
    playerChar[1] = 0;

    Collision playerCl;
    character(playerChar, bcannonPlayer.pos.x, bcannonPlayer.pos.y, &playerCl);
    if (playerCl.isColliding.rect[YELLOW]) {
        play(EXPLOSION);
        gameOver();
    }
}

void addGameBCannon() {
    addGame(bcannonTitle, bcannonDescription, bcannonCharacters, bcannonCharactersCount,
            &bcannonOptions, false, &bcannonUpdate);
}
