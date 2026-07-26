#include "../cglp.h"

char* bwallsTitle = "B WALLS";
char* bwallsDescription = "[Tap] Shoot";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bwallsCharacters = {
    {   // Player 'a'
        "  ll  ",
        "  ll  ",
        "rrLLrr",
        "rrLLrr",
        "  ll  ",
        "      ",
    },
    {   // Enemy 'b'
        "l ll l",
        " lyyl ",
        "lyyyyl",
        " ylly ",
        " ylly ",
        "  ll  ",
    },
    {   // Shot 'c'
        " y    ",
        "yyy   ",
        "lll   ",
        "yyy   ",
        "lll   ",
        " y    ",
    }
};
int bwallsCharactersCount = 3;

Options bwallsOptions = {100, 100, 19, false};

struct BwallsWall {
    float y;
    float width;
    float interval;
    float ox;
    float vx;
    bool isAlive;
};

// Helper function to match JS modulo behavior
float bwallsJsModulo(float x, float m) {
    float result = fmod(x, m);
    if (result >= 0) {
        return result;
    }
    return result + m;
}

#define MAX_WALLS 80
BwallsWall[MAX_WALLS] bwallsWalls;
int bwallsWallIndex;
float bwallsNextWallDist;
float bwallsScr;
float bwallsWallTicks;
float bwallsShotY;
bool bwallsHasShot;
int bwallsMultiplier;

void bwallsUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(bwallsWalls);
        bwallsWallIndex = 0;
        bwallsNextWallDist = 0;
        bwallsScr = 0;
        bwallsWallTicks = 0;
        bwallsShotY = 0;
        bwallsHasShot = false;
        bwallsMultiplier = 1;
    }

    const float wallStopInterval = 120 / sqrt(difficulty);
    const bool isWallStopping = bwallsJsModulo(bwallsWallTicks / wallStopInterval, 1.0) > 0.5;

    if (!bwallsHasShot && input.isJustPressed) {
        play(POWER_UP);
        bwallsShotY = 90;
        bwallsHasShot = true;
        bwallsMultiplier = 1;
    }

    if (bwallsHasShot) {
        bwallsShotY -= sqrt(difficulty) * 3;
        character("c", 50, bwallsShotY, &scratch);
        if (bwallsShotY > 66) {
            bwallsScr += (bwallsShotY - 66) * 0.1;
        }
    }

    if (!isWallStopping) {
        play(HIT);
    }

    float maxY = 0;
    FOR_EACH(bwallsWalls, i) {
        ASSIGN_ARRAY_ITEM(bwallsWalls, i, BwallsWall, w);
        SKIP_IS_NOT_ALIVE(w);

        w->y += bwallsScr;
        if (w->y > maxY) {
            maxY = w->y;
        }

        if (!isWallStopping) {
            w->ox = bwallsJsModulo(w->ox + w->vx, w->width + w->interval);
        }

        // Draw all wall segments
        float x = w->ox - w->width;
        color = YELLOW;
        while (x < 99) {
            Collision c;
            rect(x, w->y, w->width, 5, &c);
            if (bwallsHasShot && c.isColliding.character['c']) {
                play(SELECT);
                bwallsHasShot = false;
                bwallsShotY = 0;
            }
            x += w->width + w->interval;
        }

        color = BLACK;
        Collision c;
        character("b", 50, w->y - 3, &c);
        if (bwallsHasShot && c.isColliding.character['c']) {
            play(EXPLOSION);
            addScore(bwallsMultiplier, 50, w->y - 3);
            bwallsMultiplier *= 2;
            w->isAlive = false;
            continue;
        }
    }

    if (!bwallsHasShot) {
        bwallsWallTicks++;
    } else if (bwallsShotY < -9 || (!isWallStopping && bwallsShotY < maxY + 7)) {
        play(SELECT);
        bwallsHasShot = false;
        bwallsShotY = 0;
    }

    bwallsNextWallDist -= bwallsScr;
    if (bwallsNextWallDist < 0) {
        float w = rnd(10, 20);
        float in = rnd(20, 40);

        ASSIGN_ARRAY_ITEM(bwallsWalls, bwallsWallIndex, BwallsWall, wall);
        wall->y = -9 - bwallsNextWallDist;
        wall->width = w;
        wall->interval = in;
        wall->ox = rnd(0, w + in);
        wall->vx = 0;
        wall->isAlive = true;

        bool isValid = false;
        for (int attempt = 0; attempt < 99; attempt++) {
            float vx = rnd(5, 10) * RNDPM() * sqrt(difficulty); // rnds(5, 10)
            if (fabs(bwallsJsModulo(vx * wallStopInterval * 0.5, w + in)) > 19) {
                wall->vx = vx;
                isValid = true;
                break;
            }
        }
        if (!isValid) {
            wall->width = 0;
        }

        bwallsWallIndex = cgl_wrap(bwallsWallIndex + 1, 0, MAX_WALLS);
        bwallsNextWallDist += 11;
    }

    if (maxY < 40) {
        bwallsScr += (40 - maxY) * 0.01;
    } else {
        bwallsScr *= 0.5;
    }
    bwallsScr += difficulty * 0.01;

    color = BLACK;
    character("a", 50, 90, &scratch);
    if (scratch.isColliding.rect[YELLOW]) {
        play(RANDOM);  // Equivalent to "lucky" in JS
        gameOver();
    }
}

void addGameBwalls() {
    addGame(bwallsTitle, bwallsDescription, bwallsCharacters, bwallsCharactersCount,
            &bwallsOptions, false, &bwallsUpdate);
}
