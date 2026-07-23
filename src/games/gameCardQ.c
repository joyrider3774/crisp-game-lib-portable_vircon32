#include "../cglp.h"

char* cardqTitle = "CARD Q";
char* cardqDescription = "[Tap]\n Pull out a card";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] cardqCharacters = {
    {
        "l  l  ",
        "l l l ",
        "l l l ",
        "l l l ",
        "l l l ",
        "l  l  ",
    }
};
int cardqCharactersCount = 1;

Options cardqOptions = {100, 100, 3, false};

// Card number characters
int*[14] cardqNumChars = {
    "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

struct CardqCard {
    int num;
    Vector pos;
    Vector tPos;
    Vector gPos;
    bool isAlive;
};

struct CardqGameState {
    int activeCardCount;
};

#define CARD_INTERVAL_X 15
#define CARD_ROW_COUNT 5
#define CARD_COLUMN_COUNT 5
#define MAX_CARDS (CARD_ROW_COUNT * CARD_COLUMN_COUNT)
#define MAX_PLACED_CARDS 20

CardqCard[MAX_CARDS] cardqPlayerCards;
CardqCard[MAX_CARDS] cardqEnemyCards;
CardqCard[MAX_PLACED_CARDS] cardqPlacedCards;
CardqGameState cardqPlayerState;
CardqGameState cardqEnemyState;
int cardqPlacedCardIndex;
int[2] cardqPlacedCardNumbers;
float cardqCenterY;
float cardqTargetCenterY;
int cardqPlayerPrevMoveIndex;
int cardqEnemyPrevMoveIndex;
int cardqEnemyNextMoveIndex;
bool cardqHasEnemyNextMove;
float cardqEnemyNextMoveTicks;
float cardqShuffleTicks;
int cardqShuffleCount;
int cardqPenaltyIndex;
float cardqPenaltyTicks;
int cardqMultiplier;

// Forward declarations
void cardqMovePos(Vector* p, Vector* tp, float ratio);
float cardqCalcCardX(int i);
float cardqCalcPlacedCardX(int i);
void cardqCalcPlayerCardPos(Vector* pos, Vector* p);
void cardqCalcEnemyCardPos(Vector* pos, Vector* p);
void cardqDrawCard(float x, float y, int n, int gy, int* edgeColor);
void cardqCheckPlacedIndex(int idx, int ppi, CardqCard* cards, int* outPi, int* outCn, int* outCi);
int cardqPlaceCard(int idx, int ppi, CardqCard* cards);
void cardqDropCardsInColumn(CardqCard* cards, int idx);
void cardqAddPlacedCard(CardqCard* card, int pIdx);

// Vircon32 port note: "Card card" was passed BY VALUE here (~8 words,
// way over the 1-word limit) - now takes a pointer, like every other
// by-value struct param in this port.
void cardqAddPlacedCard(CardqCard* card, int pIdx) {
    if (cardqPlacedCardIndex >= 19) {
        memcpy(&cardqPlacedCards[0], &cardqPlacedCards[1], (cardqPlacedCardIndex - 1) * sizeof(cardqPlacedCards[0]));
        cardqPlacedCardIndex--;
    }
    cardqPlacedCards[cardqPlacedCardIndex] = *card;
    cardqPlacedCards[cardqPlacedCardIndex].tPos.x = cardqCalcPlacedCardX(pIdx);
    cardqPlacedCards[cardqPlacedCardIndex].tPos.y = 0;
    cardqPlacedCards[cardqPlacedCardIndex].isAlive = true;
    cardqPlacedCardIndex++;
}

void cardqDropCardsInColumn(CardqCard* cards, int idx) {
    int moveCount = 0;
    CardqCard[CARD_ROW_COUNT] movedCards;
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].isAlive && cards[i].gPos.x == idx) {
            movedCards[moveCount] = cards[i];
            moveCount++;
            cards[i].isAlive = false;
        }
    }

    for (int i = 0; i < moveCount; i++) {
        CardqCard c = movedCards[i];
        c.gPos.y--;
        for (int j = 0; j < MAX_CARDS; j++) {
            if (!cards[j].isAlive) {
                cards[j] = c;
                if (cards == &cardqPlayerCards[0]) {
                    cardqCalcPlayerCardPos(&cards[j].tPos, &cards[j].gPos);
                } else {
                    cardqCalcEnemyCardPos(&cards[j].tPos, &cards[j].gPos);
                }
                cards[j].isAlive = true;
                break;
            }
        }
    }
}

