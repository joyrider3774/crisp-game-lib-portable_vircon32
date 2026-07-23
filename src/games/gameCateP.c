#include "../cglp.h"

char* catepTitle = "CATE P";
char* catepDescription = "[Tap]\n Turn & Shot";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] catepCharacters = {
    {
        " llll ",
        "llllll",
        "lllrrr",
        "lllrrr",
        "llllll",
        " llll ",
    },
    {
        " llll ",
        "llllll",
        "lll   ",
        "lll   ",
        "llllll",
        " llll ",
    },
    {
        " l ll ",
        "     l",
        " lllll",
        "     l",
        " l ll ",
        "      ",
    }
};
int catepCharactersCount = 3;

Options catepOptions = {100, 100, 11, true};

#define MAX_CATE_COUNT 32

struct CatepCate {
    Vector pos;
    int angle;
    float speed;
    float ticks;
    bool isAppearing;
    bool isAlive;
};

struct CatepAppCate {
    Vector pos;
    int angle;
    float speed;
    float ticks;
    int count;
    bool isAlive;
};

struct CatepPlayer {
    Vector pos;
    int angle;
};

struct CatepShot {
    Vector pos;
    int angle;
    float multiplier;
    bool isAlive;
};

CatepCate[MAX_CATE_COUNT] catepCates;
CatepAppCate catepAppCate;
CatepCate* catepAppearingCate;
CatepPlayer catepPlayer;
CatepShot catepShot;
float catepAppCateTicks;

// Vector array for angle-based movement
Vector[8] catepAngleVec = {
    {1, 0},    // 0
    {0.7, 0.7},// 1
    {0, 1},    // 2
    {-0.7, 0.7},// 3
    {-1, 0},   // 4
    {-0.7, -0.7},// 5
    {0, -1},   // 6
    {0.7, -0.7} // 7
};

// Vircon32 port note: "Vector pos" was passed BY VALUE here (2 words) -
// now takes a pointer, like every other by-value Vector param in this
// port.
int catepAddCate(Vector* pos, int angle, float speed, float ticks, bool isAppearing) {
    FOR_EACH(catepCates, i) {
        ASSIGN_ARRAY_ITEM(catepCates, i, CatepCate, c);
        if (!c->isAlive) {
            vectorSet(&c->pos, pos->x, pos->y);
            c->angle = angle;
            c->speed = speed;
            c->ticks = ticks;
            c->isAppearing = isAppearing;
            c->isAlive = true;
            return i;
        }
    }
    return -1;
}

void catepUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(catepCates);
        catepAppCateTicks = 0;
        catepAppearingCate = NULL;

        vectorSet(&catepPlayer.pos, 50.0, 50.0);
        catepPlayer.angle = 0;

        catepShot.isAlive = false;
    }

    // Draw border
    color = LIGHT_BLUE;
    rect(0, 0, 99, 6, &scratch);
    rect(0, 93, 99, 6, &scratch);
    rect(0, 6, 6, 87, &scratch);
    rect(93, 6, 6, 87, &scratch);

    if ((ticks && input.isJustPressed) ||
        catepPlayer.pos.x < 9 || catepPlayer.pos.x > (9 + 82) ||
        catepPlayer.pos.y < 9 || catepPlayer.pos.y > (9 + 82)) {
        if (!catepShot.isAlive && input.isJustPressed) {
            play(POWER_UP);
            vectorSet(&catepShot.pos, catepPlayer.pos.x, catepPlayer.pos.y);
            catepShot.angle = catepPlayer.angle;
            catepShot.multiplier = 1;
            catepShot.isAlive = true;
        }
        catepPlayer.pos.x = clamp(catepPlayer.pos.x, 9, 9 + 81);
        catepPlayer.pos.y = clamp(catepPlayer.pos.y, 9, 9 + 81);
        catepPlayer.angle = cgl_wrap(catepPlayer.angle + 2, 0, 8);
    }

    // Move player
    catepPlayer.pos.x += catepAngleVec[catepPlayer.angle].x * difficulty * 0.2;
    catepPlayer.pos.y += catepAngleVec[catepPlayer.angle].y * difficulty * 0.2;

    // Draw player
    color = BLACK;
    characterOptions.rotation = catepPlayer.angle / 2;
    if (catepShot.isAlive) {
        character("b", catepPlayer.pos.x, catepPlayer.pos.y, &scratch);
    } else {
        character("a", catepPlayer.pos.x, catepPlayer.pos.y, &scratch);
    }
    characterOptions.rotation = 0;

    // Handle shot
    if (catepShot.isAlive) {
        if (catepShot.pos.x < 9 || catepShot.pos.x > (9 + 82) ||
            catepShot.pos.y < 9 || catepShot.pos.y > (9 + 82)) {
            catepShot.isAlive = false;
        } else {
            catepShot.pos.x += catepAngleVec[catepShot.angle].x * difficulty * 1.5;
            catepShot.pos.y += catepAngleVec[catepShot.angle].y * difficulty * 1.5;
            color = BLACK;
            characterOptions.rotation = catepShot.angle / 2;
            character("c", catepShot.pos.x, catepShot.pos.y, &scratch);
            characterOptions.rotation = 0;
        }
    }

    // Spawn new Cate
    if (catepAppCateTicks <= 0) {
        catepAppCate.isAlive = true;
        float spawnY;
        if (rnd(0.0, 1.0) < 0.5) {
            spawnY = -3.0;
        } else {
            spawnY = 103.0;
        }
        vectorSet(&catepAppCate.pos, rnd(9.0, 90.0), spawnY);

        if (rnd(0.0, 1.0) < 0.5) {
            float temp = catepAppCate.pos.x;
            catepAppCate.pos.x = catepAppCate.pos.y;
            catepAppCate.pos.y = temp;
        }

        // Determine initial angle based on spawn position
        if (catepAppCate.pos.x > 99) catepAppCate.angle = 4;
        else if (catepAppCate.pos.y > 99) catepAppCate.angle = 6;
        else if (catepAppCate.pos.x < 0) catepAppCate.angle = 0;
        else catepAppCate.angle = 2;

        catepAppCate.speed = rnd(1.0, difficulty) * 0.2;
        catepAppCate.ticks = rnd(0.0, 999.0);
        catepAppCate.count = rndi(5, 9);

        catepAppCateTicks = rnd(60.0, 90.0) / difficulty;
    }

    // Process appearing Cate
    catepAppCate.ticks += catepAppCate.speed;
    if (catepAppCate.count <= 0) {
        catepAppCateTicks--;
    }

    // Check for new Cate spawning
    if (catepAppearingCate != NULL)
        if (catepAppearingCate->pos.x >= 3 && catepAppearingCate->pos.x <= (3 + 94) &&
            catepAppearingCate->pos.y >= 3 && catepAppearingCate->pos.y <= (3 + 94)) {
            catepAppearingCate = NULL;
        }

    // Spawn Cate if possible
    if (catepAppCate.count > 0 && catepAppearingCate == NULL) {
        play(LASER);

        Vector newPos;
        vectorSet(&newPos, catepAppCate.pos.x, catepAppCate.pos.y);
        int newCateIndex = catepAddCate(
            &newPos,
            catepAppCate.angle,
            catepAppCate.speed,
            catepAppCate.ticks,
            true
        );

        if (newCateIndex >= 0) {
            catepAppearingCate = &catepCates[newCateIndex];
            catepAppCate.count--;
        }
    }
    // Process existing Cates
    FOR_EACH(catepCates, i) {
        ASSIGN_ARRAY_ITEM(catepCates, i, CatepCate, c);
        SKIP_IS_NOT_ALIVE(c);

        c->pos.x += catepAngleVec[c->angle].x * c->speed;
        c->pos.y += catepAngleVec[c->angle].y * c->speed;

        // Handle appearing state
        if (c->isAppearing) {
            c->isAppearing = !(
                c->pos.x >= 9 && c->pos.x <= 9 + 81 &&
                c->pos.y >= 9 && c->pos.y <= 9 + 81
            );
        } else {
            int hitCount = 0;
            if (c->pos.x < 9 || c->pos.x > (9 + 81)) hitCount++;
            if (c->pos.y < 9 || c->pos.y > (9 + 81)) hitCount++;

            if (hitCount == 1) {
                c->angle = cgl_wrap(c->angle + 3, 0, 8);
                c->pos.x = clamp(c->pos.x, 9, 9 + 81);
                c->pos.y = clamp(c->pos.y, 9, 9 + 81);
            } else if (hitCount == 2) {
                c->angle = cgl_wrap(c->angle + 4, 0, 8);
                c->pos.x = clamp(c->pos.x, 9, 9 + 81);
                c->pos.y = clamp(c->pos.y, 9, 9 + 81);
            }
        }

        // Update ticks
        c->ticks += c->speed;
        int t = cgl_wrap((int)c->ticks, 0, 8);
        float lo;
        if (t < 4) {
            lo = (5 * t) / 5.0 - 2;
        } else {
            lo = (5 * (8 - t)) / 5.0 - 2;
        }

        // Draw Cate trajectory
        color = RED;
        Vector o;
        vectorSet(&o, 3.0, 0.0);
        rotate(&o, (c->angle * M_PI) / 4.0 + M_PI / 2);
        Vector boxPos1;
        vectorSet(&boxPos1, c->pos.x, c->pos.y);
        vectorAdd(&boxPos1, o.x, o.y);
        Vector o2;
        vectorSet(&o2, lo, 0.0);
        rotate(&o2, (c->angle * M_PI) / 4.0);
        vectorAdd(&boxPos1, o2.x, o2.y);
        box(boxPos1.x, boxPos1.y, 2, 2, &scratch);

        vectorSet(&o, -3.0, 0.0);
        rotate(&o, (c->angle * M_PI) / 4.0 + M_PI / 2);
        Vector boxPos2;
        vectorSet(&boxPos2, c->pos.x, c->pos.y);
        vectorAdd(&boxPos2, o.x, o.y);
        vectorSet(&o2, -lo, 0.0);
        rotate(&o2, (c->angle * M_PI) / 4.0);
        vectorAdd(&boxPos2, o2.x, o2.y);
        box(boxPos2.x, boxPos2.y, 2, 2, &scratch);

        // Check collision
        color = GREEN;
        Collision col;
        box(c->pos.x, c->pos.y, 5, 5, &col);
        if (col.isColliding.character['c']) {
            if (catepShot.isAlive) {
                play(COIN);
                addScore(catepShot.multiplier, c->pos.x, c->pos.y);
                particle(c->pos.x, c->pos.y, 9.0, sqrt(catepShot.multiplier), 0.0, M_PI * 2.0);
                catepShot.multiplier++;

                if (c == catepAppearingCate) {
                    catepAppearingCate = NULL;
                }

                c->isAlive = false;
            }
        }

        if (col.isColliding.character['a'] || col.isColliding.character['b']) {
            play(EXPLOSION);
            gameOver();
        }
    }
}

void addGameCateP() {
    addGame(catepTitle, catepDescription, catepCharacters, catepCharactersCount,
            &catepOptions, false, &catepUpdate);
}
