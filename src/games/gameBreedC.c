#include "../cglp.h"

char* breedcTitle = "BREED C";
char* breedcDescription = "[Tap]\n Erase blocks\n (4 or more\n  linked)";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] breedcCharacters = {
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    }
};
int breedcCharactersCount = 1;

Options breedcOptions = {100, 100, 1, false};

#define GRID_SIZE 15
// Vircon32 port note: renamed from the original's own "COLOR_COUNT" macro,
// which collided with cglp.h's engine-wide COLOR_COUNT (=15) macro of the
// same name but a different value.
#define BREEDC_COLOR_COUNT 4

int[GRID_SIZE][GRID_SIZE] breedcGrid;
int[GRID_SIZE][GRID_SIZE] breedcPrevGrid;
bool[GRID_SIZE][GRID_SIZE] breedcErasingGrid;
float breedcErasingTicks;
int breedcErasingCount;
float breedcNextGridTicks;
int breedcNextGridCount;
int breedcMultiplier;

// Forward declarations of helper functions
void breedcAddGrid();
void breedcAddHorizontal();
void breedcAddVertical();
int breedcDownHorizontal();
int breedcDownVertical();
int breedcCheckErasingDown();
int breedcCheckErasingUp();
int breedcDrawGrid();

void breedcUpdate() {
    Collision scratch;
    // Never reads a Collision result anywhere in this file - the tapped
    // grid cell is computed directly from input.pos via grid math (see
    // "ip.x"/"ip.y" below), so the engine's own O(n^2) hitbox scan (see
    // checkHitBox() in cglp.c) is pure waste here. Restored automatically
    // when the next real game starts, via resetDrawState() in
    // initInGame().
    hasCollision = false;
    int cgp = floor((float)GRID_SIZE / 2.0);

    if (!ticks) {
        memset(breedcGrid, -1, sizeof(breedcGrid));
        memset(breedcPrevGrid, -1, sizeof(breedcPrevGrid));
        memset(breedcErasingGrid, false, sizeof(breedcErasingGrid));
        breedcNextGridTicks = 0;
        breedcNextGridCount = 0;
        breedcErasingTicks = 0;
        breedcMultiplier = 1;
    }

    Vector ip;
    ip.x = floor((input.pos.x - 50) / 6 + (float)GRID_SIZE / 2);
    ip.y = floor((input.pos.y - 53) / 6 + (float)GRID_SIZE / 2);

    if (ip.x >= 0 && ip.x < GRID_SIZE && ip.y >= 0 && ip.y < GRID_SIZE) {
        color = LIGHT_BLACK;
        box((ip.x - (float)GRID_SIZE / 2) * 6 + 53, (ip.y - (float)GRID_SIZE / 2) * 6 + 56, 7, 7, &scratch);
    }

    breedcErasingTicks--;
    if (breedcErasingTicks >= 0) {
        if (breedcErasingTicks == 0) {
            for (int i = 0; i < 99; i++) {
                int dc;
                if (breedcErasingCount % 2 == 0) {
                    dc = breedcDownHorizontal() + breedcDownVertical();
                } else {
                    dc = breedcDownVertical() + breedcDownHorizontal();
                }
                if (dc == 0) {
                    break;
                }
            }
            breedcNextGridTicks = 120 / sqrt(difficulty);
            breedcErasingCount++;
        }
        breedcDrawGrid();
        return;
    }

    if (breedcGrid[cgp][cgp] < 0) {
        breedcGrid[cgp][cgp] = rnd(0, BREEDC_COLOR_COUNT);
    }

    if (input.isJustPressed) {
        if (ip.x >= 0 && ip.x < GRID_SIZE &&
            ip.y >= 0 && ip.y < GRID_SIZE &&
            breedcGrid[(int)ip.x][(int)ip.y] >= 0) {

            memset(breedcErasingGrid, false, sizeof(breedcErasingGrid));

            breedcErasingGrid[(int)ip.x][(int)ip.y] = true;
            int tec = 1;

            for (int i = 0; i < 99; i++) {
                int ec = breedcCheckErasingDown() + breedcCheckErasingUp();
                tec += ec;
                if (ec == 0) {
                    break;
                }
            }

            if (tec < 4) {
                play(HIT);
                addScore(-tec, input.pos.x, input.pos.y);
                breedcAddGrid();
                breedcAddGrid();
            } else {
                play(POWER_UP);
                addScore(tec * breedcMultiplier, input.pos.x, input.pos.y);
                breedcMultiplier++;

                for (int x = 0; x < GRID_SIZE; x++) {
                    for (int y = 0; y < GRID_SIZE; y++) {
                        if (breedcErasingGrid[x][y]) {
                            breedcGrid[x][y] = -1;
                        }
                    }
                }
                breedcErasingTicks = ceil(60 / sqrt(difficulty));
                breedcDrawGrid();
                return;
            }
        } else {
            breedcAddGrid();
            breedcAddGrid();
        }
    }

    breedcNextGridTicks--;
    if (breedcNextGridTicks < 0) {
        breedcAddGrid();
    }

    if (breedcDrawGrid() >= GRID_SIZE * GRID_SIZE) {
        play(EXPLOSION);
        gameOver();
    }
}

int breedcDrawGrid() {
    Collision scratch;
    color = BLACK;
    int[16] multText;
    strcpy(multText, "x");
    strcat(multText, intToChar(breedcMultiplier));
    text(multText, 3, 9, &scratch);

    int gc = 0;
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            int c = breedcGrid[x][y];
            if (c >= 0) {
                if (c == 0) {
                    color = RED;
                } else if (c == 1) {
                    color = GREEN;
                } else if (c == 2) {
                    color = BLUE;
                } else if (c == 3) {
                    color = YELLOW;
                }
                box(53 + (x - ((float)GRID_SIZE / 2.0)) * 6, 56 + (y - ((float)GRID_SIZE / 2.0)) * 6, 5, 5, &scratch);
                gc++;
            }
        }
    }
    return gc;
}