void cardqUpdate() {
    Collision scratch;
    // Never reads a Collision result anywhere in this file - the tapped
    // card column is computed directly from input.pos via grid math (see
    // "pci" below), so the engine's own O(n^2) hitbox scan (see
    // checkHitBox() in cglp.c) is pure waste here. Restored automatically
    // when the next real game starts, via resetDrawState() in
    // initInGame().
    hasCollision = false;
    if (!ticks) {
        cardqPlacedCardNumbers[0] = 2;
        cardqPlacedCardNumbers[1] = 12;
        cardqPlacedCardIndex = 0;
        cardqPlayerState.activeCardCount = 0;
        cardqEnemyState.activeCardCount = 0;

        for (int i = 0; i < 2; i++) {
            CardqCard card;
            memset(&card, 0, sizeof(CardqCard));
            card.num = cardqPlacedCardNumbers[i];
            vectorSet(&card.pos, cardqCalcPlacedCardX(i), 0);
            card.tPos = card.pos;
            card.isAlive = true;
            cardqAddPlacedCard(&card, i);
        }

        // Initialize player cards
        int[5] firstRow = {1, 3, 3, 11, 13};
        for (int i = 0; i < MAX_CARDS; i++) {
            Vector gPos;
            gPos.x = i % CARD_COLUMN_COUNT;
            gPos.y = floor(i / CARD_COLUMN_COUNT);
            cardqPlayerCards[i].gPos = gPos;
            if (gPos.y == 0) {
                cardqPlayerCards[i].num = firstRow[(int)gPos.x];
            } else {
                cardqPlayerCards[i].num = rnd(1, 14);
            }
            cardqCalcPlayerCardPos(&cardqPlayerCards[i].pos, &gPos);
            cardqPlayerCards[i].tPos = cardqPlayerCards[i].pos;
            cardqPlayerCards[i].isAlive = true;
            cardqPlayerState.activeCardCount++;
        }

        // Initialize enemy cards
        for (int i = 0; i < MAX_CARDS; i++) {
            Vector gPos;
            gPos.x = i % CARD_COLUMN_COUNT;
            gPos.y = floor(i / CARD_COLUMN_COUNT);
            cardqEnemyCards[i].gPos = gPos;
            cardqEnemyCards[i].num = rnd(1, 14);
            cardqCalcEnemyCardPos(&cardqEnemyCards[i].pos, &gPos);
            cardqEnemyCards[i].tPos = cardqEnemyCards[i].pos;
            cardqEnemyCards[i].isAlive = true;
            cardqEnemyState.activeCardCount++;
        }

        cardqCenterY = cardqTargetCenterY = 40;
        cardqPlayerPrevMoveIndex = cardqEnemyPrevMoveIndex = 0;
        cardqHasEnemyNextMove = false;
        cardqEnemyNextMoveTicks = 120;
        cardqShuffleTicks = cardqShuffleCount = 0;
        cardqPenaltyTicks = -1;
        cardqMultiplier = 1;
    }

    cardqShuffleTicks++;
    if (cardqShuffleTicks > 60) {
        bool isPlacable = false;
        bool isPlayerPlacable = false;

        for (int i = 0; i < CARD_COLUMN_COUNT; i++) {
            int pIdx, cn, ci;
            cardqCheckPlacedIndex(i, cardqPlayerPrevMoveIndex, cardqPlayerCards, &pIdx, &cn, &ci);
            if (pIdx >= 0) {
                isPlacable = isPlayerPlacable = true;
                break;
            }

            cardqCheckPlacedIndex(i, cardqEnemyPrevMoveIndex, cardqEnemyCards, &pIdx, &cn, &ci);
            if (pIdx >= 0) {
                isPlacable = true;
                break;
            }
        }

        if (!isPlayerPlacable) {
            cardqEnemyNextMoveTicks *= 0.3;
        }

        cardqShuffleCount++;
        if (!isPlacable || cardqShuffleCount > 2) {
            play(POWER_UP);

            for (int i = 0; i < cardqPlacedCardIndex; i++) {
                if (cardqPlacedCards[i].pos.x < 50) {
                    cardqPlacedCards[i].tPos.x = -50;
                } else {
                    cardqPlacedCards[i].tPos.x = 150;
                }
            }

            cardqPlacedCardNumbers[0] = rnd(1, 14);
            cardqPlacedCardNumbers[1] = rnd(1, 14);

            for (int i = 0; i < 2; i++) {
                CardqCard card;
                memset(&card, 0, sizeof(CardqCard));
                card.num = cardqPlacedCardNumbers[i];
                if (i == 0) {
                    card.pos.x = -5;
                } else {
                    card.pos.x = 105;
                }
                card.pos.y = 0;
                card.isAlive = true;
                cardqAddPlacedCard(&card, i);
            }

            cardqShuffleCount = 0;
        }
        cardqShuffleTicks = 0;
    }

    // Handle input
    int pci = floor((input.pos.x - 50) / CARD_INTERVAL_X + CARD_COLUMN_COUNT / 2.0);

    if (input.isJustPressed) {
        if (pci >= 0 && pci < CARD_COLUMN_COUNT) {
            int result = cardqPlaceCard(pci, cardqPlayerPrevMoveIndex, cardqPlayerCards);
            if (result < 0) {
                play(HIT);
                cardqPenaltyIndex = pci;
                cardqPenaltyTicks = 60;
                cardqTargetCenterY += 5;
                cardqMultiplier = 1;
                cardqShuffleTicks = cardqShuffleCount = 0;
            } else {
                play(COIN);
                cardqPlayerPrevMoveIndex = result;
                cardqTargetCenterY -= 5;
                float scoreX;
                if (result == 0) {
                    scoreX = 8;
                } else {
                    scoreX = 92;
                }
                addScore(cardqMultiplier, scoreX, cardqCenterY);
                cardqMultiplier++;
            }
        }
    }

    // Enemy AI
    cardqEnemyNextMoveTicks--;
    if (cardqEnemyNextMoveTicks < 0) {
        cardqEnemyNextMoveTicks = rnd(50, 70) / sqrt(difficulty);

        if (cardqHasEnemyNextMove) {
            int pIdx, cn, ci;
            cardqCheckPlacedIndex(cardqEnemyNextMoveIndex, cardqEnemyPrevMoveIndex, cardqEnemyCards, &pIdx, &cn, &ci);
            if (pIdx < 0) {
                cardqEnemyNextMoveTicks *= 3;
            } else {
                play(SELECT);
                cardqPlaceCard(cardqEnemyNextMoveIndex, cardqEnemyPrevMoveIndex, cardqEnemyCards);
                cardqEnemyPrevMoveIndex = pIdx;
                cardqTargetCenterY += 5;
                cardqMultiplier = 1;
            }
        }

        cardqHasEnemyNextMove = false;
        int ni = rnd(0, CARD_COLUMN_COUNT);

        for (int i = 0; i < CARD_COLUMN_COUNT; i++) {
            int pIdx, cn, ci;
            cardqCheckPlacedIndex(ni, cardqEnemyPrevMoveIndex, cardqEnemyCards, &pIdx, &cn, &ci);
            if (pIdx >= 0) {
                if (pIdx != cardqEnemyPrevMoveIndex) {
                    cardqEnemyNextMoveTicks *= 1.5;
                }
                cardqHasEnemyNextMove = true;
                cardqEnemyNextMoveIndex = ni;
                break;
            }
            ni = cgl_wrap(ni + 1, 0, CARD_COLUMN_COUNT);
        }
    }

    // Update positions
    cardqCenterY += (cardqTargetCenterY - cardqCenterY) * 0.1;

    // Draw player cards
    for (int i = 0; i < MAX_CARDS; i++) {
        CardqCard* c = &cardqPlayerCards[i];
        if (!c->isAlive) continue;

        cardqMovePos(&c->pos, &c->tPos, 0.2);
        int* ec;
        if (c->gPos.y == 0 && c->gPos.x == pci) {
            ec = "green";
        } else {
            ec = NULL;
        }
        cardqDrawCard(c->pos.x, c->pos.y + cardqCenterY, c->num, c->gPos.y, ec);
    }

    // Draw enemy cards
    for (int i = 0; i < MAX_CARDS; i++) {
        CardqCard* c = &cardqEnemyCards[i];
        if (!c->isAlive) continue;

        cardqMovePos(&c->pos, &c->tPos, 0.2);
        int* ec;
        if (c->gPos.y == 0 && c->gPos.x == cardqEnemyNextMoveIndex && cardqHasEnemyNextMove) {
            ec = "red";
        } else {
            ec = NULL;
        }
        cardqDrawCard(c->pos.x, c->pos.y + cardqCenterY, c->num, c->gPos.y, ec);
    }

    // Draw placed cards
    for (int i = 0; i < cardqPlacedCardIndex; i++) {
        CardqCard* c = &cardqPlacedCards[i];
        if (!c->isAlive) continue;

        cardqMovePos(&c->pos, &c->tPos, 0.2);
        cardqDrawCard(c->pos.x, c->pos.y + cardqCenterY, c->num, 0, NULL);
    }

    // Draw penalty
    if (cardqPenaltyTicks > 0) {
        cardqPenaltyTicks--;
        color = RED;
        text("X", cardqCalcCardX(cardqPenaltyIndex), cardqCenterY + 6, &scratch);
        color = BLACK;
    }

    // Update center position
    if (cardqTargetCenterY < 16) {
        cardqTargetCenterY += (16 - cardqTargetCenterY) * 0.1;
    }

    if (cardqCenterY > 94) {
        play(EXPLOSION);
        gameOver();
    }
}

