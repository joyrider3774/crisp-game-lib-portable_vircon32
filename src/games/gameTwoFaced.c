#include "../cglp.h"

char* twofacedTitle = "TWO FACED";
char* twofacedDescription = "[Tap]  Turn\n[Hold] Go forward";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] twofacedCharacters = {
    {
        " RRRR ",
        "Rr rrR",
        "R rrrR",
        "RrrrrR",
        "RrrrrR",
        " RRRR ",
    }
};
int twofacedCharactersCount = 1;

Options twofacedOptions = {200, 100, 9, false};

struct TwofacedHead {
    Vector pos;
    int side;
    float angle;
    float av;
    float speed;
    float baseSpeed;
};

struct TwofacedTail {
    Vector pos;
    int side;
    float angle;
    bool isAlive;
};

struct TwofacedItem {
    Vector pos;
    int side;
    float angle;
};

#define MAX_TAILS 128
TwofacedHead twofacedHead;
TwofacedTail[MAX_TAILS] twofacedTails;
int twofacedTailIndex;
int twofacedTailCount;
int twofacedActiveTailCount;
float twofacedLightTailCount;
TwofacedItem twofacedItem;
float twofacedMultiplier;

void twofacedGetItem(float x, float y);
void twofacedCheckSide(Vector* pos, float* angle, int* side, float* av, bool hasAv);

void twofacedUpdate() {
    Collision scratch;
    if (!ticks) {
        vectorSet(&twofacedHead.pos, 0, 20);
        twofacedHead.side = -1;
        twofacedHead.angle = -M_PI / 2;
        twofacedHead.av = 1;
        twofacedHead.speed = 1;
        twofacedHead.baseSpeed = 1;

        INIT_UNALIVED_ARRAY_FAST(twofacedTails);
        twofacedTailIndex = 0;
        twofacedTailCount = 3;
        twofacedActiveTailCount = 0;
        twofacedMultiplier = 3;
        twofacedLightTailCount = 2;

        vectorSet(&twofacedItem.pos, 0, 0);
        twofacedItem.side = -1;
        twofacedItem.angle = 0;
    }

    float sd = sqrt(difficulty);
    Vector v;
    vectorSet(&v, 0, 0);

    if (input.isJustPressed) {
        play(LASER);
        twofacedHead.av *= -1;
        twofacedHead.speed += 0.1;
    }

    if (input.isPressed) {
        play(HIT);
        twofacedHead.speed += 0.01;
    } else {
        twofacedHead.speed += (1 - twofacedHead.speed) * 0.1;
        twofacedHead.angle += twofacedHead.av * 0.03 * sd * twofacedHead.speed * twofacedHead.baseSpeed;
    }

    twofacedHead.baseSpeed *= 1.002;
    addWithAngle(&twofacedHead.pos, twofacedHead.angle, sd * 0.5 * twofacedHead.speed * twofacedHead.baseSpeed);
    twofacedCheckSide(&twofacedHead.pos, &twofacedHead.angle, &twofacedHead.side, &twofacedHead.av, true);

    // Add new tail segment
    if (twofacedActiveTailCount < MAX_TAILS) {
        ASSIGN_ARRAY_ITEM(twofacedTails, twofacedTailIndex, TwofacedTail, t);
        vectorSet(&t->pos, twofacedHead.pos.x, twofacedHead.pos.y);
        t->side = twofacedHead.side;
        t->angle = twofacedHead.angle;
        t->isAlive = true;
        twofacedTailIndex = cgl_wrap(twofacedTailIndex + 1, 0, MAX_TAILS);
        if (twofacedActiveTailCount < twofacedTailCount * 9) {
            twofacedActiveTailCount++;
        }
    }

    // Update item
    addWithAngle(&twofacedItem.pos, twofacedItem.angle, sd * 0.2);
    twofacedCheckSide(&twofacedItem.pos, &twofacedItem.angle, &twofacedItem.side, NULL, false);

    // Draw item in background layer
    color = BLACK;
    if (twofacedItem.side == -1) {
        character("a", twofacedItem.pos.x + 150, twofacedItem.pos.y + 50, &scratch);
    }
    if (twofacedItem.side == 1) {
        character("a", twofacedItem.pos.x + 50, twofacedItem.pos.y + 50, &scratch);
    }

    // Draw head
    color = GREEN;
    thickness = 4;
    if (twofacedHead.side == -1) {
        bar(twofacedHead.pos.x + 150, twofacedHead.pos.y + 50, 3, twofacedHead.angle, &scratch);
    }
    if (twofacedHead.side == 1) {
        bar(twofacedHead.pos.x + 50, twofacedHead.pos.y + 50, 3, twofacedHead.angle, &scratch);
    }

    // Draw boxes
    color = LIGHT_YELLOW;
    box(50, 50, 80, 80, &scratch);
    color = LIGHT_PURPLE;
    box(150, 50, 80, 80, &scratch);

    // Draw item in foreground layer
    color = BLACK;
    if (twofacedItem.side == -1) {
        character("a", twofacedItem.pos.x + 50, twofacedItem.pos.y + 50, &scratch);
    }
    if (twofacedItem.side == 1) {
        character("a", twofacedItem.pos.x + 150, twofacedItem.pos.y + 50, &scratch);
    }

    // Update and draw tails
    color = LIGHT_GREEN;
    twofacedLightTailCount += (2 - twofacedLightTailCount) * 0.03;

    for (int i = 0; i < twofacedTailCount; i++) {
        int ti = i * 9;
        if (ti >= twofacedActiveTailCount) continue;

        int accessIndex = (twofacedTailIndex - 1 - ti + MAX_TAILS) % MAX_TAILS;
        TwofacedTail* t = &twofacedTails[accessIndex];
        if (!t->isAlive) continue;

        if (i < twofacedLightTailCount) {
            color = LIGHT_BLACK;
        } else {
            color = LIGHT_GREEN;
        }
        thickness = 3;

        if (t->side == -1) {
            Collision tc1;
            bar(t->pos.x + 50, t->pos.y + 50, 4, t->angle, &tc1);
            if (twofacedItem.side == -1 && tc1.isColliding.character['a']) {
                twofacedGetItem(t->pos.x + 50, t->pos.y + 50);
            }
        }
        if (t->side == 1) {
            Collision tc2;
            bar(t->pos.x + 150, t->pos.y + 50, 4, t->angle, &tc2);
            if (twofacedItem.side == 1 && tc2.isColliding.character['a']) {
                twofacedGetItem(t->pos.x + 150, t->pos.y + 50);
            }
        }
    }

    // Draw and check head collisions
    color = GREEN;
    thickness = 4;
    if (twofacedHead.side == -1) {
        Collision hc1;
        bar(twofacedHead.pos.x + 50, twofacedHead.pos.y + 50, 3, twofacedHead.angle, &hc1);
        if (twofacedItem.side == -1 && hc1.isColliding.character['a']) {
            twofacedGetItem(twofacedHead.pos.x + 50, twofacedHead.pos.y + 50);
        }
        if (hc1.isColliding.rect[LIGHT_GREEN]) {
            play(EXPLOSION);
            gameOver();
            return;
        }
    }
    if (twofacedHead.side == 1) {
        Collision hc2;
        bar(twofacedHead.pos.x + 150, twofacedHead.pos.y + 50, 3, twofacedHead.angle, &hc2);
        if (twofacedItem.side == 1 && hc2.isColliding.character['a']) {
            twofacedGetItem(twofacedHead.pos.x + 150, twofacedHead.pos.y + 50);
        }
        if (hc2.isColliding.rect[LIGHT_GREEN]) {
            play(EXPLOSION);
            gameOver();
            return;
        }
    }

    // Update multiplier
    int pp = ceil(twofacedMultiplier);
    twofacedMultiplier -= 0.01;
    if (twofacedMultiplier < 1) {
        twofacedMultiplier = 1;
    }
    if (ceil(twofacedMultiplier) < pp) {
        play(COIN);
    }

    color = BLACK;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar((int)ceil(twofacedMultiplier)));
    text(multText, 3, 9, &scratch);
}

