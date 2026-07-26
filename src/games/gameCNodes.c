#include "../cglp.h"

char* cnodesTitle = "C NODES";
char* cnodesDescription = "[Tap] Cut";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] cnodesCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int cnodesCharactersCount = 1;

Options cnodesOptions = {100, 100, 6, false};

struct CnodesNode {
    Vector pos;
    Vector vel;
    int color;
    bool isAlive;
};

struct CnodesLine {
    CnodesNode* node1;
    CnodesNode* node2;
    float length;
    bool isAlive;
};

#define MAX_NODES 64
#define MAX_LINES 64

CnodesNode[MAX_NODES] cnodesNodes;
CnodesLine[MAX_LINES] cnodesLines;
float cnodesNextNodeDist;
float cnodesMaxY;
float cnodesScr;
float cnodesPressedScr;
int cnodesMultiplier = 1;
float cnodesFixedY = 20;

CnodesNode* cnodesFindFreeNode() {
    FOR_EACH(cnodesNodes, i) {
        ASSIGN_ARRAY_ITEM(cnodesNodes, i, CnodesNode, n);
        if (!n->isAlive) {
            return n;
        }
    }
    return NULL;
}

CnodesLine* cnodesFindFreeLine() {
    FOR_EACH(cnodesLines, i) {
        ASSIGN_ARRAY_ITEM(cnodesLines, i, CnodesLine, l);
        if (!l->isAlive) {
            return l;
        }
    }
    return NULL;
}

void cnodesCheckRedNodes() {
    FOR_EACH(cnodesNodes, i) {
        ASSIGN_ARRAY_ITEM(cnodesNodes, i, CnodesNode, n);
        if (n->color == RED) {
            n->color = BLUE;
        }
    }

    for (int iteration = 0; iteration < 9; iteration++) {
        bool isAddingRed = false;
        FOR_EACH(cnodesLines, j) {
            ASSIGN_ARRAY_ITEM(cnodesLines, j, CnodesLine, l);
            SKIP_IS_NOT_ALIVE(l);

            if (l->node1->color == BLACK || l->node1->color == RED ||
                l->node2->color == BLACK || l->node2->color == RED) {

                if (l->node1->color == BLUE) {
                    l->node1->color = RED;
                    isAddingRed = true;
                }
                if (l->node2->color == BLUE) {
                    l->node2->color = RED;
                    isAddingRed = true;
                }
            }
        }

        if (!isAddingRed) {
            break;
        }
    }
}

void cnodesUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(cnodesNodes);
        INIT_UNALIVED_ARRAY(cnodesLines);
        cnodesNextNodeDist = 0;
        cnodesScr = 0;
        cnodesPressedScr = 0;
        cnodesMultiplier = 1;
    }

    color = LIGHT_RED;
    rect(0, 0, 100, cnodesFixedY, &scratch);

    color = LIGHT_BLACK;
    if (input.isJustPressed && input.pos.y > cnodesFixedY) {
        play(HIT);
        box(input.pos.x, input.pos.y, 7, 7, &scratch);
        cnodesMultiplier = 1;
        cnodesPressedScr = 1;
    }

    cnodesScr = difficulty * 0.03 + cnodesPressedScr;
    cnodesPressedScr *= 0.7;
    if (cnodesMaxY < cnodesFixedY + 9) {
        cnodesScr += (cnodesFixedY + 9 - cnodesMaxY) * 0.1;
    }

    cnodesNextNodeDist -= cnodesScr;
    if (cnodesNextNodeDist < 0) {
        CnodesNode* nn = cnodesFindFreeNode();
        if (nn) {
            vectorSet(&nn->pos, rnd(10, 90), 0);
            vectorSet(&nn->vel, 0, 0);
            nn->color = BLACK;
            nn->isAlive = true;

            FOR_EACH(cnodesNodes, i) {
                ASSIGN_ARRAY_ITEM(cnodesNodes, i, CnodesNode, n);
                SKIP_IS_NOT_ALIVE(n);

                if (n->color == BLACK &&
                    (fabs(nn->pos.x - n->pos.x) / 2 + fabs(nn->pos.y - n->pos.y) < 30) &&
                    rnd(0, 1) < 0.7) {
                    CnodesLine* newLine = cnodesFindFreeLine();
                    if (newLine) {
                        newLine->node1 = nn;
                        newLine->node2 = n;
                        newLine->length = distanceTo(&nn->pos, n->pos.x, n->pos.y);
                        newLine->isAlive = true;
                    }
                }
            }

            play(LASER);
            cnodesNextNodeDist = 4;
        }
    }

    color = BLACK;
    FOR_EACH(cnodesLines, i) {
        ASSIGN_ARRAY_ITEM(cnodesLines, i, CnodesLine, l);
        SKIP_IS_NOT_ALIVE(l);

        float d = distanceTo(&l->node1->pos, l->node2->pos.x, l->node2->pos.y) - l->length;
        float a = angleTo(&l->node1->pos, l->node2->pos.x, l->node2->pos.y);

        addWithAngle(&l->node1->vel, a, d * 0.02);
        addWithAngle(&l->node2->vel, a, -d * 0.02);

        thickness = 2;
        Collision lineCl;
        line(l->node1->pos.x, l->node1->pos.y, l->node2->pos.x, l->node2->pos.y, &lineCl);
        if (lineCl.isColliding.rect[LIGHT_BLACK]) {
            play(SELECT);
            l->isAlive = false;
        }

        l->isAlive &= !(l->node1->pos.y > 100 && l->node2->pos.y > 100);
    }

    cnodesMaxY = 0;
    FOR_EACH(cnodesNodes, i) {
        ASSIGN_ARRAY_ITEM(cnodesNodes, i, CnodesNode, n);
        SKIP_IS_NOT_ALIVE(n);

        n->pos.y += cnodesScr;
        if (cnodesMaxY < n->pos.y) {
            cnodesMaxY = n->pos.y;
        }

        if (n->pos.y > cnodesFixedY) {
            if (n->color == BLACK) {
                n->color = BLUE;
                cnodesCheckRedNodes();
            }
            vectorAdd(&n->pos, n->vel.x, n->vel.y);
            n->vel.y += 0.03;
            vectorMul(&n->vel, 0.95);
        } else {
            vectorSet(&n->vel, 0, 0);
        }

        if (n->color == BLACK) {
            color = BLACK;
        } else if (n->color == RED) {
            color = RED;
        } else {
            color = BLUE;
        }
        box(n->pos.x, n->pos.y, 5, 5, &scratch);

        if (n->pos.y > 100 && n->color == RED) {
            play(EXPLOSION);
            color = RED;
            text("X", n->pos.x, 95, &scratch);
            gameOver();
        }

        if (n->pos.y > 140) {
            play(POWER_UP);
            addScore(cnodesMultiplier, n->pos.x - 1, 99);
            if (cnodesMultiplier < 64) {
                cnodesMultiplier *= 2;
            }
            n->isAlive = false;
        }
    }

    if (input.isPressed) {
        cnodesCheckRedNodes();
    }
}

void addGameCNodes() {
    addGame(cnodesTitle, cnodesDescription, cnodesCharacters, cnodesCharactersCount,
            &cnodesOptions, true, &cnodesUpdate);
}
