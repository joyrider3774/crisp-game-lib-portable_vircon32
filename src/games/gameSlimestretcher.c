#include "../cglp.h"

int* slimestretcherTitle = "SLIME STRETCHER";
int* slimestretcherDescription = "[Hold]\n Stretch\n[Release]\n Contract";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] slimestretcherCharacters = {{
    " llll ",
    "l  lll",
    "l llll",
    "l llll",
    "llll l",
    " llll ",
}};
int slimestretcherCharactersCount = 1;

Options slimestretcherOptions = {200, 100, 3, false};

struct SlimestretcherSlime {
  Vector pos;
  float width;
  float height;
  float baseWidth;
  float baseHeight;
  float maxHeight;
  float velocity;
  bool isOnGround;
};
SlimestretcherSlime slimestretcherSlime;

struct SlimestretcherWall {
  Vector pos;
  float width;
  float height;
  bool isAlive;
};
#define SLIMESTRETCHER_MAX_WALL_COUNT 32
SlimestretcherWall[SLIMESTRETCHER_MAX_WALL_COUNT] slimestretcherWalls;
int slimestretcherWallIndex;

struct SlimestretcherCollectible {
  Vector pos;
  bool isAlive;
};
#define SLIMESTRETCHER_MAX_COLLECTIBLE_COUNT 32
SlimestretcherCollectible[SLIMESTRETCHER_MAX_COLLECTIBLE_COUNT] slimestretcherCollectibles;
int slimestretcherCollectibleIndex;

float slimestretcherScrollVelocity;
float slimestretcherNextWallDistance;
float slimestretcherNextCollectibleDistance;
int slimestretcherMultiplier;

void slimestretcherUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&slimestretcherSlime.pos, 50, 90);
    slimestretcherSlime.width = 20;
    slimestretcherSlime.height = 20;
    slimestretcherSlime.baseWidth = 20;
    slimestretcherSlime.baseHeight = 20;
    slimestretcherSlime.maxHeight = 80;
    slimestretcherSlime.velocity = 1;
    slimestretcherSlime.isOnGround = true;
    INIT_UNALIVED_ARRAY_FAST(slimestretcherWalls);
    slimestretcherWallIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(slimestretcherCollectibles);
    slimestretcherCollectibleIndex = 0;
    slimestretcherScrollVelocity = 1;
    slimestretcherNextWallDistance = 50;
    slimestretcherNextCollectibleDistance = 30;
    slimestretcherMultiplier = 1;
  }
  if (input.isPressed) {
    slimestretcherSlime.height = fmin(slimestretcherSlime.height + 2, slimestretcherSlime.maxHeight);
    slimestretcherSlime.width = fmax(slimestretcherSlime.width - 0.5, slimestretcherSlime.baseWidth / 2);
    slimestretcherSlime.velocity = 0.5 * difficulty;
    slimestretcherSlime.pos.y = fmax(slimestretcherSlime.pos.y - 2, 10);
  } else {
    slimestretcherSlime.height = fmax(slimestretcherSlime.height - 4, slimestretcherSlime.baseHeight);
    slimestretcherSlime.width = fmin(slimestretcherSlime.width + 1, slimestretcherSlime.baseWidth);
    slimestretcherSlime.velocity = 1.5 * difficulty;
  }
  slimestretcherSlime.pos.x += slimestretcherSlime.velocity;
  if (slimestretcherSlime.pos.x < 50) {
    slimestretcherSlime.pos.x++;
  }
  if (slimestretcherSlime.pos.x < -slimestretcherSlime.width) {
    play(EXPLOSION);
    gameOver();
  }

  slimestretcherNextWallDistance -= slimestretcherSlime.velocity;
  if (slimestretcherNextWallDistance <= 0) {
    float wallWidth = rnd(20, 60);
    ASSIGN_ARRAY_ITEM(slimestretcherWalls, slimestretcherWallIndex, SlimestretcherWall, nw);
    vectorSet(&nw->pos, 200, rnd(0, 70));
    nw->width = wallWidth;
    nw->height = rnd(20, 40);
    nw->isAlive = true;
    slimestretcherWallIndex = cgl_wrap(slimestretcherWallIndex + 1, 0, SLIMESTRETCHER_MAX_WALL_COUNT);
    slimestretcherNextWallDistance = rndi(35, 55) + wallWidth;
  }

  slimestretcherNextCollectibleDistance -= slimestretcherSlime.velocity;
  if (slimestretcherNextCollectibleDistance <= 0) {
    ASSIGN_ARRAY_ITEM(slimestretcherCollectibles, slimestretcherCollectibleIndex, SlimestretcherCollectible, nc);
    vectorSet(&nc->pos, 203, rnd(20, 80));
    nc->isAlive = true;
    slimestretcherCollectibleIndex =
        cgl_wrap(slimestretcherCollectibleIndex + 1, 0, SLIMESTRETCHER_MAX_COLLECTIBLE_COUNT);
    slimestretcherNextCollectibleDistance = rndi(30, 60);
  }

  slimestretcherSlime.isOnGround = false;
  FOR_EACH(slimestretcherWalls, wi) {
    ASSIGN_ARRAY_ITEM(slimestretcherWalls, wi, SlimestretcherWall, wall);
    SKIP_IS_NOT_ALIVE(wall);
    if (slimestretcherSlime.pos.x + slimestretcherSlime.width > wall->pos.x &&
        slimestretcherSlime.pos.x < wall->pos.x + wall->width &&
        slimestretcherSlime.pos.y + slimestretcherSlime.height > wall->pos.y &&
        slimestretcherSlime.pos.y < wall->pos.y + wall->height) {
      if (slimestretcherSlime.pos.y + slimestretcherSlime.height < wall->pos.y + 10) {
        slimestretcherSlime.height--;
        slimestretcherSlime.pos.y = wall->pos.y - slimestretcherSlime.height;
        slimestretcherSlime.isOnGround = true;
      } else if (slimestretcherSlime.pos.y > wall->pos.y + wall->height - 10) {
        slimestretcherSlime.pos.y = wall->pos.y + wall->height;
        slimestretcherSlime.height -= 2;
      } else if (slimestretcherSlime.pos.x + slimestretcherSlime.width < wall->pos.x + 5) {
        slimestretcherSlime.pos.x = wall->pos.x - slimestretcherSlime.width;
        slimestretcherSlime.width--;
      } else {
        slimestretcherSlime.pos.y = wall->pos.y - slimestretcherSlime.height;
        slimestretcherSlime.isOnGround = true;
      }
    }
  }
  if (!slimestretcherSlime.isOnGround) {
    slimestretcherSlime.pos.y = fmin(slimestretcherSlime.pos.y + 1, 90);
  }
  if (slimestretcherSlime.pos.y + slimestretcherSlime.height >= 90) {
    slimestretcherSlime.pos.y = 90 - slimestretcherSlime.height;
    slimestretcherSlime.isOnGround = true;
  }

  color = GREEN;
  rect(slimestretcherSlime.pos.x, slimestretcherSlime.pos.y, slimestretcherSlime.width,
       slimestretcherSlime.height, &scratch);
  color = BLACK;
  FOR_EACH(slimestretcherWalls, wi2) {
    ASSIGN_ARRAY_ITEM(slimestretcherWalls, wi2, SlimestretcherWall, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    rect(obstacle->pos.x, obstacle->pos.y, obstacle->width, obstacle->height, &scratch);
  }
  rect(0, 90, 200, 10, &scratch);

  slimestretcherScrollVelocity = slimestretcherSlime.velocity;
  slimestretcherSlime.pos.x -= slimestretcherScrollVelocity;
  FOR_EACH(slimestretcherWalls, wi3) {
    ASSIGN_ARRAY_ITEM(slimestretcherWalls, wi3, SlimestretcherWall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.x -= slimestretcherScrollVelocity;
    if (w->pos.x < -w->width) {
      w->isAlive = false;
      continue;
    }
  }

  color = YELLOW;
  FOR_EACH(slimestretcherCollectibles, ci) {
    ASSIGN_ARRAY_ITEM(slimestretcherCollectibles, ci, SlimestretcherCollectible, c);
    SKIP_IS_NOT_ALIVE(c);
    Collision cl;
    character("a", c->pos.x, c->pos.y, &cl);
    if (cl.isColliding.rect[BLACK]) {
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.rect[GREEN]) {
      play(COIN);
      addScore(slimestretcherMultiplier, c->pos.x, c->pos.y);
      slimestretcherMultiplier++;
      c->isAlive = false;
      continue;
    }
    c->pos.x -= slimestretcherScrollVelocity;
    if (c->pos.x < -3) {
      slimestretcherMultiplier--;
      if (slimestretcherMultiplier < 1) {
        slimestretcherMultiplier = 1;
      }
      c->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  // Vircon32 port note: the JS version draws this with an `isSmallText`
  // option this port doesn't support (no small-font variant exists here) -
  // drawn at normal text size instead (visual only, see gameQuantumleaper.c
  // for precedent - it drops the same option the same way).
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(slimestretcherMultiplier));
  text(multText, 2, 10, &scratch);
}

void addGameSlimestretcher() {
  addGame(slimestretcherTitle, slimestretcherDescription, slimestretcherCharacters,
          slimestretcherCharactersCount, &slimestretcherOptions, false, &slimestretcherUpdate);
}
