#include "../cglp.h"

char* counterbTitle = "COUNTER B";
char* counterbDescription = "[Hold] Beam";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] counterbCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int counterbCharactersCount = 0;

Options counterbOptions = {200, 100, 10, true};

struct CounterbEnemy {
    Vector pos;
    float angle;
    float speed;
    float beamLength;
    bool isAlive;
};

struct CounterbPlayer {
    Vector pos;
    float speed;
    float beamLength;
    float baseX;
    int invincibleTicks;
};

struct CounterbCounter {
    Vector pos;
    float radius;
    CounterbEnemy* enemy;
    bool hasPos;
};

#define COUNTER_B_MAX_ENEMY_COUNT 64
CounterbEnemy[COUNTER_B_MAX_ENEMY_COUNT] counterbEnemies;
int counterbEnemyIndex;
float counterbNextEnemyTicks;
CounterbPlayer counterbPlayer;
CounterbCounter counterbCounter;
float counterbScrX;
int counterbMultiplier;

void counterbUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(counterbEnemies);
        counterbEnemyIndex = 0;
        counterbPlayer.pos.x = 20;
        counterbPlayer.pos.y = 60;
        counterbPlayer.speed = 0;
        counterbPlayer.beamLength = 0;
        counterbPlayer.baseX = 30;
        counterbPlayer.invincibleTicks = 0;

        // Add initial enemy
        ASSIGN_ARRAY_ITEM(counterbEnemies, counterbEnemyIndex, CounterbEnemy, e);
        e->pos.x = 205;
        e->pos.y = counterbPlayer.pos.y;
        e->angle = M_PI;
        e->speed = 0.5;
        e->beamLength = 0;
        e->isAlive = true;
        counterbEnemyIndex = cgl_wrap(counterbEnemyIndex + 1, 0, COUNTER_B_MAX_ENEMY_COUNT);

        counterbNextEnemyTicks = 9;
        counterbCounter.hasPos = false;
        counterbCounter.radius = 0;
        counterbCounter.enemy = NULL;
        counterbMultiplier = 1;
        counterbScrX = 0;
    }

    float scr = (counterbPlayer.pos.x - counterbPlayer.baseX) * 0.1;
    counterbPlayer.baseX += (30 - counterbPlayer.baseX) * 0.001;

    if (counterbPlayer.pos.x < 0) {
        play(EXPLOSION);
        counterbPlayer.pos.x = 0;
        gameOver();
    }

    color = YELLOW;
    rect(0, 90, 200, 10, &scratch);
    counterbScrX = cgl_wrap(counterbScrX - scr, 0, 200);
    color = WHITE;
    rect(counterbScrX, 90, 2, 10, &scratch);

    float beamSpeed = 4 * sqrt(difficulty);
    float speedTarget;
    if (input.isPressed) {
        speedTarget = 0.2;
    } else {
        speedTarget = 2;
    }
    counterbPlayer.speed += (speedTarget * sqrt(sqrt(difficulty)) - counterbPlayer.speed) * 0.2;
    counterbPlayer.pos.x += counterbPlayer.speed - scr;

    if (!counterbCounter.hasPos) {
        if (input.isPressed) {
            if (counterbPlayer.beamLength <= 0) {
                play(SELECT);
                color = CYAN;
                particle(counterbPlayer.pos.x, counterbPlayer.pos.y, 20, 3, 0, 0.3);
            }
            counterbPlayer.beamLength = clamp(counterbPlayer.beamLength + beamSpeed, 0, 300);
        } else {
            counterbPlayer.beamLength = 0;
        }
    } else {
        counterbPlayer.beamLength = counterbCounter.pos.x - counterbPlayer.pos.x;
        if (counterbPlayer.beamLength < 0 && counterbCounter.enemy != NULL) {
            counterbCounter.enemy->beamLength = -9999;
            counterbCounter.enemy = NULL;
        }
    }

    if (counterbPlayer.beamLength > 0) {
        color = LIGHT_CYAN;
        rect(counterbPlayer.pos.x, counterbPlayer.pos.y - 1, counterbPlayer.beamLength, 3, &scratch);
        color = CYAN;
        box(counterbPlayer.pos.x + counterbPlayer.beamLength, counterbPlayer.pos.y, 5, 5, &scratch);
    }

    if (counterbCounter.hasPos) {
        counterbCounter.pos.x -= scr;
        color = PURPLE;
        arc(counterbCounter.pos.x, counterbCounter.pos.y, counterbCounter.radius, ticks * 0.1, ticks * 0.1 + M_PI * 2, &scratch);
        particle(counterbCounter.pos.x, counterbCounter.pos.y, 1, counterbCounter.radius * 0.1, 0, M_PI * 2);

        if (counterbCounter.enemy == NULL) {
            counterbCounter.radius -= sqrt(difficulty) * 2;
            if (counterbCounter.radius < 1) {
                counterbCounter.hasPos = false;
            }
        } else {
            float radiusStep;
            if (input.isPressed) {
                radiusStep = 1;
            } else {
                radiusStep = -2;
            }
            counterbCounter.radius = clamp(
                counterbCounter.radius + radiusStep * sqrt(difficulty),
                0,
                30
            );
            if (counterbCounter.radius < 1) {
                if (counterbCounter.enemy != NULL) {
                    counterbCounter.enemy->beamLength = -9999;
                }
                counterbCounter.hasPos = false;
            }
        }
    }

    counterbNextEnemyTicks--;
    if (counterbNextEnemyTicks < 0) {
        ASSIGN_ARRAY_ITEM(counterbEnemies, counterbEnemyIndex, CounterbEnemy, e);
        if (rnd(0, 1) < 0.6) {
            e->pos.x = rnd(150, 300);
            e->pos.y = -5;
        } else {
            e->pos.x = 205;
            e->pos.y = rnd(0, 85);
        }

        Vector targetPos;
        if (rnd(0, 1) < 0.05) {
            targetPos.x = counterbPlayer.pos.x + 9;
            targetPos.y = counterbPlayer.pos.y;
        } else {
            targetPos.x = e->pos.x - rnd(30, 150);
            targetPos.y = counterbPlayer.pos.y;
        }

        e->angle = angleTo(&e->pos, targetPos.x, targetPos.y);
        e->speed = rnd(1, sqrt(difficulty));
        e->beamLength = -rnd(0, 1) * sqrt(difficulty) * 20;
        e->isAlive = true;
        counterbEnemyIndex = cgl_wrap(counterbEnemyIndex + 1, 0, COUNTER_B_MAX_ENEMY_COUNT);
        counterbNextEnemyTicks = rnd(30, 40) / difficulty;
    }

    FOR_EACH(counterbEnemies, i) {
        ASSIGN_ARRAY_ITEM(counterbEnemies, i, CounterbEnemy, e);
        SKIP_IS_NOT_ALIVE(e);

        addWithAngle(&e->pos, e->angle, e->speed);
        e->pos.x -= scr;

        if (counterbCounter.enemy == e && counterbCounter.hasPos) {
            e->beamLength = distanceTo(&e->pos, counterbCounter.pos.x, counterbCounter.pos.y);
            e->speed *= 0.8;
        } else {
            e->beamLength += beamSpeed * 0.2;
            if (e->beamLength - beamSpeed * 0.3 < 0 && e->beamLength >= 0) {
                play(LASER);
                color = RED;
                particle(e->pos.x, e->pos.y, 9, 3, e->angle, 0.5);
            }
        }

        if (e->beamLength > 0) {
            color = LIGHT_RED;
            Vector beamEnd;
            vectorSet(&beamEnd, e->pos.x, e->pos.y);
            addWithAngle(&beamEnd, e->angle, e->beamLength);
            line(e->pos.x, e->pos.y, beamEnd.x, beamEnd.y, &scratch);
            color = RED;
            Collision beamColl;
            box(beamEnd.x, beamEnd.y, 5, 5, &beamColl);

            if (!counterbCounter.hasPos && beamColl.isColliding.rect[CYAN]) {
                play(POWER_UP);
                counterbMultiplier = 1;
                counterbCounter.hasPos = true;
                counterbCounter.pos.x = counterbPlayer.pos.x + counterbPlayer.beamLength;
                counterbCounter.pos.y = counterbPlayer.pos.y;
                counterbCounter.enemy = e;
                counterbCounter.radius = 1;
                color = PURPLE;
                particle(counterbCounter.pos.x, counterbCounter.pos.y, 30, 5, 0, M_PI * 2);
            }

            if (counterbCounter.enemy != e &&
                (beamColl.isColliding.rect[PURPLE] || beamEnd.y > 90 || beamEnd.x < -99)) {
                e->beamLength = -9999;
            }
        }

        color = BLACK;
        Vector barPos;
        vectorSet(&barPos, e->pos.x, e->pos.y);
        addWithAngle(&barPos, e->angle + (M_PI / 5) * 4, 3);
        thickness = 1;
        bar(barPos.x, barPos.y, 2, e->angle, &scratch);
        thickness = 2;
        Collision barColl;
        bar(e->pos.x, e->pos.y, 3, e->angle, &barColl);

        if (barColl.isColliding.rect[PURPLE] || barColl.isColliding.rect[LIGHT_CYAN]) {
            if (barColl.isColliding.rect[PURPLE]) {
                play(COIN);
                addScore(counterbMultiplier, e->pos.x, e->pos.y);
                if (counterbMultiplier < 64) {
                    counterbMultiplier *= 2;
                }
            } else {
                play(HIT);
            }
            particle(e->pos.x, e->pos.y, 9, 1, 0, M_PI * 2);
            if (counterbCounter.enemy == e) {
                counterbCounter.enemy = NULL;
            }
            e->isAlive = false;
            continue;
        }

        if (e->beamLength < -999) {
            if (e->speed < 1) {
                e->speed += (1 - e->speed) * 0.1;
            }
            if (e->angle > 0) {
                e->angle += (M_PI - e->angle) * 0.05;
            }
        }

        if (e->pos.x < -5) {
            e->isAlive = false;
        }
    }

    Vector playerBarPos;
    vectorSet(&playerBarPos, counterbPlayer.pos.x, counterbPlayer.pos.y);
    addWithAngle(&playerBarPos, -(M_PI / 5) * 4, 3);

    if (counterbPlayer.invincibleTicks > 0 && counterbPlayer.invincibleTicks % 10 < 5) {
        color = TRANSPARENT;
    } else {
        color = BLUE;
    }
    box(playerBarPos.x, playerBarPos.y, 3, 2, &scratch);

    counterbPlayer.invincibleTicks--;

    Collision playerColl;
    box(counterbPlayer.pos.x, counterbPlayer.pos.y, 5, 3, &playerColl);
    if (playerColl.isColliding.rect[LIGHT_RED] &&
        counterbPlayer.invincibleTicks < 0 &&
        !(counterbCounter.hasPos &&
          distanceTo(&counterbCounter.pos, counterbPlayer.pos.x, counterbPlayer.pos.y) < counterbCounter.radius + 9)) {
        play(EXPLOSION);
        particle(counterbPlayer.pos.x, counterbPlayer.pos.y, 50, 4, 0, M_PI * 2);
        counterbPlayer.baseX -= 10;
        counterbPlayer.pos.x -= 10;
        counterbPlayer.invincibleTicks = 60;
    }
}

void addGameCounterB() {
    addGame(counterbTitle, counterbDescription, counterbCharacters, counterbCharactersCount,
            &counterbOptions, false, &counterbUpdate);
}
