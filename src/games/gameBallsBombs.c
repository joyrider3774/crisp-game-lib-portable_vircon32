#include "../cglp.h"

char* ballsbombsTitle = "BALLS BOMBS";
char* ballsbombsDescription = "[Hold] Walk";

int[8][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ballsbombsCharacters = {
    {
        " llll ",
        "llLlll",
        "lLllll",
        "llllll",
        "llllll",
        " llll ",
    },
    {
        "      ",
        "      ",
        " llll ",
        "lLLlll",
        "llllll",
        " llll ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "    r ",
        "   l  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
    },
    {
        "   r  ",
        "   l  ",
        " llll ",
        "llllll",
        "llllll",
        "  ll  ",
    },
    {
        "    r ",
        "   l  ",
        " llll ",
        "ll l l",
        "ll l l",
        " llll ",
    },
    {
        "   r  ",
        "   l  ",
        " llll ",
        "ll l l",
        "ll l l",
        " llll ",
    }
};
int ballsbombsCharactersCount = 8;

Options ballsbombsOptions = {150, 100, 1, false};

struct BallsbombsBall {
    Vector pos;
    Vector vel;
    bool isAlive;
};

#define MAX_BALL_COUNT 80
BallsbombsBall[MAX_BALL_COUNT] ballsbombsBalls;
int ballsbombsBallIndex;
float ballsbombsNextBallDist;
float ballsbombsPlayerX;
float ballsbombsWalkSpeed;
bool ballsbombsIsBombPlayer;
float ballsbombsBombX;
bool ballsbombsHasBomb;
float ballsbombsNextBombDist;
float ballsbombsExplosionX;
bool ballsbombsHasExplosion;
float ballsbombsExplosionXRadius;
float ballsbombsWallX;
int ballsbombsMultiplier;

void ballsbombsUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(ballsbombsBalls);
        ballsbombsBallIndex = 0;
        ballsbombsNextBallDist = 0;
        ballsbombsPlayerX = 30;
        ballsbombsWalkSpeed = 1;
        ballsbombsHasBomb = false;
        ballsbombsIsBombPlayer = false;
        ballsbombsNextBombDist = 50;
        ballsbombsHasExplosion = false;
        ballsbombsExplosionXRadius = 0;
        ballsbombsWallX = 99;
        ballsbombsMultiplier = 1;
    }

    float sd = sqrt(difficulty);
    float scr;
    if (ballsbombsPlayerX > 50) {
        scr = (ballsbombsPlayerX - 50) * 0.1;
    } else {
        scr = 0;
    }

    if (ballsbombsHasExplosion) {
        ballsbombsExplosionX -= scr;
        ballsbombsExplosionXRadius += 1;
        if (ballsbombsExplosionXRadius > 60) {
            color = LIGHT_RED;
        } else {
            color = RED;
        }

        float oldThickness = thickness;
        thickness = 3.0;
        arc(ballsbombsExplosionX, 87, ballsbombsExplosionXRadius, M_PI - 0.1, M_PI * 2 + 0.1, &scratch);
        if (ballsbombsExplosionXRadius > 64) {
            ballsbombsHasExplosion = false;
            ballsbombsMultiplier = 1;
        }
        thickness = oldThickness;
    }

    color = LIGHT_BLACK;
    rect(0, 90, 150, 10, &scratch);
    ballsbombsWallX = cgl_wrap(ballsbombsWallX - scr, -5, 155);
    rect(ballsbombsWallX, 0, 1, 90, &scratch);

    color = WHITE;
    rect(cgl_wrap(ballsbombsWallX + 99, -5, 155), 90, 1, 10, &scratch);

    color = BLACK;
    if (ballsbombsIsBombPlayer) {
        ballsbombsNextBombDist -= 0;
    } else {
        ballsbombsNextBombDist -= scr;
    }
    if (ballsbombsNextBombDist < 0) {
        ballsbombsBombX = 153;
        ballsbombsHasBomb = true;
        ballsbombsNextBombDist += rnd(150, 180);
    }

    if (ballsbombsHasBomb) {
        ballsbombsBombX -= scr;
        int[2] bombChar;
        bombChar[0] = 'e' + (ticks / 20) % 2;
        bombChar[1] = 0;
        character(bombChar, ballsbombsBombX, 87, &scratch);
    }

    if (input.isJustPressed) {
        play(LASER);
        ballsbombsWalkSpeed = 2;
    }

    float walkStep;
    if (input.isPressed) {
        walkStep = sd * clamp(ballsbombsWalkSpeed, 0, 1);
    } else {
        walkStep = 0;
    }
    ballsbombsPlayerX += walkStep - scr;
    ballsbombsWalkSpeed *= 0.998;

    int[2] playerChar;
    int playerBase;
    if (ballsbombsIsBombPlayer) {
        playerBase = 'g';
    } else {
        playerBase = 'c';
    }
    playerChar[0] = playerBase + (ticks / 20) % 2;
    playerChar[1] = 0;
    Collision playerCol;
    character(playerChar, ballsbombsPlayerX, 87, &playerCol);

    if (playerCol.isColliding.character['e'] || playerCol.isColliding.character['f']) {
        play(POWER_UP);
        ballsbombsIsBombPlayer = true;
        ballsbombsHasBomb = false;
    }

    ballsbombsNextBallDist -= clamp(scr, sd * 0.2, 99);
    if (ballsbombsNextBallDist < 0) {
        ASSIGN_ARRAY_ITEM(ballsbombsBalls, ballsbombsBallIndex, BallsbombsBall, b);
        float posX;
        if (rnd(0, 1) < 0.1) {
            posX = rnd(-20, -3);
        } else {
            posX = rnd(153, 200);
        }
        vectorSet(&b->pos, posX, rnd(10, 40));
        float velXFactor;
        if (posX < 0) {
            velXFactor = 2;
        } else {
            velXFactor = -1;
        }
        vectorSet(&b->vel, velXFactor * rnd(0.3, 0.7), 0);
        b->isAlive = true;
        ballsbombsBallIndex = cgl_wrap(ballsbombsBallIndex + 1, 0, MAX_BALL_COUNT);
        ballsbombsNextBallDist += rnd(24, 32) / sd;
    }

    FOR_EACH(ballsbombsBalls, i) {
        ASSIGN_ARRAY_ITEM(ballsbombsBalls, i, BallsbombsBall, b);
        SKIP_IS_NOT_ALIVE(b);

        b->vel.y += 0.1;
        b->pos.x += b->vel.x * sd - scr;
        b->pos.y += b->vel.y * sd;

        if (b->pos.y > 87 && b->vel.y > 0) {
            play(HIT);
            b->vel.y *= -1.01;
            b->pos.y = 87;
        }

        if (b->pos.x < 3 && b->vel.x < 0) {
            b->vel.x *= -1.01 * 2;
        }
        if (b->pos.x > 147 && b->vel.x > 0) {
            b->vel.x *= -1.01 / 2;
        }

        int[2] ballChar;
        if (b->pos.y > 80 && b->vel.y < 0) {
            ballChar[0] = 'a' + 1;
        } else {
            ballChar[0] = 'a' + 0;
        }
        ballChar[1] = 0;
        Collision ballCol;
        character(ballChar, b->pos.x, clamp(b->pos.y, 0, 87), &ballCol);

        if (ballCol.isColliding.rect[RED] || ballCol.isColliding.rect[LIGHT_RED]) {
            play(COIN);
            addScore(ballsbombsMultiplier, b->pos.x, b->pos.y);
            particle(b->pos.x, b->pos.y, 9, 1, 0, M_PI * 2);
            ballsbombsMultiplier++;
            b->isAlive = false;
        } else if (ballCol.isColliding.character['c'] || ballCol.isColliding.character['d']) {
            play(RANDOM);
            gameOver();
        } else if (ballCol.isColliding.character['g'] || ballCol.isColliding.character['h']) {
            play(EXPLOSION);
            ballsbombsExplosionX = ballsbombsPlayerX;
            ballsbombsExplosionXRadius = 6;
            ballsbombsHasExplosion = true;
            ballsbombsHasBomb = false;
            ballsbombsIsBombPlayer = false;
            b->isAlive = false;
        }
    }
}

void addGameBallsBombs() {
    addGame(ballsbombsTitle, ballsbombsDescription, ballsbombsCharacters,
            ballsbombsCharactersCount, &ballsbombsOptions, false, &ballsbombsUpdate);
}
