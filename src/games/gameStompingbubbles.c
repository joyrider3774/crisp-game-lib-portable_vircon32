#include "../cglp.h"

int* stompingbubblesTitle = "STOMPING\nBUBBLES";
int* stompingbubblesDescription = "[Hold] Stomp";
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stompingbubblesCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stompingbubblesCharactersCount = 1;

Options stompingbubblesOptions = {100, 100, 5, false};

#define STOMPINGBUBBLES_PLAYER_SIZE 5
#define STOMPINGBUBBLES_MIN_BUBBLE_SIZE 5
#define STOMPINGBUBBLES_MAX_BUBBLE_SIZE 9
#define STOMPINGBUBBLES_MIN_BUBBLE_SPEED 0.1
#define STOMPINGBUBBLES_MAX_BUBBLE_SPEED 0.3

struct StompingbubblesPlayer {
  Vector pos;
  float vy;
};
StompingbubblesPlayer stompingbubblesPlayer;
// Intentionally not reset in the !ticks block - matches upstream, which never resets it either.
float stompingbubblesPlayerSpeed = 1.5;

struct StompingbubblesBubble {
  Vector pos;
  float size;
  float speed;
  bool isAlive;
};
#define STOMPINGBUBBLES_MAX_BUBBLE_COUNT 192
StompingbubblesBubble[STOMPINGBUBBLES_MAX_BUBBLE_COUNT] stompingbubblesBubbles;
int stompingbubblesBubbleIndex;
float stompingbubblesNextBubbleTicks;

struct StompingbubblesChainReaction {
  Vector pos;
  float size;
  float duration;
  bool isAlive;
};
#define STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT 64
StompingbubblesChainReaction[STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT] stompingbubblesChainReactions;
int stompingbubblesChainReactionIndex;

float stompingbubblesMultiplier;

void stompingbubblesUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&stompingbubblesPlayer.pos, 50, 0);
    stompingbubblesPlayer.vy = 0;
    INIT_UNALIVED_ARRAY_FAST(stompingbubblesBubbles);
    stompingbubblesBubbleIndex = 0;
    stompingbubblesNextBubbleTicks = 0;
    INIT_UNALIVED_ARRAY_FAST(stompingbubblesChainReactions);
    stompingbubblesChainReactionIndex = 0;
    stompingbubblesMultiplier = 1;
  }
  float sd = sqrt(difficulty);
  stompingbubblesPlayer.pos.x += stompingbubblesPlayerSpeed * sd;
  if ((stompingbubblesPlayer.pos.x > 95 && stompingbubblesPlayerSpeed > 0) ||
      (stompingbubblesPlayer.pos.x < 5 && stompingbubblesPlayerSpeed < 0)) {
    stompingbubblesPlayerSpeed *= -1;
  }
  stompingbubblesPlayer.pos.y += stompingbubblesPlayer.vy * sd;
  if (input.isPressed) {
    stompingbubblesPlayer.vy += 0.01 * 9;
  } else {
    stompingbubblesPlayer.vy += 0.01 * 1;
  }
  stompingbubblesPlayer.vy *= 0.99;
  if (stompingbubblesPlayer.pos.y < 0 && stompingbubblesPlayer.vy < 0) {
    stompingbubblesPlayer.vy *= -0.5;
  }
  if (stompingbubblesPlayer.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  stompingbubblesNextBubbleTicks -= sd;
  if (stompingbubblesNextBubbleTicks < 0) {
    float size = rnd(STOMPINGBUBBLES_MIN_BUBBLE_SIZE, STOMPINGBUBBLES_MAX_BUBBLE_SIZE);
    ASSIGN_ARRAY_ITEM(stompingbubblesBubbles, stompingbubblesBubbleIndex, StompingbubblesBubble, nb);
    vectorSet(&nb->pos, rnd(0, 100), 102 + size);
    nb->size = size;
    nb->speed = rnd(STOMPINGBUBBLES_MIN_BUBBLE_SPEED, STOMPINGBUBBLES_MAX_BUBBLE_SPEED);
    nb->isAlive = true;
    stompingbubblesBubbleIndex = cgl_wrap(stompingbubblesBubbleIndex + 1, 0, STOMPINGBUBBLES_MAX_BUBBLE_COUNT);
    stompingbubblesNextBubbleTicks += 9;
  }
  FOR_EACH(stompingbubblesBubbles, bi) {
    ASSIGN_ARRAY_ITEM(stompingbubblesBubbles, bi, StompingbubblesBubble, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= b->speed * sd;
  }
  FOR_EACH(stompingbubblesChainReactions, ci) {
    ASSIGN_ARRAY_ITEM(stompingbubblesChainReactions, ci, StompingbubblesChainReaction, cr);
    SKIP_IS_NOT_ALIVE(cr);
    cr->size += sd;
    cr->duration -= sd;
    if (cr->duration <= 0) {
      cr->isAlive = false;
      continue;
    }
  }

  color = RED;
  box(stompingbubblesPlayer.pos.x, stompingbubblesPlayer.pos.y, STOMPINGBUBBLES_PLAYER_SIZE,
      STOMPINGBUBBLES_PLAYER_SIZE, &scratch);
  color = YELLOW;
  FOR_EACH(stompingbubblesChainReactions, ci2) {
    ASSIGN_ARRAY_ITEM(stompingbubblesChainReactions, ci2, StompingbubblesChainReaction, cr2);
    SKIP_IS_NOT_ALIVE(cr2);
    arc(cr2->pos.x, cr2->pos.y, cr2->size, 0, CGLP_PI * 2, &scratch);
  }

  color = CYAN;
  bool isHit = false;
  FOR_EACH(stompingbubblesBubbles, bi2) {
    ASSIGN_ARRAY_ITEM(stompingbubblesBubbles, bi2, StompingbubblesBubble, b2);
    SKIP_IS_NOT_ALIVE(b2);
    Collision c;
    arc(b2->pos.x, b2->pos.y, b2->size, 0, CGLP_PI * 2, &c);
    if (c.isColliding.rect[RED] || c.isColliding.rect[YELLOW]) {
      play(POWER_UP);
      particle(b2->pos.x, b2->pos.y, b2->size * 2, 3, 0, CGLP_PI * 2);
      addScore(stompingbubblesMultiplier, b2->pos.x, b2->pos.y);
      stompingbubblesMultiplier++;
      ASSIGN_ARRAY_ITEM(stompingbubblesChainReactions, stompingbubblesChainReactionIndex,
                         StompingbubblesChainReaction, ncr);
      vectorSet(&ncr->pos, b2->pos.x, b2->pos.y);
      ncr->size = b2->size;
      ncr->duration = 5;
      ncr->isAlive = true;
      stompingbubblesChainReactionIndex =
          cgl_wrap(stompingbubblesChainReactionIndex + 1, 0, STOMPINGBUBBLES_MAX_CHAIN_REACTION_COUNT);
      if (c.isColliding.rect[RED] && !isHit) {
        stompingbubblesPlayer.vy *= -1;
        stompingbubblesPlayer.pos.y += stompingbubblesPlayer.vy * 2;
        isHit = true;
      }
      b2->isAlive = false;
      continue;
    }
    if (b2->pos.y < -b2->size) {
      b2->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(stompingbubblesChainReactions, chainReactionCount);
  if (chainReactionCount == 0) {
    stompingbubblesMultiplier = 1;
  }
}

void addGameStompingbubbles() {
  addGame(stompingbubblesTitle, stompingbubblesDescription, stompingbubblesCharacters,
          stompingbubblesCharactersCount, &stompingbubblesOptions, false, &stompingbubblesUpdate);
}