void breedcAddGrid() {
    play(COIN);
    breedcMultiplier = 1;

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            breedcPrevGrid[x][y] = breedcGrid[x][y];
        }
    }

    if (breedcNextGridCount % 2 == 0) {
        breedcAddHorizontal();
    } else {
        breedcAddVertical();
    }
    breedcNextGridCount++;
    breedcNextGridTicks = 120 / sqrt(difficulty);
}

void breedcAddHorizontal() {
    int cx = floor((float)GRID_SIZE / 2.0);

    for (int x = 0; x < cx - 1; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            breedcGrid[x][y] = breedcPrevGrid[x + 1][y];
        }
    }

    for (int x = GRID_SIZE - 1; x > cx + 1; x--) {
        for (int y = 0; y < GRID_SIZE; y++) {
            breedcGrid[x][y] = breedcPrevGrid[x - 1][y];
        }
    }

    for (int y = 0; y < GRID_SIZE; y++) {
        int c = breedcPrevGrid[cx][y];
        if (c >= 0) {
            int nx;
            if (rnd(0, 1) < 0.5) {
                nx = -1;
            } else {
                nx = 1;
            }
            int nc = cgl_wrap(c + rnd(0, BREEDC_COLOR_COUNT - 1), 0, BREEDC_COLOR_COUNT);
            breedcGrid[cx + nx][y] = nc;
            breedcGrid[cx][y] = nc;
            breedcGrid[cx - nx][y] = c;
        }
    }
}

void breedcAddVertical() {
    int cy = floor((float)GRID_SIZE / 2.0);

    for (int y = 0; y < cy - 1; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            breedcGrid[x][y] = breedcPrevGrid[x][y + 1];
        }
    }

    for (int y = GRID_SIZE - 1; y > cy + 1; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            breedcGrid[x][y] = breedcPrevGrid[x][y - 1];
        }
    }

    for (int x = 0; x < GRID_SIZE; x++) {
        int c = breedcPrevGrid[x][cy];
        if (c >= 0) {
            int ny;
            if (rnd(0, 1) < 0.5) {
                ny = -1;
            } else {
                ny = 1;
            }
            int nc = cgl_wrap(c + rnd(0, BREEDC_COLOR_COUNT - 1), 0, BREEDC_COLOR_COUNT);
            breedcGrid[x][cy + ny] = nc;
            breedcGrid[x][cy] = nc;
            breedcGrid[x][cy - ny] = c;
        }
    }
}

int breedcDownHorizontal() {
    int dc = 0;
    int cx = floor((float)GRID_SIZE / 2.0);

    for (int x = cx; x >= 1; x--) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (breedcGrid[x][y] == -1 && breedcGrid[x - 1][y] >= 0) {
                breedcGrid[x][y] = breedcGrid[x - 1][y];
                breedcGrid[x - 1][y] = -1;
                dc++;
            }
        }
    }

    for (int x = cx; x <= GRID_SIZE - 2; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (breedcGrid[x][y] == -1 && breedcGrid[x + 1][y] >= 0) {
                breedcGrid[x][y] = breedcGrid[x + 1][y];
                breedcGrid[x + 1][y] = -1;
                dc++;
            }
        }
    }

    return dc;
}

int breedcDownVertical() {
    int dc = 0;
    int cy = floor((float)GRID_SIZE / 2.0);

    for (int y = cy; y >= 1; y--) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (breedcGrid[x][y] == -1 && breedcGrid[x][y - 1] >= 0) {
                breedcGrid[x][y] = breedcGrid[x][y - 1];
                breedcGrid[x][y - 1] = -1;
                dc++;
            }
        }
    }

    for (int y = cy; y <= GRID_SIZE - 2; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (breedcGrid[x][y] == -1 && breedcGrid[x][y + 1] >= 0) {
                breedcGrid[x][y] = breedcGrid[x][y + 1];
                breedcGrid[x][y + 1] = -1;
                dc++;
            }
        }
    }

    return dc;
}

int breedcCheckErasingDown() {
    int ec = 0;
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (!breedcErasingGrid[x][y]) {
                continue;
            }
            int c = breedcGrid[x][y];
            if (x < GRID_SIZE - 1 && !breedcErasingGrid[x + 1][y] && breedcGrid[x + 1][y] == c) {
                breedcErasingGrid[x + 1][y] = true;
                ec++;
            }
            if (y < GRID_SIZE - 1 && !breedcErasingGrid[x][y + 1] && breedcGrid[x][y + 1] == c) {
                breedcErasingGrid[x][y + 1] = true;
                ec++;
            }
        }
    }
    return ec;
}

int breedcCheckErasingUp() {
    int ec = 0;
    for (int x = GRID_SIZE - 1; x >= 0; x--) {
        for (int y = GRID_SIZE - 1; y >= 0; y--) {
            if (!breedcErasingGrid[x][y]) {
                continue;
            }
            int c = breedcGrid[x][y];
            if (x > 0 && !breedcErasingGrid[x - 1][y] && breedcGrid[x - 1][y] == c) {
                breedcErasingGrid[x - 1][y] = true;
                ec++;
            }
            if (y > 0 && !breedcErasingGrid[x][y - 1] && breedcGrid[x][y - 1] == c) {
                breedcErasingGrid[x][y - 1] = true;
                ec++;
            }
        }
    }
    return ec;
}

void addGameBreedc() {
    addGame(breedcTitle, breedcDescription, breedcCharacters, breedcCharactersCount,
            &breedcOptions, true, &breedcUpdate);
}
