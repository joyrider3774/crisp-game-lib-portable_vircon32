#include "../cglp.h"

char* chargebeamTitle = "CHARGE BEAM";
char* chargebeamDescription = "[Tap]     Shot\n[Hold]    Charge\n[Release] Fire";

int[5][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] chargebeamCharacters = {
    {
        "      ",
        "rllbb ",
        "lllccb",
        "llyl b",
        "      ",
        "      "
    },
    {
        "  r rr",
        "rrrrrr",
        "  grr ",
        "  grr ",
        "rrrrrr",
        "  r rr"
    },
    {
        " LLLL ",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        " LLLL "
    },
    {
        "   bbb",
        "  bccb",
        "bbllcb",
        "bcllcb",
        "  bccb",
        "   bbb"
    },
    {
        "      ",
        "l llll",
        "l llll",
        "      ",
        "      ",
        "      "
    }
};
int chargebeamCharactersCount = 5;

Options chargebeamOptions = {200, 60, 1, false};

struct ChargebeamObj {
    float x;
    float size;
    int type;
    bool isAlive;
};

struct ChargebeamInhalingCoin {
    float x;
    float vx;
    bool isAlive;
};

#define MAX_OBJS 80
#define MAX_INHALING_COINS 50

ChargebeamObj[MAX_OBJS] chargebeamObjs;
ChargebeamInhalingCoin[MAX_INHALING_COINS] chargebeamInhalingCoins;
float chargebeamNextObjDist;
int chargebeamCoinMultiplier;
int chargebeamCoinPenaltyMultiplier;
int chargebeamEnemyMultiplier;
float chargebeamShotX;
int chargebeamShotSize;
float chargebeamCharge;
float chargebeamPenaltyVx;
int chargebeamPrevType;

struct ChargebeamPlayer {
    float x;
    float vx;
};

void chargebeamAddCoinPenalty(float x, bool isShotHit) {
    play(POWER_UP);
    particle(x, 30, 9, 5, 0, M_PI * 2);
    if (isShotHit) {
        chargebeamShotX = -1;
    }
    addScore(-chargebeamCoinPenaltyMultiplier, x + sqrt(chargebeamCoinPenaltyMultiplier) * 4, 50);
    if (chargebeamCoinPenaltyMultiplier < 64) {
        chargebeamCoinPenaltyMultiplier *= 2;
    }
    chargebeamPenaltyVx += 0.5;
}

void chargebeamUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(chargebeamObjs);
        INIT_UNALIVED_ARRAY_FAST(chargebeamInhalingCoins);
        chargebeamObjs[0].x = 150;
        chargebeamObjs[0].size = 1;
        chargebeamObjs[0].type = 'e';
        chargebeamObjs[0].isAlive = true;
        for (int i = 0; i < 3; i++) {
            chargebeamObjs[i + 1].x = 160 + i * 10;
            chargebeamObjs[i + 1].size = 1;
            chargebeamObjs[i + 1].type = 'c';
            chargebeamObjs[i + 1].isAlive = true;
        }
        chargebeamNextObjDist = 30;
        chargebeamCoinMultiplier = chargebeamEnemyMultiplier = chargebeamCoinPenaltyMultiplier = 1;
        chargebeamShotX = -1;
        chargebeamShotSize = 0;
        chargebeamCharge = 0;
        chargebeamPenaltyVx = 0;
        chargebeamPrevType = 'c';
    }

    if (chargebeamShotX < 0) {
        text("BEAM", 30, 55, &scratch);
        if (input.isPressed && chargebeamCharge < 99) {
            play(HIT);
            chargebeamCharge += difficulty * 1.5;
            color = CYAN;
            float c = chargebeamCharge;
            float x = 60;
            if (c < 25) {
                rect(x, 53, c, 5, &scratch);
                chargebeamShotSize = 1;
            } else {
                rect(x, 53, 25, 5, &scratch);
                c -= 25;
                x += 27;
                chargebeamShotSize = 1;
                while (c > 9) {
                    rect(x, 53, 9, 5, &scratch);
                    x += 11;
                    c -= 9;
                    chargebeamShotSize++;
                }
                rect(x, 53, c, 5, &scratch);
                chargebeamShotSize++;
            }
            color = BLACK;
        } else if (chargebeamCharge > 0) {
            play(LASER);
            chargebeamShotX = 10;
            chargebeamCharge = 0;
            chargebeamCoinMultiplier = chargebeamEnemyMultiplier = chargebeamCoinPenaltyMultiplier = 1;
        }
    }

    if (chargebeamShotX >= 0) {
        chargebeamShotX += difficulty * 2.5;
        float x = chargebeamShotX;
        if (chargebeamShotSize == 1) {
            character("e", chargebeamShotX, 30, &scratch);
        } else {
            for (int i = 0; i < chargebeamShotSize; i++) {
                if (chargebeamShotSize % 2 == 1 && i == 0) {
                    character("d", x, 30, &scratch);
                    x += 6;
                } else {
                    if (i % 2 == chargebeamShotSize % 2) {
                        character("d", x, 27, &scratch);
                    } else {
                        character("d", x, 33, &scratch);
                        x += 6;
                    }
                }
            }
        }
        if (chargebeamShotX > 203) {
            chargebeamShotX = -1;
        }
    }

    chargebeamPenaltyVx -= 0.02;
    if (chargebeamPenaltyVx < 0) {
        chargebeamPenaltyVx = 0;
    }
    float vx = (-difficulty - chargebeamPenaltyVx) * 0.5;
    color = RED;
    for (int i = 0; i < ceil(chargebeamPenaltyVx * 2 + 0.1); i++) {
        text("<", i * 6 + 3, 48, &scratch);
    }
    color = BLACK;
    chargebeamNextObjDist += vx;

    if (chargebeamNextObjDist < 0) {
        int type;
        if (chargebeamPrevType != 'c' && rnd(0, 1) < 0.5) {
            type = 'c';
        } else {
            type = 'e';
        }
        chargebeamPrevType = type;
        int c = rndi(3, 9);
        float x = 200;
        if (type == 'c') {
            for (int i = 0; i < c; i++) {
                FOR_EACH(chargebeamObjs, j) {
                    ASSIGN_ARRAY_ITEM(chargebeamObjs, j, ChargebeamObj, o);
                    if (!o->isAlive) {
                        o->x = x;
                        o->size = 1;
                        o->type = 'c';
                        o->isAlive = true;
                        break;
                    }
                }
                x += 10;
            }
        } else {
            for (int i = 0; i < c; i++) {
                float size;
                if (rnd(0, 1) < 0.3) {
                    size = rndi(2, 6);
                } else {
                    size = 1;
                }
                FOR_EACH(chargebeamObjs, j) {
                    ASSIGN_ARRAY_ITEM(chargebeamObjs, j, ChargebeamObj, o);
                    if (!o->isAlive) {
                        o->x = x;
                        o->size = size;
                        o->type = 'e';
                        o->isAlive = true;
                        break;
                    }
                }
                x += 10 + ceil((size - 1) / 2) * 6;
            }
        }
        x += 10;
        chargebeamNextObjDist = x - 200;
    }

    float minCoinX = 999;
    float minEnemyX = 999;

    FOR_EACH(chargebeamObjs, i) {
        ASSIGN_ARRAY_ITEM(chargebeamObjs, i, ChargebeamObj, o);
        SKIP_IS_NOT_ALIVE(o);

        o->x += vx;
        if (o->type == 'c') {
            if (o->x < minCoinX) {
                minCoinX = o->x;
            }
        } else {
            if (o->x < minEnemyX) {
                minEnemyX = o->x;
            }
        }

        if (o->type == 'c') {
            Collision col;
            character("c", o->x, 30, &col);
            if (col.isColliding.character['d'] || col.isColliding.character['e']) {
                chargebeamAddCoinPenalty(o->x, col.isColliding.character['e']);
                o->isAlive = false;
            } else if (col.isColliding.character['b'] || col.isColliding.character['c']) {
                o->isAlive = false;
            }
        } else {
            float x = o->x;
            Collision col;
            memset(&col, 0, sizeof(Collision));
            for (int j = 0; j < o->size; j++) {
                Collision tempCol;
                if ((int)o->size % 2 == 1 && j == 0) {
                    character("b", x, 30, &tempCol);
                    for (int k = 0; k < ASCII_CHARACTER_COUNT; k++) {
                        col.isColliding.character[k] |= tempCol.isColliding.character[k];
                    }
                    x += 6;
                } else {
                    if (j % 2 == (int)o->size % 2) {
                        character("b", x, 27, &tempCol);
                        for (int k = 0; k < ASCII_CHARACTER_COUNT; k++) {
                            col.isColliding.character[k] |= tempCol.isColliding.character[k];
                        }
                    } else {
                        character("b", x, 33, &tempCol);
                        for (int k = 0; k < ASCII_CHARACTER_COUNT; k++) {
                            col.isColliding.character[k] |= tempCol.isColliding.character[k];
                        }
                        x += 6;
                    }
                }
            }

            if (col.isColliding.character['d'] || col.isColliding.character['e']) {
                play(EXPLOSION);
                if (col.isColliding.character['e']) {
                    chargebeamShotX = -1;
                }
                if (o->size <= chargebeamShotSize) {
                    particle(o->x, 30, o->size * 3, 1, 0, M_PI * 2);
                    addScore(chargebeamEnemyMultiplier * o->size, o->x, 30);
                    chargebeamEnemyMultiplier++;
                    if (o->size > 1) {
                        chargebeamShotSize -= o->size;
                        if (chargebeamShotSize <= 0) {
                            chargebeamShotX = -1;
                        }
                    }
                    o->isAlive = false;
                } else {
                    o->size -= chargebeamShotSize;
                    chargebeamShotX = -1;
                }
            }
            if (col.isColliding.character['b'] || col.isColliding.character['c']) {
                o->isAlive = false;
            }
        }
    }

    if (minCoinX > 200 && minEnemyX > 200) {
        chargebeamNextObjDist = 0;
    }

    if (minCoinX < minEnemyX) {
        FOR_EACH(chargebeamObjs, i) {
            ASSIGN_ARRAY_ITEM(chargebeamObjs, i, ChargebeamObj, o);
            SKIP_IS_NOT_ALIVE(o);
            if (o->type == 'c' && o->x < minEnemyX) {
                FOR_EACH(chargebeamInhalingCoins, j) {
                    ASSIGN_ARRAY_ITEM(chargebeamInhalingCoins, j, ChargebeamInhalingCoin, ic);
                    if (!ic->isAlive) {
                        ic->x = o->x;
                        ic->vx = -1;
                        ic->isAlive = true;
                        break;
                    }
                }
                o->isAlive = false;
            }
        }
    }

    // Draw player and check for collision
    Collision playerCl;
    character("a", 10, 30, &playerCl);
    if (playerCl.isColliding.character['b']) {
        play(RANDOM);
        gameOver();
    }

    FOR_EACH(chargebeamInhalingCoins, i) {
        ASSIGN_ARRAY_ITEM(chargebeamInhalingCoins, i, ChargebeamInhalingCoin, ic);
        SKIP_IS_NOT_ALIVE(ic);

        ic->x += ic->vx;
        ic->vx -= 0.1;
        Collision col;
        character("c", ic->x, 30, &col);
        if (col.isColliding.character['d'] || col.isColliding.character['e']) {
            chargebeamAddCoinPenalty(ic->x, col.isColliding.character['e']);
            ic->isAlive = false;
        } else if (ic->x < 10) {
            play(COIN);
            addScore(chargebeamCoinMultiplier, 10 + sqrt(chargebeamCoinMultiplier) * 4, 30);
            if (chargebeamCoinMultiplier < 64) {
                chargebeamCoinMultiplier *= 2;
            }
            ic->isAlive = false;
        }
    }
}

void addGameChargeBeam() {
    addGame(chargebeamTitle, chargebeamDescription, chargebeamCharacters, chargebeamCharactersCount,
            &chargebeamOptions, false, &chargebeamUpdate);
}
