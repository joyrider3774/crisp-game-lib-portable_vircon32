#include "../cglp.h"

char* grapplinghTitle = "GRAPPLING H";
char* grapplinghDescription = "\n[Tap]\n Release hook\n Hold anchor";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] grapplinghCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int grapplinghCharactersCount = 0;

Options grapplinghOptions = {100, 150, 5, false};

struct GrapplinghPlayer {
  Vector pos;
  Vector vel;
  Vector* attachedAnchor;
  bool isAlive;
};
struct AnchorPoint {
  Vector pos;
  bool isAlive;
};

#define MAX_ANCHOR_POINTS 32
GrapplinghPlayer grapplinghPlayer;
AnchorPoint[MAX_ANCHOR_POINTS] grapplinghAnchorPoints;
int grapplinghAnchorPointIndex;
float grapplinghScrolledDistanceY;
float grapplinghScrolledDistanceYForScore;
bool grapplinghInputIsReleased;
bool grapplinghInputIsJustPressed;
float grapplinghNextAnchorDistance = 20;
float grapplinghPlayerRadius = 4;
float grapplinghAnchorRadius = 1;

void grapplinghAddAnchorPoint(float x, float y) {
  ASSIGN_ARRAY_ITEM(grapplinghAnchorPoints, grapplinghAnchorPointIndex, AnchorPoint, ap);
  ap->pos.x = x;
  ap->pos.y = y;
  ap->isAlive = true;
  grapplinghAnchorPointIndex = cgl_wrap(grapplinghAnchorPointIndex + 1, 0, MAX_ANCHOR_POINTS);
}

void grapplinghUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - anchor point
  // range/grab checks use distanceTo() instead (see the dist checks
  // below), so the engine's own O(n^2) hitbox scan (see checkHitBox() in
  // cglp.c) is pure waste here. Restored automatically when the next
  // real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(grapplinghAnchorPoints);
    grapplinghAnchorPointIndex = 0;
    grapplinghAddAnchorPoint(50, 0);

    grapplinghPlayer.pos.x = 30;
    grapplinghPlayer.pos.y = 30;
    grapplinghPlayer.vel.x = 0;
    grapplinghPlayer.vel.y = 0;
    grapplinghPlayer.attachedAnchor = &grapplinghAnchorPoints[0].pos;
    grapplinghPlayer.isAlive = true;

    grapplinghScrolledDistanceY = 0;
    grapplinghScrolledDistanceYForScore = 0;
    grapplinghInputIsReleased = false;
    grapplinghInputIsJustPressed = false;
  }

  grapplinghInputIsJustPressed = input.isJustPressed;
  float scrollY = 0.1 * difficulty;
  if (grapplinghPlayer.pos.y < 75) {
    scrollY += (75 - grapplinghPlayer.pos.y) * 0.1;
  }
  grapplinghScrolledDistanceY += scrollY;
  grapplinghScrolledDistanceYForScore += scrollY;
  while (grapplinghScrolledDistanceY > grapplinghNextAnchorDistance) {
    grapplinghScrolledDistanceY -= grapplinghNextAnchorDistance;
    grapplinghAddAnchorPoint(rnd(10, 90), -5 + grapplinghScrolledDistanceY);
  }

  AnchorPoint* attachableAnchor = NULL;
  float attachableDist = 25;
  FOR_EACH(grapplinghAnchorPoints, i) {
    ASSIGN_ARRAY_ITEM(grapplinghAnchorPoints, i, AnchorPoint, ap);
    SKIP_IS_NOT_ALIVE(ap);
    float dist = distanceTo(&grapplinghPlayer.pos, ap->pos.x, ap->pos.y);
    if (grapplinghPlayer.attachedAnchor == NULL && dist < attachableDist) {
      attachableAnchor = ap;
      attachableDist = dist;
    }
  }
  FOR_EACH(grapplinghAnchorPoints, i) {
    ASSIGN_ARRAY_ITEM(grapplinghAnchorPoints, i, AnchorPoint, ap);
    SKIP_IS_NOT_ALIVE(ap);
    ap->pos.y += scrollY;
    if (ap == attachableAnchor) {
      color = RED;
    } else {
      color = BLACK;
    }
    arc(ap->pos.x, ap->pos.y, grapplinghAnchorRadius, 0, M_PI * 2, &scratch);
    if (ap == attachableAnchor) {
      color = RED;
      line(grapplinghPlayer.pos.x, grapplinghPlayer.pos.y, ap->pos.x, ap->pos.y, &scratch);
      if (grapplinghInputIsReleased && input.isPressed) {
        play(COIN);
        grapplinghPlayer.attachedAnchor = &ap->pos;
        if (grapplinghScrolledDistanceYForScore > 0) {
          addScore(ceil(sqrt(grapplinghScrolledDistanceYForScore * grapplinghScrolledDistanceYForScore) * 0.1), ap->pos.x, ap->pos.y);
        }
        grapplinghInputIsJustPressed = false;
      }
    }
    ap->isAlive = ap->pos.y <= 155;
  }

  color = BLUE;
  vectorAdd(&grapplinghPlayer.pos, grapplinghPlayer.vel.x, grapplinghPlayer.vel.y);
  vectorMul(&grapplinghPlayer.vel, 1 - fabs(vectorLength(&grapplinghPlayer.vel)) * 0.001);
  grapplinghPlayer.vel.y += 0.1;
  grapplinghPlayer.pos.y += scrollY;
  if ((grapplinghPlayer.pos.x < 0 && grapplinghPlayer.vel.x < 0) || (grapplinghPlayer.pos.x > 100 && grapplinghPlayer.vel.x > 0)) {
    play(HIT);
    grapplinghPlayer.vel.x *= -0.8;
  }
  if (grapplinghPlayer.attachedAnchor != NULL) {
    if (grapplinghPlayer.attachedAnchor->y > 150) {
      play(EXPLOSION);
      gameOver();
    }
    float dist = distanceTo(&grapplinghPlayer.pos, grapplinghPlayer.attachedAnchor->x, grapplinghPlayer.attachedAnchor->y);
    float angle = angleTo(&grapplinghPlayer.pos, grapplinghPlayer.attachedAnchor->x, grapplinghPlayer.attachedAnchor->y);
    float force = dist;
    addWithAngle(&grapplinghPlayer.vel, angle, force * 0.01);
    line(grapplinghPlayer.pos.x, grapplinghPlayer.pos.y, grapplinghPlayer.attachedAnchor->x, grapplinghPlayer.attachedAnchor->y, &scratch);
    if (grapplinghInputIsJustPressed) {
      play(JUMP);
      vectorSet(grapplinghPlayer.attachedAnchor, 0, 999);
      grapplinghPlayer.attachedAnchor = NULL;
      addWithAngle(&grapplinghPlayer.vel, angle, force * 0.2);
      grapplinghScrolledDistanceYForScore = 0;
      grapplinghInputIsReleased = false;
    }
  } else {
    if (grapplinghPlayer.pos.y > 150 && grapplinghPlayer.vel.y > 0) {
      play(EXPLOSION);
      gameOver();
    }
  }
  arc(grapplinghPlayer.pos.x, grapplinghPlayer.pos.y, grapplinghPlayerRadius, 0, M_PI * 2, &scratch);
  if (input.isJustReleased) {
    grapplinghInputIsReleased = true;
  }
}

void addGameGrapplingH() {
  addGame(grapplinghTitle, grapplinghDescription, grapplinghCharacters,
          grapplinghCharactersCount, &grapplinghOptions, false, &grapplinghUpdate);
}
