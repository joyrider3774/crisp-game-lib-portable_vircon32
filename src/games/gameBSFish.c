#include "../cglp.h"

char* bsfishTitle = "BS FISH";
char* bsfishDescription = "[Hold] Speed up";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bsfishCharacters = {
    {   // Fish 'a'
        "      ",
        "      ",
        "  rrr ",
        "rrbrrr",
        " rrr r",
        "      ",
    },
    {   // Bird frame 1 'b'
        "      ",
        " ll   ",
        "  ll  ",
        "llllly",
        "      ",
        "      ",
    },
    {   // Bird frame 2 'c'
        "      ",
        "      ",
        "llllly",
        "  ll  ",
        " ll   ",
        "      ",
    }
};
int bsfishCharactersCount = 3;

Options bsfishOptions = {200, 100, 4, false};

struct BsfishBird {
    Vector pos;
    float vx;
    float ticks;
};

enum BsfishFishType {
    BSFISH_NORMAL,
    BSFISH_EYE,
    BSFISH_BIG,
    BSFISH_FAKE
};

struct BsfishFish {
    Vector pos;
    Vector vel;
    BsfishFishType type;
    bool isAlive;
};

// Vircon32 port note: this used to return "Collision" (a ~270-word
// struct) BY VALUE - reworked to the out-pointer idiom like every other
// Collision-producing function in this port (see cglp.h's header
// comment). It also drew from *its own* file-scope "characters" table
// directly (not the engine's currentCharacters), so it references
// bsfishCharacters/bsfishCharactersCount below rather than going through
// the character()/currentCharacters machinery. Only used for the BIG/
// FAKE fish now, which are drawn scaled up (sc=6) - normal-sized fish
// (sc=1, no scaling needed) go through the standard character() instead,
// see bsfishUpdate() below.
void bsfishCharacterScaled(int* chr, float x, float y, float scaleX, float scaleY, Collision* result) {
    memset(result, 0, sizeof(Collision));

    int charIndex = chr[0] - 'a';
    if (charIndex < 0 || charIndex >= bsfishCharactersCount) {
        return;
    }

    float offsetX = (CHARACTER_WIDTH * scaleX) / 2.0;
    float offsetY = (CHARACTER_HEIGHT * scaleY) / 2.0;

    for (int gridY = 0; gridY < CHARACTER_HEIGHT; gridY++) {
        for (int gridX = 0; gridX < CHARACTER_WIDTH; gridX++) {
            int pixel = bsfishCharacters[charIndex][gridY][gridX];

            if (pixel == ' ') continue;

            int prevColor = color;

            if (color == BLACK) {
                if (pixel == 'w') {
                    color = WHITE;
                } else if (pixel == 'r') {
                    color = RED;
                } else if (pixel == 'g') {
                    color = GREEN;
                } else if (pixel == 'y') {
                    color = YELLOW;
                } else if (pixel == 'b') {
                    color = BLUE;
                } else if (pixel == 'p') {
                    color = PURPLE;
                } else if (pixel == 'c') {
                    color = CYAN;
                } else if (pixel == 'l') {
                    color = BLACK;
                } else if (pixel == 'R') {
                    color = LIGHT_RED;
                } else if (pixel == 'G') {
                    color = LIGHT_GREEN;
                } else if (pixel == 'Y') {
                    color = LIGHT_YELLOW;
                } else if (pixel == 'B') {
                    color = LIGHT_BLUE;
                } else if (pixel == 'P') {
                    color = LIGHT_PURPLE;
                } else if (pixel == 'C') {
                    color = LIGHT_CYAN;
                } else if (pixel == 'L') {
                    color = LIGHT_BLACK;
                } else {
                    color = BLACK;
                }
            }
            // else: use the current color value

            float pixelX = x + (gridX * scaleX) - offsetX;
            float pixelY = y + (gridY * scaleY) - offsetY;

            Collision pixelCollision;
            box(pixelX, pixelY, scaleX, scaleY, &pixelCollision);

            for (int i = 0; i < COLOR_COUNT; i++) {
                result->isColliding.rect[i] |= pixelCollision.isColliding.rect[i];
            }
            for (int i = 0; i < ASCII_CHARACTER_COUNT; i++) {
                result->isColliding.text[i] |= pixelCollision.isColliding.text[i];
                result->isColliding.character[i] |= pixelCollision.isColliding.character[i];
            }

            color = prevColor;
        }
    }
}