void twofacedGetItem(float x, float y) {
    play(POWER_UP);
    twofacedHead.baseSpeed = 1;
    addScore(ceil(twofacedMultiplier), x, y);
    if (twofacedTailCount < 25) {
        twofacedTailCount++;
    }
    twofacedMultiplier += twofacedTailCount;

    twofacedItem.pos.x = rnd(-35, 35);
    twofacedItem.pos.y = rnd(-35, 35);
    twofacedItem.angle = rnd(-M_PI, M_PI);
    twofacedItem.side = twofacedHead.side * -1;
}

void twofacedCheckSide(Vector* pos, float* angle, int* side, float* av, bool hasAv) {
    Vector v;
    vectorSet(&v, 0, 0);
    addWithAngle(&v, *angle, 1);

    if ((pos->x < -40 && v.x < 0) || (pos->x > 40 && v.x > 0)) {
        *side *= -1;
        v.x *= -1;
        if (hasAv && av != NULL) {
            *av *= -1;
            twofacedLightTailCount += 2;
        }
    }
    if ((pos->y < -40 && v.y < 0) || (pos->y > 40 && v.y > 0)) {
        *side *= -1;
        v.y *= -1;
        if (hasAv && av != NULL) {
            *av *= -1;
            twofacedLightTailCount += 2;
        }
    }

    if (twofacedLightTailCount >= twofacedTailCount - 1) {
        twofacedLightTailCount = twofacedTailCount - 1;
    }
    *angle = vectorAngle(&v);
}

void addGameTwofaced() {
    addGame(twofacedTitle, twofacedDescription, twofacedCharacters, twofacedCharactersCount,
            &twofacedOptions, false, &twofacedUpdate);
}
