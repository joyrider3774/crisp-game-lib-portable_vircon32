#include "../cglp.h"

char* ctowerTitle = "C TOWER";
char* ctowerDescription = "[Hold] Climb";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ctowerCharacters = {
    {
        "  ll  ",
        "  l  l",
        "lllll ",
        "  l   ",
        "ll ll ",
        "     l"
    },
    {
        "  ll  ",
        "l l   ",
        " lllll",
        "  l   ",
        " l lll",
        "l     "
    }
};
int ctowerCharactersCount = 2;

Options ctowerOptions = {100, 100, 50, true};

struct CtowerPlayer {
    float angle;
    float va;
    float tva;
    float y;
    float height;
    float speed;
};

struct CtowerWall {
    float angle;
    float y;
    float width;
    bool isFloating;
    bool isBonus;
    bool isAlive;
};

#define CTOWER_MAX_WALL_COUNT 64
CtowerWall[CTOWER_MAX_WALL_COUNT] ctowerWalls;
int ctowerWallIndex;
float ctowerNextWallDist;
int ctowerNextBonusCount;
int ctowerMultiplier;
float ctowerRadius = 40.0;
CtowerPlayer ctowerPlayer;

void ctowerUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(ctowerWalls);
        ctowerWallIndex = 0;
        ctowerPlayer.angle = 0.0;
        ctowerPlayer.height = 0.0;
        ctowerPlayer.y = 80.0;
        ctowerPlayer.va = -1.0;
        ctowerPlayer.tva = -1.0;
        ctowerPlayer.speed = 3.0;
        ctowerNextWallDist = 0.0;
        ctowerNextBonusCount = 1;
        ctowerMultiplier = 1;
    }

    if (input.isJustPressed) {
        play(LASER);
    }

    float speedTarget;
    if (input.isPressed) {
        speedTarget = 9.0;
    } else {
        speedTarget = 1.0;
    }
    ctowerPlayer.speed += (speedTarget * sqrt(difficulty) - ctowerPlayer.speed) * 0.2;
    ctowerPlayer.va += (ctowerPlayer.tva - ctowerPlayer.va) * 0.2;
    if (ctowerPlayer.y > 80.0) {
        ctowerPlayer.y -= 0.01;
    }
    ctowerPlayer.angle += ctowerPlayer.va * sqrt(difficulty) * 0.05;
    float scr = ctowerPlayer.speed * 0.3;
    ctowerPlayer.height += scr;
    ctowerNextWallDist -= scr;

    if (ctowerNextWallDist < 0) {
        bool isBonus = false;
        ctowerNextBonusCount--;
        if (ctowerNextBonusCount < 0) {
            isBonus = true;
            ctowerNextBonusCount = rndi(9, 12);
        }

        ASSIGN_ARRAY_ITEM(ctowerWalls, ctowerWallIndex, CtowerWall, w);
        w->angle = rnd(0.0, M_PI * 2.0);
        w->width = rnd(0.5, 1.0) * M_PI / 2.0;
        w->y = -5.0;
        w->isFloating = isBonus || rnd(0.0, 1.0) < 0.3;
        w->isBonus = isBonus;
        w->isAlive = true;
        ctowerWallIndex = cgl_wrap(ctowerWallIndex + 1, 0, CTOWER_MAX_WALL_COUNT);
        ctowerNextWallDist += rnd(30.0, 40.0);
    }
    FOR_EACH(ctowerWalls, i) {
        ASSIGN_ARRAY_ITEM(ctowerWalls, i, CtowerWall, w);
        SKIP_IS_NOT_ALIVE(w);

        w->y += scr;
        float wa = ctowerPlayer.angle - w->angle;
        float fa = clamp(cgl_wrap(wa - w->width / 2.0, 0.0, M_PI * 2.0), M_PI / 2.0, (M_PI / 2.0) * 3.0);
        float ta = clamp(cgl_wrap(wa + w->width / 2.0, 0.0, M_PI * 2.0), M_PI / 2.0, (M_PI / 2.0) * 3.0);

        if (fa < ta) {
            float floatScale;
            if (w->isFloating) {
                floatScale = 1.2;
            } else {
                floatScale = 1.1;
            }
            float fx = sin(fa) * ctowerRadius * floatScale;
            float tx = sin(ta) * ctowerRadius * floatScale;
            if (w->isBonus) {
                color = YELLOW;
            } else {
                color = PURPLE;
            }
            rect(fx + 50.0, w->y, tx - fx, -5.0, &scratch);
        }
    }
    float ho = fmod(ctowerPlayer.height, 400.0) - 100.0;
    // Draw backgrounds
    color = CYAN;
    rect(50.0 - ctowerRadius, ho, ctowerRadius * 2.0, -100.0, &scratch);
    rect(50.0 - ctowerRadius, fmod(ho + 200.0, 400.0), ctowerRadius * 2.0, -100.0, &scratch);

    color = LIGHT_CYAN;
    rect(50.0 - ctowerRadius, fmod(ho + 100.0, 400.0), ctowerRadius * 2.0, -100.0, &scratch);
    rect(50.0 - ctowerRadius, fmod(ho + 300.0, 400.0), ctowerRadius * 2.0, -100.0, &scratch);
    color = WHITE;

    float a = cgl_wrap(ctowerPlayer.angle, -M_PI, M_PI);
    if (a > -M_PI / 2.0 && a < M_PI / 2.0) {
        rect(50.0 + sin(a) * ctowerRadius, ho, 1.0, -100.0, &scratch);
    }
    a = cgl_wrap(ctowerPlayer.angle + M_PI / 2.0, -M_PI, M_PI);
    if (a > -M_PI / 2.0 && a < M_PI / 2.0) {
        rect(50.0 + sin(a) * ctowerRadius, fmod(ho + 100.0, 400.0), 1.0, -100.0, &scratch);
    }
    a = cgl_wrap(ctowerPlayer.angle + M_PI, -M_PI, M_PI);
    if (a > -M_PI / 2.0 && a < M_PI / 2.0) {
        rect(50.0 + sin(a) * ctowerRadius, fmod(ho + 200.0, 400.0), 1.0, -100.0, &scratch);
    }
    a = cgl_wrap(ctowerPlayer.angle + (M_PI / 2.0) * 3.0, -M_PI, M_PI);
    if (a > -M_PI / 2.0 && a < M_PI / 2.0) {
        rect(50.0 + sin(a) * ctowerRadius, fmod(ho + 300.0, 400.0), 1.0, -100.0, &scratch);
    }

    FOR_EACH(ctowerWalls, i) {
        ASSIGN_ARRAY_ITEM(ctowerWalls, i, CtowerWall, w);
        SKIP_IS_NOT_ALIVE(w);

        if (w->isFloating) {
            float wa = ctowerPlayer.angle - w->angle;
            float fa = clamp(cgl_wrap(wa - w->width / 2.0, -M_PI, M_PI), -M_PI / 2.0, M_PI / 2.0);
            float ta = clamp(cgl_wrap(wa + w->width / 2.0, -M_PI, M_PI), -M_PI / 2.0, M_PI / 2.0);

            if (fa < ta) {
                float fx = sin(fa) * ctowerRadius * 1.2;
                float tx = sin(ta) * ctowerRadius * 1.2;
                if (w->isBonus) {
                    color = LIGHT_YELLOW;
                } else {
                    color = LIGHT_PURPLE;
                }
                float cfx = clamp(fx + 53.0, 50.0 - ctowerRadius, 50.0 + ctowerRadius);
                float ctx = clamp(tx + 53.0, 50.0 - ctowerRadius, 50.0 + ctowerRadius);
                rect(cfx, w->y + 6.0, ctx - cfx, -5.0, &scratch);
            }
        }
    }

    color = BLACK;
    characterOptions.isMirrorX = ctowerPlayer.va < 0;
    int[2] pc;
    pc[0] = 'a' + (int)(ctowerPlayer.height / 9.0) % 2;
    pc[1] = 0;
    character(pc, 50.0 + 30.0 * ctowerPlayer.va, ctowerPlayer.y, &scratch);
    characterOptions.isMirrorX = false;

    if (ctowerPlayer.y > 97.0) {
        play(EXPLOSION);
        gameOver();
    }

    FOR_EACH(ctowerWalls, i) {
        ASSIGN_ARRAY_ITEM(ctowerWalls, i, CtowerWall, w);
        SKIP_IS_NOT_ALIVE(w);

        float wa = ctowerPlayer.angle - w->angle;
        float fa = clamp(cgl_wrap(wa - w->width / 2.0, -M_PI, M_PI), -M_PI / 2.0, M_PI / 2.0);
        float ta = clamp(cgl_wrap(wa + w->width / 2.0, -M_PI, M_PI), -M_PI / 2.0, M_PI / 2.0);

        if (fa < ta) {
            float floatScale;
            if (w->isFloating) {
                floatScale = 1.2;
            } else {
                floatScale = 1.1;
            }
            float fx = sin(fa) * ctowerRadius * floatScale;
            float tx = sin(ta) * ctowerRadius * floatScale;

            if (!w->isFloating) {
                color = LIGHT_PURPLE;
                float cfx = clamp(fx + 51.0, 50.0 - ctowerRadius, 50.0 + ctowerRadius);
                float ctx = clamp(tx + 51.0, 50.0 - ctowerRadius, 50.0 + ctowerRadius);
                rect(cfx, w->y + 2.0, ctx - cfx, -5.0, &scratch);
            }

            if (w->isBonus) {
                color = YELLOW;
            } else {
                color = PURPLE;
            }
            hasCollision = true;
            Collision c;
            rect(fx + 50.0, w->y, tx - fx, -5.0, &c);

            if (c.isColliding.character['a'] || c.isColliding.character['b']) {
                if (w->isBonus) {
                    play(POWER_UP);
                    ctowerMultiplier++;
                    w->isBonus = false;
                } else if (!w->isFloating) {
                    play(COIN);
                    if (ctowerPlayer.tva > 0) {
                        ctowerPlayer.angle = w->angle - w->width / 2.0 - 0.7;
                    } else {
                        ctowerPlayer.angle = w->angle + w->width / 2.0 + 0.7;
                    }
                    ctowerPlayer.tva *= -1.0;
                    ctowerPlayer.y = clamp(w->y + 6.0, 0.0, 99.0);
                }
            }
        }

        if (w->y > 105.0) {
            w->isAlive = false;
        }
    }

    color = BLACK;
    int[16] scoreText;
    strcpy(scoreText, "x");
    strcat(scoreText, intToChar(ctowerMultiplier));
    text(scoreText, 3, 9, &scratch);
    addScore(scr * ctowerMultiplier, 0.0, 0.0);
}

void addGameCTower() {
    addGame(ctowerTitle, ctowerDescription, ctowerCharacters, ctowerCharactersCount,
            &ctowerOptions, false, &ctowerUpdate);
}