void cardqMovePos(Vector* p, Vector* tp, float ratio) {
    Vector diff;
    diff.x = tp->x - p->x;
    diff.y = tp->y - p->y;
    vectorMul(&diff, ratio);
    vectorAdd(p, diff.x, diff.y);

    if (distanceTo(p, tp->x, tp->y) < 1) {
        p->x = tp->x;
        p->y = tp->y;
    }
}

float cardqCalcCardX(int i) {
    return 50.5 + (i - CARD_COLUMN_COUNT / 2.0 + 0.5) * CARD_INTERVAL_X;
}

float cardqCalcPlacedCardX(int i) {
    return 50 + (i - 0.5) * 25;
}

// Vircon32 port note: "Vector p" was passed BY VALUE here - now takes a
// pointer.
void cardqCalcPlayerCardPos(Vector* pos, Vector* p) {
    pos->x = cardqCalcCardX(p->x);
    pos->y = (p->y + 1) * 11;
}

void cardqCalcEnemyCardPos(Vector* pos, Vector* p) {
    pos->x = cardqCalcCardX(p->x);
    pos->y = (p->y + 1) * -11;
}

void cardqDrawCard(float x, float y, int n, int gy, int* edgeColor) {
    Collision scratch;
    if (y < -5 || y > 105) return;

    if (edgeColor != NULL) {
        if (strcmp(edgeColor, "green") == 0) color = GREEN;
        else if (strcmp(edgeColor, "red") == 0) color = RED;
    } else {
        if (gy == 0) {
            color = BLACK;
        } else {
            color = LIGHT_BLACK;
        }
    }
    box(x, y, 9, 10, &scratch);

    // Draw card background
    color = WHITE;
    box(x, y, 7, 8, &scratch);

    // Draw number
    if (gy == 0) {
        color = BLACK;
    } else {
        color = LIGHT_BLACK;
    }
    if (n == 10) {
        character("a", x, y, &scratch);
    } else {
        text(cardqNumChars[n], x, y, &scratch);
    }
}

