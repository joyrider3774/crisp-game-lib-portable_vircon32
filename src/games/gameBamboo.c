#include "../cglp.h"

char* bambooTitle = "BAMBOO";
char* bambooDescription = "[Tap]  Turn\n[Hold] Through";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] bambooCharacters = {
    {   // Runner frame 1 'b'
        "  ll  ",
        "  l  l",
        "lpppp ",
        "  prrr",
        " r    ",
        "r     ",
    },
    {   // Runner frame 2 'c'
        "   ll ",
        "   l  ",
        "lpppp ",
        " rp  l",
        "r  r  ",
        "    r ",
    }
};
int bambooCharactersCount = 2;

Options bambooOptions = {200, 100, 1, false};

struct Bamboo {
    Vector pos;
    float height;
    float speed;
    bool isAlive;
};

#define MAX_BAMBOO_COUNT 80
Bamboo[MAX_BAMBOO_COUNT] bambooBamboos;
int bambooBambooIndex;
float bambooNextBambooTicks;
Vector bambooPlayerPos;
float bambooVx;
float bambooAvx;
float bambooAnimTicks;
int bambooSpeedBambooTicks;

void bambooUpdate() {
    Collision scratch;
    if (!ticks) {
        INIT_UNALIVED_ARRAY_FAST(bambooBamboos);
        bambooBambooIndex = 0;
        bambooNextBambooTicks = 0;
        vectorSet(&bambooPlayerPos, 190, 87);
        bambooVx = 1;
        bambooAvx = 0;
        bambooAnimTicks = 0;
        bambooSpeedBambooTicks = 5;
    }

    color = BLACK;
    if (input.isJustPressed) {
        play(SELECT);
        bambooVx *= -1;
    }

    bambooPlayerPos.x = cgl_wrap(bambooPlayerPos.x + bambooVx * difficulty * (1 + bambooAvx), -3, 203);
    bambooPlayerPos.y = 87;
    bambooAvx *= 0.9;
    bambooAnimTicks += difficulty;

    characterOptions.isMirrorX = bambooVx < 0;
    characterOptions.rotation = 0;
    int[2] playerChar;
    if (input.isPressed) {
        playerChar[0] = 'b';
    } else {
        playerChar[0] = 'a' + ((int)(bambooAnimTicks / 20) % 2);
    }
    playerChar[1] = 0;
    character(playerChar, bambooPlayerPos.x, bambooPlayerPos.y, &scratch);

    bambooNextBambooTicks--;
    if (bambooNextBambooTicks < 0) {
        bambooSpeedBambooTicks--;
        ASSIGN_ARRAY_ITEM(bambooBamboos, bambooBambooIndex, Bamboo, b);
        vectorSet(&b->pos, rnd(5, 195), 0);
        b->height = 0;
        if (bambooSpeedBambooTicks < 0) {
            b->speed = 2;
        } else {
            b->speed = 1;
        }
        b->isAlive = true;
        bambooBambooIndex = cgl_wrap(bambooBambooIndex + 1, 0, MAX_BAMBOO_COUNT);
        bambooNextBambooTicks = rnd(70, 100) / difficulty;
        bambooSpeedBambooTicks = rndi(4, 7);
    }

    FOR_EACH(bambooBamboos, i) {
        ASSIGN_ARRAY_ITEM(bambooBamboos, i, Bamboo, b);
        SKIP_IS_NOT_ALIVE(b);

        b->height += b->speed * difficulty * 0.14;
        float h = b->height / 4;
        float y = 90 - h / 2;
        if (h < 1) {
            y += (1 - h) * 3;
            h = 1;
        }

        bool collision = false;
        for (int j = 0; j < 4; j++) {
            if (b->height < 5) {
                color = LIGHT_YELLOW;
            } else if (b->height > 50) {
                color = GREEN;
            } else if (b->height > 25) {
                if (j % 2 == 0) {
                    color = GREEN;
                } else {
                    color = LIGHT_GREEN;
                }
            } else if (b->height > 23) {
                color = YELLOW;
            } else {
                if (j % 2 == 0) {
                    color = YELLOW;
                } else {
                    color = LIGHT_YELLOW;
                }
            }

            Collision cl;
            box(b->pos.x, y, (4 - j) * 2, ceil(h), &cl);
            if ((cl.isColliding.character['b'] || cl.isColliding.character['a']) && !input.isPressed) {
                collision = true;
            }
            y -= h;
        }

        if (collision) {
            if (b->height < 5) {
                // Do nothing
            } else if (b->height <= 25) {
                float s = ceil((b->height - 5) / 3);
                if (s == 7) {
                    s = 10;
                    play(POWER_UP);
                } else {
                    play(COIN);
                }
                addScore(s * s, b->pos.x, 90 - b->height);
                b->isAlive = false;
                continue;
            } else {
                play(HIT);
                b->speed *= 0.6;
                b->height *= 0.7;
                bambooAvx++;
                if (bambooAvx > 5) {
                    bambooAvx = 5;
                }
                bambooVx *= -1;
                float particleAngle;
                if (bambooVx > 0) {
                    particleAngle = 0;
                } else {
                    particleAngle = M_PI;
                }
                particle(b->pos.x, bambooPlayerPos.y, 9, difficulty * (1 + bambooAvx) * 0.5, particleAngle, 0.4);
                if (b->height <= 25) {
                    play(EXPLOSION);
                    b->isAlive = false;
                    continue;
                }
            }
        }

        if (b->height > 50) {
            b->speed *= 0.997;
        }
        if (b->height >= 89) {
            color = RED;
            text("X", b->pos.x, 3, &scratch);
            play(RANDOM);
            gameOver();
        }
    }

    color = PURPLE;
    rect(0, 90, 200, 10, &scratch);
}

void addGameBamboo() {
    addGame(bambooTitle, bambooDescription, bambooCharacters, bambooCharactersCount,
            &bambooOptions, false, &bambooUpdate);
}
