#include "../cglp.h"

char* catapultTitle = "CATAPULT";
char* catapultDescription = "[Tap] Throw";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] catapultCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int catapultCharactersCount = 1;

Options catapultOptions = {200, 100, 2, false};

#define MAX_BARS 32

struct CatapultBar {
    Vector pos;
    float width;
    bool isPassed;
    bool isAlive;
};

struct CatapultBall {
    Vector pos;
    Vector vel;
    CatapultBar* bar;
    float barPos;
    float grv;
};

CatapultBar[MAX_BARS] catapultBars;
CatapultBall catapultBall;
float catapultNextBarDist;
float catapultBaseScore;
int catapultPassedCount;
int catapultNextBarIndex;

void catapultInitBar(CatapultBar* bar, float x, float y, float width) {
    vectorSet(&bar->pos, x, y);
    bar->width = width;
    bar->isPassed = false;
    bar->isAlive = true;
}

void catapultUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(catapultBars);
        catapultNextBarIndex = 0;

        catapultInitBar(&catapultBars[0], 200, 60, 20);
        catapultBars[0].isPassed = true;

        vectorSet(&catapultBall.pos, 200, 60);
        vectorSet(&catapultBall.vel, 0, 0);
        catapultBall.bar = &catapultBars[0];
        catapultBall.barPos = -10;
        catapultBall.grv = 0.1;

        catapultBaseScore = 1;
        catapultPassedCount = 0;
        catapultNextBarDist = 50;
        catapultNextBarIndex = cgl_wrap(catapultNextBarIndex + 1, 0, MAX_BARS);
    }

    bool ip = input.isPressed;
    float scr = difficulty * 0.05;
    if (catapultBall.pos.x > 30) {
        scr += (catapultBall.pos.x - 30) * 0.1;
    }
    catapultBall.pos.x -= scr;

    if (catapultBall.bar != NULL) {
        float barPosStep;
        if (ip) {
            barPosStep = -1;
        } else {
            barPosStep = 1;
        }
        catapultBall.barPos += barPosStep * difficulty * 0.2;

        if (catapultBaseScore > 1) {
            catapultBaseScore -= 0.5;
        }

        Vector barBase, tempVec;
        vectorSet(&barBase, catapultBall.bar->pos.x, catapultBall.bar->pos.y);
        vectorSet(&tempVec, barBase.x, barBase.y);

        float angle1;
        if (ip) {
            angle1 = -M_PI / 4;
        } else {
            angle1 = (-M_PI / 4) * 3;
        }
        addWithAngle(&tempVec, angle1, 4);
        float angle2;
        if (ip) {
            angle2 = -(M_PI / 4) * 3;
        } else {
            angle2 = (M_PI / 4) * 3;
        }
        addWithAngle(&tempVec, angle2, catapultBall.barPos);

        vectorSet(&catapultBall.pos, tempVec.x, tempVec.y);

        if (fabs(catapultBall.barPos) > catapultBall.bar->width * 0.65) {
            catapultBall.bar = NULL;
            vectorSet(&catapultBall.vel, 0, 0);
        } else if (catapultBall.barPos > 0) {
            if (catapultBall.barPos > catapultBall.bar->width * 0.45) {
                color = LIGHT_RED;
            } else {
                color = LIGHT_BLACK;
            }

            Vector predPos, predVel;
            vectorSet(&predPos, catapultBall.bar->pos.x, catapultBall.bar->pos.y);
            addWithAngle(&predPos, -M_PI / 4, 4);
            addWithAngle(&predPos, (-M_PI / 4) * 3, catapultBall.barPos);

            vectorSet(&predVel, 0, 0);
            addWithAngle(&predVel, -M_PI / 4, sqrt(catapultBall.barPos) * sqrt(difficulty));

            vectorAdd(&predPos, predVel.x, predVel.y);

            for (int i = 0; i < 99; i++) {
                vectorAdd(&predPos, predVel.x, predVel.y);
                predVel.y += catapultBall.grv;

                if (i % 5 == 0) {
                    box(predPos.x, predPos.y, 3, 3, &scratch);
                }

                if (predPos.y > 99) {
                    break;
                }
            }

            color = BLACK;

            if (input.isJustPressed) {
                play(JUMP);
                vectorSet(&catapultBall.vel, 0, 0);
                addWithAngle(&catapultBall.vel, -M_PI / 4, sqrt(catapultBall.barPos) * sqrt(difficulty));
                vectorAdd(&catapultBall.pos, catapultBall.vel.x, catapultBall.vel.y);
                catapultBall.bar = NULL;
                catapultPassedCount = 0;
            }
        }
    } else {
        vectorAdd(&catapultBall.pos, catapultBall.vel.x, catapultBall.vel.y);
        catapultBall.vel.y += catapultBall.grv;
        catapultBaseScore += catapultBall.vel.x * 0.1;
    }

    if (input.isJustPressed) {
        play(LASER);
    }

    box(catapultBall.pos.x, catapultBall.pos.y, 5, 5, &scratch);

    if (catapultBall.pos.y > 99 || catapultBall.pos.x < 0) {
        play(EXPLOSION);
        gameOver();
    }

    FOR_EACH(catapultBars, i) {
        ASSIGN_ARRAY_ITEM(catapultBars, i, CatapultBar, b);
        SKIP_IS_NOT_ALIVE(b);

        b->pos.x -= scr;

        barCenterPosRatio = 0.5;
        thickness = 3;
        float barAngle;
        if (ip) {
            barAngle = M_PI / 4;
        } else {
            barAngle = -M_PI / 4;
        }
        Collision barCollision;
        bar(b->pos.x, b->pos.y, b->width, barAngle, &barCollision);

        if (barCollision.isColliding.rect[BLACK] && catapultBall.bar == NULL) {
            play(POWER_UP);

            addScore(ceil(catapultBaseScore), catapultBall.pos.x, catapultBall.pos.y);

            Vector p;
            vectorSet(&p, b->pos.x, b->pos.y);
            float pAngle;
            if (ip) {
                pAngle = -M_PI / 4;
            } else {
                pAngle = (-M_PI / 4) * 3;
            }
            addWithAngle(&p, pAngle, 4);

            catapultBall.bar = b;
            float dirSign;
            if (catapultBall.pos.x > p.x) {
                dirSign = -1;
            } else {
                dirSign = 1;
            }
            catapultBall.barPos = clamp(
                distanceTo(&p, catapultBall.pos.x, catapultBall.pos.y) * dirSign,
                -b->width * 0.4,
                b->width * 0.4
            );
            catapultBall.grv = 0.1 * difficulty;
            b->isPassed = true;
        }

        if (!b->isPassed && b->pos.x + b->width * 0.5 < catapultBall.pos.x) {
            play(COIN);
            b->isPassed = true;
            catapultPassedCount++;
            catapultBaseScore += catapultPassedCount * 10;
            addScore(catapultPassedCount * 10, b->pos.x, b->pos.y);
        }

        b->isAlive = b->pos.x >= -b->width / 2;
    }

    catapultNextBarDist -= scr;
    if (catapultNextBarDist < 0) {
        int initialIndex = catapultNextBarIndex;
        while (catapultBars[catapultNextBarIndex].isAlive) {
            catapultNextBarIndex = cgl_wrap(catapultNextBarIndex + 1, 0, MAX_BARS);

            if (catapultNextBarIndex == initialIndex) return;
        }

        CatapultBar* newBar = &catapultBars[catapultNextBarIndex];
        newBar->width = rnd(20, 30);
        vectorSet(&newBar->pos, 200 + newBar->width / 2, rnd(50, 90));
        newBar->isPassed = false;
        newBar->isAlive = true;

        catapultNextBarDist += newBar->width / 2 + rnd(30, 50);

        catapultNextBarIndex = cgl_wrap(catapultNextBarIndex + 1, 0, MAX_BARS);
    }

    text(intToChar(ceil(catapultBaseScore)), 3, 9, &scratch);
}

void addGameCatapult() {
    addGame(catapultTitle, catapultDescription, catapultCharacters, catapultCharactersCount,
            &catapultOptions, false, &catapultUpdate);
}
