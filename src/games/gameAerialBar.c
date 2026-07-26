#include "../cglp.h"

char* aerialbarTitle = "AERIAL BAR";
char* aerialbarDescription = "[Tap]  Jump\n[Hold] Fly";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] aerialbarCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int aerialbarCharactersCount = 1;

Options aerialbarOptions = {100, 100, 9000, false};

struct AerialbarBar {
    Vector pos;
    float length;
    float angle;
    float angleVel;
    bool isHeld;
    bool isAlive;
};

struct AerialbarPlayer {
    Vector pos;
    float length;
    float angle;
    float angleVel;
    float center;
    AerialbarBar* bar;
    Vector vel;
};

#define AERIAL_BAR_MAX_COUNT 32
AerialbarBar[AERIAL_BAR_MAX_COUNT] aerialbarBars;
int aerialbarBarIndex;
float aerialbarNextBarDist;
AerialbarPlayer aerialbarPlayer;
float aerialbarFlyingTicks;
float aerialbarCeilingY;
float aerialbarTargetCeilingY;

void aerialbarAddBar(float x, float length, float angle, float angleVel, bool isHeld) {
    ASSIGN_ARRAY_ITEM(aerialbarBars, aerialbarBarIndex, AerialbarBar, b);
    vectorSet(&b->pos, x, 0);
    b->length = length;
    b->angle = angle;
    b->angleVel = angleVel;
    b->isHeld = isHeld;
    b->isAlive = true;
    aerialbarBarIndex = cgl_wrap(aerialbarBarIndex + 1, 0, AERIAL_BAR_MAX_COUNT);
}

void aerialbarUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(aerialbarBars);
        aerialbarBarIndex = 0;
        aerialbarAddBar(50, 50, M_PI / 2, 0.03, true);
        vectorSet(&aerialbarPlayer.pos, 0, 0);
        vectorSet(&aerialbarPlayer.vel, 0, 0);
        aerialbarPlayer.length = 10;
        aerialbarPlayer.angle = 0;
        aerialbarPlayer.angleVel = 0;
        aerialbarPlayer.center = 0.2;
        aerialbarPlayer.bar = &aerialbarBars[0];
        aerialbarFlyingTicks = 0;
        aerialbarCeilingY = aerialbarTargetCeilingY = 10;
    }

    aerialbarCeilingY += (aerialbarTargetCeilingY - aerialbarCeilingY) * 0.1;
    color = LIGHT_CYAN;
    rect(0, 0, 100, aerialbarCeilingY, &scratch);
    color = LIGHT_BLUE;
    rect(0, 90, 100, 10, &scratch);

    float scr = difficulty * 0.02;
    if (aerialbarPlayer.pos.x > 50) {
        scr += (aerialbarPlayer.pos.x - 50) * 0.1;
    }
    aerialbarPlayer.pos.x -= scr;

    if (aerialbarPlayer.bar != NULL) {
        AerialbarBar* b = aerialbarPlayer.bar;
        Vector endPos;
        vectorSet(&endPos, b->pos.x, aerialbarCeilingY);
        addWithAngle(&endPos, b->angle, b->length);
        vectorSet(&aerialbarPlayer.pos, endPos.x, endPos.y);
        aerialbarPlayer.angleVel += b->angleVel * b->length * 0.003;

        if (b->pos.x < 0) {
            color = RED;
            text("X", 3, aerialbarCeilingY, &scratch);
            play(EXPLOSION);
            gameOver();
            return;
        }

        if (input.isJustPressed) {
            play(SELECT);
            vectorSet(&aerialbarPlayer.vel, 0, 0);
            addWithAngle(&aerialbarPlayer.vel, b->angle + M_PI / 2.0,
                (b->angleVel * b->length + aerialbarPlayer.angleVel * 3) * sqrt(difficulty));
            vectorAdd(&aerialbarPlayer.vel, 0, -sqrt(difficulty) * 0.5);
            aerialbarPlayer.bar = NULL;
            aerialbarFlyingTicks = 1;
        }
    } else {
        aerialbarFlyingTicks += difficulty;
        vectorAdd(&aerialbarPlayer.pos, aerialbarPlayer.vel.x, aerialbarPlayer.vel.y);
        float vyStep;
        if (input.isPressed) {
            vyStep = 0.01;
        } else {
            vyStep = 0.1;
        }
        aerialbarPlayer.vel.y += vyStep * sqrt(difficulty);
        float velDamp;
        if (input.isPressed) {
            velDamp = 0.99;
        } else {
            velDamp = 0.95;
        }
        vectorMul(&aerialbarPlayer.vel, velDamp);

        if (aerialbarPlayer.pos.y > 89) {
            play(HIT);
            aerialbarPlayer.vel.y *= -1.5;
            aerialbarPlayer.pos.y = 88;
            aerialbarTargetCeilingY += 10;
            aerialbarFlyingTicks = -9999;
            FOR_EACH(aerialbarBars, i) {
                ASSIGN_ARRAY_ITEM(aerialbarBars, i, AerialbarBar, b);
                SKIP_IS_NOT_ALIVE(b);
                b->isHeld = false;
            }
        }

        if (aerialbarPlayer.pos.x < 0) {
            color = RED;
            text("X", 3, aerialbarPlayer.pos.y, &scratch);
            play(EXPLOSION);
            gameOver();
            return;
        }
    }

    aerialbarPlayer.angleVel *= 0.99;
    aerialbarPlayer.angle += aerialbarPlayer.angleVel;
    color = CYAN;

    float oldThickness = thickness;
    thickness = 4;
    Collision c;
    bar(aerialbarPlayer.pos.x, aerialbarPlayer.pos.y, aerialbarPlayer.length, aerialbarPlayer.angle, &c);
    thickness = oldThickness;
    if (c.isColliding.rect[LIGHT_CYAN] && aerialbarPlayer.vel.y < 0) {
        aerialbarPlayer.vel.y *= -0.5;
    }

    aerialbarNextBarDist -= scr;
    while (aerialbarNextBarDist < 0) {
        float length = rnd(20, 50);
        aerialbarAddBar(110, length, M_PI / 2.0 - rnd(0, M_PI / 4),
          rnd(0.02, 0.04) * RNDPM(), false);
        aerialbarNextBarDist = length + rnd(0, 20);
    }

    FOR_EACH(aerialbarBars, i) {
        ASSIGN_ARRAY_ITEM(aerialbarBars, i, AerialbarBar, b);
        SKIP_IS_NOT_ALIVE(b);
        b->pos.x -= scr;
        b->angleVel += cos(b->angle) * b->length * 0.00005 * sqrt(difficulty);
        b->angle += b->angleVel;

        color = BLACK;
        Vector endPos;
        vectorSet(&endPos, b->pos.x, aerialbarCeilingY);
        addWithAngle(&endPos, b->angle, b->length);
        line(b->pos.x, aerialbarCeilingY, endPos.x, endPos.y, &scratch);

        if (b->isHeld) {
            color = BLACK;
        } else {
            color = BLUE;
        }
        Collision c2;
        box(endPos.x, endPos.y, 5, 5, &c2);

        if (c2.isColliding.rect[LIGHT_BLUE]) {
            play(EXPLOSION);
            color = RED;
            text("X", endPos.x, endPos.y, &scratch);
            gameOver();
            return;
        }

        if (!b->isHeld && aerialbarPlayer.bar == NULL && c2.isColliding.rect[CYAN]) {
            play(POWER_UP);
            if (aerialbarFlyingTicks > 0) {
                addScore(ceil(aerialbarFlyingTicks), endPos.x, endPos.y);
            }
            aerialbarPlayer.bar = b;
            b->isHeld = true;
            aerialbarTargetCeilingY = clamp(aerialbarTargetCeilingY - 5, 10, 99);
        }

        b->isAlive = b->pos.x >= -30;
    }
}

void addGameAerialBar() {
    addGame(aerialbarTitle, aerialbarDescription, aerialbarCharacters,
            aerialbarCharactersCount, &aerialbarOptions, false, &aerialbarUpdate);
}