void cardqCheckPlacedIndex(int idx, int ppi, CardqCard* cards, int* outPi, int* outCn, int* outCi) {
    *outPi = -1;
    *outCn = -1;
    *outCi = -1;

    for (int i = 0; i < MAX_CARDS; i++) {
        if (cards[i].isAlive && cards[i].gPos.y == 0 && cards[i].gPos.x == idx) {
            *outCn = cards[i].num;
            *outCi = i;
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        if ((ppi == 1 || *outPi == -1) &&
            (*outCn == cgl_wrap(cardqPlacedCardNumbers[i] - 1, 1, 14) ||
             *outCn == cgl_wrap(cardqPlacedCardNumbers[i] + 1, 1, 14))) {
            *outPi = i;
        }
    }
}

int cardqPlaceCard(int idx, int ppi, CardqCard* cards) {
    int pIdx, cn, ci;
    cardqCheckPlacedIndex(idx, ppi, cards, &pIdx, &cn, &ci);
    if (pIdx == -1) {
        return -1;
    }

    CardqCard movedCard = cards[ci];
    cards[ci].isAlive = false;

    cardqPlacedCardNumbers[pIdx] = cn;

    cardqAddPlacedCard(&movedCard, pIdx);

    cardqDropCardsInColumn(cards, idx);

    Vector gPos;
    gPos.x = idx;
    gPos.y = CARD_ROW_COUNT - 1;
    for (int i = 0; i < MAX_CARDS; i++) {
        if (!cards[i].isAlive) {
            cards[i].gPos = gPos;
            cards[i].num = rnd(1, 14);
            if (cards == &cardqPlayerCards[0]) {
                cardqCalcPlayerCardPos(&cards[i].pos, &gPos);
            } else {
                cardqCalcEnemyCardPos(&cards[i].pos, &gPos);
            }
            cards[i].tPos = cards[i].pos;
            cards[i].isAlive = true;
            break;
        }
    }

    cardqShuffleTicks = cardqShuffleCount = 0;
    return pIdx;
}

void addGameCardq() {
    addGame(cardqTitle, cardqDescription, cardqCharacters, cardqCharactersCount,
            &cardqOptions, true, &cardqUpdate);
}
