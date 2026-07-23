#include "../cglp.h"

char* crosslineTitle = "CROSS LINE";
char* crosslineDescription = "[Tap]\n Connect line";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] crosslineCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int crosslineCharactersCount = 0;

Options crosslineOptions = {100, 100, 1, false};

struct CrosslinePoint {
    Vector pos;
    float vy;
    bool isAlive;
};

struct CrosslineLine {
    Vector from;
    Vector to;
    float fromVy;
    float toVy;
    int addedCount;
    bool isAlive;
};

#define CROSSLINE_MAX_POINT_COUNT 64
#define CROSSLINE_MAX_LINE_COUNT 64

CrosslinePoint[CROSSLINE_MAX_POINT_COUNT] crosslinePoints;
CrosslineLine[CROSSLINE_MAX_LINE_COUNT] crosslineLines;
float crosslineNextPointDist;
CrosslinePoint* crosslineSelectedPoint;
float crosslinePointMaxY;
int crosslinePointCount;
int crosslinePointIndex;
int crosslineLineIndex;

// Vircon32 port note: "Vector from, Vector to" were passed BY VALUE here
// - now take pointers.
void crosslineAddLineParticle(Vector* from, Vector* to) {
    float d = distanceTo(from, to->x, to->y);
    Vector p;
    Vector o;
    vectorSet(&p, from->x, from->y);
    vectorSet(&o, 7, 0);
    rotate(&o, angleTo(from, to->x, to->y));

    for (int i = 0; i < floor(d / 7); i++) {
        particle(p.x, p.y, 5, 0.5, 0, M_PI * 2);
        vectorAdd(&p, o.x, o.y);
    }
}

void crosslineUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(crosslinePoints);
        INIT_UNALIVED_ARRAY_FAST(crosslineLines);
        crosslinePointIndex = 0;
        crosslineLineIndex = 0;

        ASSIGN_ARRAY_ITEM(crosslinePoints, crosslinePointIndex, CrosslinePoint, p);
        p->pos.x = 70;
        p->pos.y = 1;
        p->vy = 1;
        p->isAlive = true;
        crosslinePointIndex = cgl_wrap(crosslinePointIndex + 1, 0, CROSSLINE_MAX_POINT_COUNT);

        ASSIGN_ARRAY_ITEM(crosslineLines, crosslineLineIndex, CrosslineLine, l);
        l->from.x = 30;
        l->from.y = 99;
        l->to.x = 70;
        l->to.y = 1;
        l->fromVy = -1;
        l->toVy = 1;
        l->addedCount = 2;
        l->isAlive = true;
        crosslineLineIndex = cgl_wrap(crosslineLineIndex + 1, 0, CROSSLINE_MAX_LINE_COUNT);

        crosslineNextPointDist = 0;
        crosslineSelectedPoint = &crosslinePoints[0];
        crosslinePointMaxY = 0;
        crosslinePointCount = 0;
    }

    float scr = sqrt(difficulty) * 0.03;
    if (crosslinePointMaxY < 40) {
        scr += (40 - crosslinePointMaxY) * 0.1;
    }

    crosslineNextPointDist -= scr;
    while (crosslineNextPointDist < 0) {
        float vy;
        if (crosslinePointCount % 2 == 0) {
            vy = -1;
        } else {
            vy = 1;
        }
        int pc = (int)(floor(crosslinePointCount / 2)) % 7;
        float x;
        if (pc == 3) {
            x = rnd(10, 30);
        } else if (pc == 6) {
            x = rnd(70, 90);
        } else {
            x = rnd(10, 90);
        }

        ASSIGN_ARRAY_ITEM(crosslinePoints, crosslinePointIndex, CrosslinePoint, p);
        p->pos.x = x;
        if (vy > 0) {
            p->pos.y = -crosslineNextPointDist - 19;
        } else {
            p->pos.y = 119 + crosslineNextPointDist;
        }
        p->vy = vy;
        p->isAlive = true;
        crosslinePointIndex = cgl_wrap(crosslinePointIndex + 1, 0, CROSSLINE_MAX_POINT_COUNT);

        crosslineNextPointDist += 4;
        crosslinePointCount++;
    }

    color = LIGHT_RED;
    if (input.isJustPressed) {
        box(input.pos.x, input.pos.y, 5, 5, &scratch);
    }

    crosslinePointMaxY = 0;
    bool isSelected = false;

    FOR_EACH(crosslinePoints, i) {
        ASSIGN_ARRAY_ITEM(crosslinePoints, i, CrosslinePoint, p);
        SKIP_IS_NOT_ALIVE(p);

        if (p->pos.x > 99) {
            p->isAlive = false;
            continue;
        }

        p->pos.y += scr * p->vy;
        color = TRANSPARENT;
        Collision c;
        box(p->pos.x, p->pos.y, 4, 4, &c);

        if (p != crosslineSelectedPoint && !isSelected && c.isColliding.rect[LIGHT_RED]) {
            play(SELECT);
            isSelected = true;
            if (crosslineSelectedPoint == NULL) {
                crosslineSelectedPoint = p;
            } else {
                color = RED;
                line(p->pos.x, p->pos.y, crosslineSelectedPoint->pos.x, crosslineSelectedPoint->pos.y, &scratch);

                FOR_EACH(crosslineLines, j) {
                    ASSIGN_ARRAY_ITEM(crosslineLines, j, CrosslineLine, l);
                    SKIP_IS_NOT_ALIVE(l);
                    l->addedCount--;
                }

                ASSIGN_ARRAY_ITEM(crosslineLines, crosslineLineIndex, CrosslineLine, nl);
                nl->from = crosslineSelectedPoint->pos;
                nl->to = p->pos;
                nl->fromVy = crosslineSelectedPoint->vy;
                nl->toVy = p->vy;
                nl->addedCount = 2;
                nl->isAlive = true;
                crosslineLineIndex = cgl_wrap(crosslineLineIndex + 1, 0, CROSSLINE_MAX_LINE_COUNT);

                crosslineSelectedPoint->pos.x = 999;
                crosslineSelectedPoint = p;
            }
        }

        float my;
        if (p->vy > 0) {
            my = p->pos.y;
        } else {
            my = 100 - p->pos.y;
        }
        if (my > crosslinePointMaxY) {
            crosslinePointMaxY = my;
        }
    }

    int multiplier = 1;
    FOR_EACH(crosslineLines, i) {
        ASSIGN_ARRAY_ITEM(crosslineLines, i, CrosslineLine, l);
        SKIP_IS_NOT_ALIVE(l);

        l->from.y += scr * l->fromVy;
        l->to.y += scr * l->toVy;

        color = BLACK;
        Collision lineCl;
        line(l->from.x, l->from.y, l->to.x, l->to.y, &lineCl);
        if (lineCl.isColliding.rect[RED]) {
            if (l->addedCount <= 0) {
                crosslineAddLineParticle(&l->from, &l->to);
                Vector scorePos;
                vectorSet(&scorePos, (l->from.x + l->to.x) / 2, (l->from.y + l->to.y) / 2);
                addScore(multiplier, scorePos.x, scorePos.y);

                if (multiplier == 1) {
                    play(POWER_UP);
                    color = RED;
                    CrosslineLine* ll = &crosslineLines[(crosslineLineIndex - 1 + CROSSLINE_MAX_LINE_COUNT) % CROSSLINE_MAX_LINE_COUNT];
                    if (ll->isAlive) {
                        crosslineAddLineParticle(&ll->from, &ll->to);
                    }
                }
                multiplier *= 2;
                l->isAlive = false;
                continue;
            }
        }

        if (l->fromVy > 0) {
            color = LIGHT_BLUE;
        } else {
            color = LIGHT_CYAN;
        }
        box(l->from.x, l->from.y, 3, 3, &scratch);
        if (l->toVy > 0) {
            color = LIGHT_BLUE;
        } else {
            color = LIGHT_CYAN;
        }
        box(l->to.x, l->to.y, 3, 3, &scratch);
        color = RED;

        if (l->from.y < 0 || l->from.y > 99) {
            play(EXPLOSION);
            text("X", l->from.x, l->from.y, &scratch);
            gameOver();
        }
        if (l->to.y < 0 || l->to.y > 99) {
            play(EXPLOSION);
            text("X", l->to.x, l->to.y, &scratch);
            gameOver();
        }
    }

    FOR_EACH(crosslinePoints, i) {
        ASSIGN_ARRAY_ITEM(crosslinePoints, i, CrosslinePoint, p);
        SKIP_IS_NOT_ALIVE(p);
        if (crosslineSelectedPoint == p) {
            color = PURPLE;
        } else if (p->vy > 0) {
            color = BLUE;
        } else {
            color = CYAN;
        }
        box(p->pos.x, p->pos.y, 4, 4, &scratch);
    }
}

void addGameCrossLine() {
    addGame(crosslineTitle, crosslineDescription, crosslineCharacters, crosslineCharactersCount,
            &crosslineOptions, true, &crosslineUpdate);
}
