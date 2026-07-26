#include "../cglp.h"

char* balanceTitle = "BALANCE";
char* balanceDescription = "[Slide] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] balanceCharacters = {
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    }
};
int balanceCharactersCount = 1;

Options balanceOptions = {150, 100, 8, false};

struct BalancePillar {
    float x;
    float vx;
    float angle;
    float length;
    float angleVel;
};

struct BalanceCoin {
    Vector pos;
    Vector vel;
    bool isAlive;
};

#define MAX_COINS 80
BalancePillar balancePillar;
BalanceCoin[MAX_COINS] balanceCoins;
int balanceCoinIndex;
float balanceNextCoinTicks;
float balanceCoinX;
float balanceLx;
float balanceWind;
float balanceMinLength = 20;

void balanceUpdate() {
    Collision scratch;
    barCenterPosRatio = 0.0;
    thickness = 1;

    if (!ticks) {
        balancePillar.x = 50;
        balancePillar.vx = 0;
        balancePillar.angle = 0;
        balancePillar.length = 20;
        balancePillar.angleVel = 0;

        INIT_UNALIVED_ARRAY_FAST(balanceCoins);
        balanceCoinIndex = 0;
        balanceNextCoinTicks = 0;
        if (rnd(0, 1) < 0.5) {
            balanceCoinX = 20;
        } else {
            balanceCoinX = 80;
        }
        balanceLx = 50;
        balanceWind = 0;
    }

    color = LIGHT_BLACK;
    rect(-50, 90, 200, 10, &scratch);

    color = WHITE;
    rect(balanceLx, 90, 1, 10, &scratch);

    float o = input.pos.x - balancePillar.x;
    if (fabs(o) < 99) {
        balancePillar.vx += o * 0.005;
    }

    balanceWind += rnd(-0.01, 0.01);
    balanceWind *= 0.98;
    balancePillar.vx -= balanceWind;
    balancePillar.vx *= 0.95;
    balancePillar.x += balancePillar.vx * difficulty;
    balancePillar.angleVel -= balancePillar.vx * 0.002;
    balancePillar.angleVel += balancePillar.angle * 0.0001 * balancePillar.length;
    balancePillar.angleVel *= 1 - 0.1 * sqrt(difficulty);
    balancePillar.angle += balancePillar.angleVel * difficulty;

    Vector tp;
    vectorSet(&tp, balancePillar.x, 90);
    addWithAngle(&tp, balancePillar.angle - M_PI / 2, balancePillar.length);

    float scr = (50 - tp.x) * 0.2;
    balanceLx = cgl_wrap(balanceLx + scr, -5, 105);
    balancePillar.x += scr;
    tp.x += scr;

    if (balancePillar.length < balanceMinLength) {
        gameOver();
        return;
    }

    color = BLACK;
    thickness = 3;
    bar(balancePillar.x, 90, balancePillar.length, balancePillar.angle - M_PI / 2, &scratch);

    color = RED;
    bar(balancePillar.x, 90, clamp(balancePillar.length, 0, balanceMinLength), balancePillar.angle - M_PI / 2, &scratch);

    color = PURPLE;
    thickness = 1;
    Collision c;
    box(tp.x, tp.y, 5, 5, &c);
    if (c.isColliding.rect[LIGHT_BLACK]) {
        play(EXPLOSION);
        balancePillar.length /= 2;
        balancePillar.angleVel *= -0.5;
        if (balancePillar.length >= balanceMinLength) {
            balancePillar.angle /= 2;
        }
    }

    balanceNextCoinTicks--;
    if (balanceNextCoinTicks < 0) {
        ASSIGN_ARRAY_ITEM(balanceCoins, balanceCoinIndex, BalanceCoin, coin);
        vectorSet(&coin->pos, balanceCoinX, -3);
        vectorSet(&coin->vel, rnd(-1.0, 1.0), 0);
        coin->isAlive = true;
        balanceCoinIndex = cgl_wrap(balanceCoinIndex + 1, 0, MAX_COINS);

        balanceCoinX = cgl_wrap(balanceCoinX + rnd(-1.0, 1.0) + scr, -20, 120);
        balanceNextCoinTicks = 5;
    }

    color = YELLOW;
    FOR_EACH(balanceCoins, i) {
        ASSIGN_ARRAY_ITEM(balanceCoins, i, BalanceCoin, coin);
        SKIP_IS_NOT_ALIVE(coin);

        coin->vel.x += balanceWind / 2;
        vectorAdd(&coin->pos, coin->vel.x, coin->vel.y);
        coin->pos.x += scr;
        coin->vel.y += 0.03;
        vectorMul(&coin->vel, 0.98);

        Collision cl;
        box(coin->pos.x, coin->pos.y, 5, 5, &cl);
        if (cl.isColliding.rect[BLACK] || cl.isColliding.rect[RED] ||
            cl.isColliding.rect[PURPLE]) {

            bool isPurple = cl.isColliding.rect[PURPLE];
            if (isPurple) {
                play(POWER_UP);
            } else {
                play(COIN);
            }

            int times;
            if (isPurple) {
                times = 3;
            } else {
                times = 1;
            }
            for (int t = 0; t < times; t++) {
                addScore(ceil(balancePillar.length), coin->pos.x, coin->pos.y - t * 6);
                if (balancePillar.length < 64) {
                    balancePillar.length++;
                }
            }
            coin->isAlive = false;
            continue;
        }

        if (cl.isColliding.rect[LIGHT_BLACK]) {
            coin->isAlive = false;
        }
    }
}

void addGameBalance() {
    addGame(balanceTitle, balanceDescription, balanceCharacters, balanceCharactersCount,
            &balanceOptions, true, &balanceUpdate);
}
