#include "../cglp.h"

char* antlionTitle = "ANT LION";
char* antlionDescription = "[Hold] Walk";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] antlionCharacters = {
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    }
};
int antlionCharactersCount = 1;

Options antlionOptions = {100, 100, 2, false};

struct AntlionBar;

struct AntlionAnt {
    float angle;
    float av;      // Angular velocity
    float dist;
    AntlionBar* bar;  // Pointer to current bar being grabbed
    float barLength;
    float ticks;
};

struct AntlionBar {
    float angle;
    float av;      // Angular velocity
    float length;
    float dist;
    float speed;
    bool isAlive;
};

#define MAX_BAR_COUNT 50
AntlionBar[MAX_BAR_COUNT] antlionBars;
int antlionBarIndex;
float antlionNextBarTicks;
float antlionSandTicks;
int antlionMultiplier;
AntlionAnt antlionAnt;
Vector antlionCp;  // Center point

void antlionUpdate() {
    Collision scratch;
    barCenterPosRatio = 0.0;  // Important for bar positioning

    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(antlionBars);
        antlionBarIndex = 0;
        antlionNextBarTicks = 0;
        antlionSandTicks = 0;
        antlionMultiplier = 0;

        // Initialize ant
        antlionAnt.angle = M_PI / 2;
        antlionAnt.av = 0;
        antlionAnt.dist = 40;
        antlionAnt.bar = NULL;
        antlionAnt.barLength = 0;
        antlionAnt.ticks = 0;

        // Set center point
        vectorSet(&antlionCp, 50, 50);
    }

    // Draw sand circle
    color = LIGHT_YELLOW;
    antlionSandTicks += difficulty;
    thickness = 5;
    arc(50, 50, cgl_wrap(70 - ((int)(antlionSandTicks / 3) % 70), 0, 70), 0, M_PI * 2, &scratch);

    // Draw center arrows
    thickness = 1;
    color = RED;
    text("<", 48.0 + sin(ticks * 0.1) * 2.0, 50, &scratch);
    text(">", 52.0 - sin(ticks * 0.1) * 2.0, 50, &scratch);

    color = BLACK;
    Vector ap;  // Ant position
    vectorSet(&ap, antlionCp.x, antlionCp.y);
    Collision c;

    if (antlionAnt.bar == NULL) {
        if (input.isPressed) {
            play(HIT);
            antlionAnt.ticks += difficulty;
        }

        // Move ant
        addWithAngle(&ap, antlionAnt.angle, antlionAnt.dist);
        float avTarget;
        if (input.isPressed) {
            avTarget = 1;
        } else {
            avTarget = -1;
        }
        antlionAnt.av += (avTarget * 0.02 * difficulty - antlionAnt.av) * 0.1;
        antlionAnt.angle += antlionAnt.av;
        float distFactor;
        if (ap.x >= 0 && ap.x <= 99 && ap.y >= 0 && ap.y <= 99) {
            distFactor = 1;
        } else {
            distFactor = 2;
        }
        antlionAnt.dist -= 0.05 * difficulty * distFactor;

        // Draw ant body
        thickness = 3;
        bar(ap.x, ap.y, 3, antlionAnt.angle + M_PI / 2, &c);

        // Draw legs exactly like JS
        addWithAngle(&ap, antlionAnt.angle, 3);
        thickness = 2;
        barCenterPosRatio = 0.5 + sin(antlionAnt.ticks * 0.3) * 0.4;
        bar(ap.x, ap.y, 5, antlionAnt.angle + M_PI / 2, &scratch);
        addWithAngle(&ap, antlionAnt.angle, -6);
        barCenterPosRatio = 0.5 - sin(antlionAnt.ticks * 0.3) * 0.4;
        bar(ap.x, ap.y, 5, antlionAnt.angle + M_PI / 2, &scratch);
        barCenterPosRatio = 0.0;
    } else {
        play(LASER);
        antlionAnt.ticks += difficulty;
        antlionAnt.barLength += difficulty * 0.3;
        antlionAnt.bar->speed += difficulty * 0.002;

        antlionAnt.angle = antlionAnt.bar->angle;
        antlionAnt.dist = antlionAnt.bar->dist + antlionAnt.barLength;

        addWithAngle(&ap, antlionAnt.angle, antlionAnt.dist);
        thickness = 4;
        bar(ap.x, ap.y, 3, antlionAnt.angle, &c);

        addWithAngle(&ap, antlionAnt.angle + M_PI / 2, 3);
        thickness = 2;
        barCenterPosRatio = 0.5 + sin(antlionAnt.ticks * 0.3) * 0.4;
        bar(ap.x, ap.y, 5, antlionAnt.angle, &scratch);
        addWithAngle(&ap, antlionAnt.angle + M_PI / 2, -6);
        barCenterPosRatio = 0.5 - sin(antlionAnt.ticks * 0.3) * 0.4;
        bar(ap.x, ap.y, 5, antlionAnt.angle, &scratch);
        barCenterPosRatio = 0.0;

        if (antlionAnt.barLength > antlionAnt.bar->length + 6) {
            antlionAnt.bar = NULL;
        }
    }

    if (c.isColliding.text['<'] || c.isColliding.text['>']) {
        play(RANDOM);  // Changed to match JS "lucky"
        gameOver();
        return;
    }

    antlionNextBarTicks--;
    if (antlionNextBarTicks < 0) {
        float length = rnd(10, 30);
        ASSIGN_ARRAY_ITEM(antlionBars, antlionBarIndex, AntlionBar, b);
        b->angle = rnd(0, M_PI * 2);
        b->av = (rnd(0.5, 1.0) * difficulty * -0.05) / length;
        b->length = length;
        b->dist = 75;
        b->speed = rnd(0.5, 1.0) * difficulty * 0.2;
        b->isAlive = true;
        antlionBarIndex = cgl_wrap(antlionBarIndex + 1, 0, MAX_BAR_COUNT);
        antlionNextBarTicks = rnd(150, 180) / difficulty;
    }

    // Update and draw bars
    color = BLUE;
    FOR_EACH(antlionBars, i) {
        ASSIGN_ARRAY_ITEM(antlionBars, i, AntlionBar, b);
        SKIP_IS_NOT_ALIVE(b);

        b->angle += b->av;
        Vector p;
        vectorSet(&p, antlionCp.x, antlionCp.y);
        addWithAngle(&p, b->angle, b->dist);

        float distSpeedFactor;
        if (p.x >= 0 && p.x <= 99 && p.y >= 0 && p.y <= 99) {
            distSpeedFactor = 1;
        } else {
            distSpeedFactor = 5;
        }
        b->dist -= b->speed * distSpeedFactor;

        thickness = 3;
        Collision barCol;
        bar(p.x, p.y, b->length, b->angle, &barCol);

        if (barCol.isColliding.rect[BLACK] && antlionAnt.bar == NULL) {
            play(COIN);
            antlionMultiplier += 2;
            addScore(antlionMultiplier, ap.x, ap.y);
            antlionAnt.bar = b;
            antlionAnt.barLength = antlionAnt.dist - b->dist;
        }

        if (b->dist < 0) {
            b->length += b->dist;
            b->dist = 0;
        }

        if (b->length < 0) {
            play(SELECT);
            if (antlionAnt.bar == b) {
                antlionAnt.bar = NULL;
            }
            if (antlionMultiplier > 1) {
                antlionMultiplier--;
            }
            b->isAlive = false;
        }
    }

    // Reset thickness before text
    thickness = 1;
    // Draw multiplier
    color = BLACK;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(antlionMultiplier));
    text(multText, 3, 9, &scratch);
}

void addGameAntLion() {
    addGame(antlionTitle, antlionDescription, antlionCharacters, antlionCharactersCount,
            &antlionOptions, false, &antlionUpdate);
}