#define MAX_FISHES 100
BsfishBird bsfishBird;
BsfishFish[MAX_FISHES] bsfishFishes;
int bsfishFishIndex;
float bsfishNextFishTicks;
int bsfishNextBigFishCount;
float bsfishScrX;
int bsfishMultiplier;
int bsfishIsGameOver;

void bsfishUpdate() {
    Collision scratch;
    if (!ticks) {
        vectorSet(&bsfishBird.pos, 20, 20);
        bsfishBird.vx = 1;
        bsfishBird.ticks = 0;
        bsfishIsGameOver = 0;

        INIT_UNALIVED_ARRAY_FAST(bsfishFishes);
        bsfishFishIndex = 0;
        bsfishNextFishTicks = 0;
        bsfishNextBigFishCount = 3;
        bsfishScrX = 0;
        bsfishMultiplier = 1;
    }

    if (input.isJustPressed) {
        play(SELECT);
    }

    float scr;
    if (bsfishBird.pos.x > 30) {
        scr = (bsfishBird.pos.x - 30) * 0.1 * sqrt(difficulty);
    } else {
        scr = 0;
    }
    float vxTarget;
    if (input.isPressed) {
        vxTarget = 3 * sqrt(difficulty);
    } else {
        vxTarget = 0.1;
    }
    bsfishBird.vx += (vxTarget - bsfishBird.vx) * 0.2;
    bsfishBird.ticks += bsfishBird.vx;
    bsfishBird.pos.x += bsfishBird.vx - scr;

    color = BLACK;
    int[2] birdChar;
    birdChar[0] = 'b' + ((int)floor(bsfishBird.ticks / 10.0)) % 2;
    birdChar[1] = 0;
    character(birdChar, bsfishBird.pos.x, bsfishBird.pos.y, &scratch);

    bsfishNextFishTicks--;
    if (bsfishNextFishTicks < 0) {
        Vector pos;
        pos.x = rnd(130, 220);
        pos.y = 120;
        Vector vel;
        vel.x = -rnd(1, 1.5) * 0.5 * sqrt(difficulty);
        vel.y = -2.5 * sqrt(difficulty);
        BsfishFishType type = BSFISH_NORMAL;
        bsfishNextBigFishCount--;

        if (bsfishNextBigFishCount < 0) {
            if (rnd(0, 1) < 0.5) {
                type = BSFISH_BIG;
            } else {
                type = BSFISH_FAKE;
            }
            bsfishNextBigFishCount = rnd(3, 9);
        }

        if (type == BSFISH_BIG) {
            if (rnd(0, 1) < 0.7) {
                vel.y *= 1.125;
                vel.x *= 0.9;
            } else {
                vel.y *= 0.97;
                vel.x *= 1.5;
            }
        }

        ASSIGN_ARRAY_ITEM(bsfishFishes, bsfishFishIndex, BsfishFish, f);
        f->pos = pos;
        f->vel = vel;
        f->type = type;
        f->isAlive = true;
        bsfishFishIndex = cgl_wrap(bsfishFishIndex + 1, 0, MAX_FISHES);
        bsfishNextFishTicks = rnd(40, 60) / difficulty;
    }

    FOR_EACH(bsfishFishes, i) {
        ASSIGN_ARRAY_ITEM(bsfishFishes, i, BsfishFish, f);
        SKIP_IS_NOT_ALIVE(f);

        Vector pp = f->pos;
        f->vel.y += 0.03 * difficulty;
        vectorAdd(&f->pos, f->vel.x, f->vel.y);
        f->pos.x -= scr;

        color = BLACK;
        float sc;
        if (f->type == BSFISH_BIG || f->type == BSFISH_FAKE) {
            sc = 6;
        } else {
            sc = 1;
        }
        float wy = 50 + 2 * sc;

        if (f->pos.y > wy) {
            color = BLUE;
        } else {
            if (f->type == BSFISH_FAKE) {
                int eyeX = 2;
                int eyeY = 1;
                int[13][2] points = {
                                  {2, 0}, {3, 0}, {4, 0},
                    {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1},
                            {1, 2}, {2, 2}, {3, 2},         {5, 2}
                };

                for (int p = 0; p < 13; p++) {
                    ASSIGN_ARRAY_ITEM(bsfishFishes, bsfishFishIndex, BsfishFish, newFish);
                    vectorSet(&newFish->pos,
                        f->pos.x + points[p][0] * 6 - 15,
                        f->pos.y + points[p][1] * 6 - 6);
                    newFish->vel = f->vel;
                    if (points[p][0] == eyeX && points[p][1] == eyeY) {
                        newFish->type = BSFISH_EYE;
                    } else {
                        newFish->type = BSFISH_NORMAL;
                    }
                    newFish->isAlive = true;
                    bsfishFishIndex = cgl_wrap(bsfishFishIndex + 1, 0, MAX_FISHES);
                }
                f->isAlive = false;
                continue;
            }

            if (f->type == BSFISH_EYE) {
                color = BLUE;
            } else {
                color = BLACK;
            }
        }

        Collision c;
        if (sc == 1) {
            // Normal-sized (NORMAL/EYE) fish don't need any scaling at
            // all, so draw them through the standard character()/
            // collision path instead of the manual pixel-by-pixel scaled
            // draw below - same visual result (bsfishCharacterScaled's
            // "use the pattern's own per-pixel colors when color==BLACK,
            // else override everything with the current color" logic is
            // exactly what the engine's own setColorGrid() already does
            // for character()), but this way gets the character-pattern
            // cache, the GPU-state-change skipping, and the merged-rect
            // drawing that bsfishCharacterScaled bypasses entirely.
            character("a", f->pos.x, f->pos.y, &c);
        } else {
            bsfishCharacterScaled("a", f->pos.x, f->pos.y, sc, sc, &c);
        }
        if ((c.isColliding.character['b'] || c.isColliding.character['c']) && (f->type == BSFISH_BIG)) {
            bsfishIsGameOver = true;
        }

        if (f->type != BSFISH_BIG && distanceTo(&f->pos, bsfishBird.pos.x, bsfishBird.pos.y) < 6) {
            addScore(bsfishMultiplier, f->pos.x, f->pos.y);
            if (f->type == BSFISH_NORMAL) {
                play(COIN);
                bsfishMultiplier++;
            } else {
                play(POWER_UP);
                bsfishMultiplier += 10;
            }
            f->isAlive = false;
            continue;
        }

        if (f->pos.x < -18 || f->pos.y > 120) {
            if (f->type != BSFISH_BIG && bsfishMultiplier > 1) {
                bsfishMultiplier--;
            }
            f->isAlive = false;
            continue;
        }

        if ((pp.y - wy) * (f->pos.y - wy) < 0) {
            play(HIT);
        }
    }

    if (bsfishIsGameOver) {
        play(EXPLOSION);
        gameOver();
    }

    bsfishScrX = cgl_wrap(bsfishScrX - scr, -10, 210);
    color = BLUE;
    rect(0, 50, 200, 2, &scratch);
    color = CYAN;

    for (int i = 0; i < 5; i++) {
        rect(cgl_wrap(bsfishScrX + (220.0 / 5) * i, -10, 210), 50, 9, 2, &scratch);
    }

    color = BLACK;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(bsfishMultiplier));
    text(multText, 3, 9, &scratch);
}

void addGameBsfish() {
    addGame(bsfishTitle, bsfishDescription, bsfishCharacters, bsfishCharactersCount,
            &bsfishOptions, false, &bsfishUpdate);
}
