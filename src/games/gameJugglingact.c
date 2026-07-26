#include "../cglp.h"

int* jugglingactTitle = "JUGGLING ACT";
int* jugglingactDescription = "[Hold] Throw ball back\n[Release] Move";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] jugglingactCharacters = {{
    "  lll ",
    "  lll ",
    "  ll  ",
    "  ll  ",
    " llll ",
    "ll  ll",
}};
int jugglingactCharactersCount = 1;

Options jugglingactOptions = {100, 100, 1, false};

#define JUGGLINGACT_STATE_READY 0
#define JUGGLINGACT_STATE_CATCH 1
#define JUGGLINGACT_BALL_FALLING 0
#define JUGGLINGACT_BALL_RISING 1
#define JUGGLINGACT_GRAVITY 0.02
#define JUGGLINGACT_THROW_VELOCITY 2
#define JUGGLINGACT_JUGGLER_VELOCITY 0.36

struct JugglingactJuggler {
  Vector pos;
  float velX;
  int state;
};
JugglingactJuggler jugglingactJuggler;

struct JugglingactBall {
  Vector pos;
  Vector vel;
  int state;
  bool isAlive;
};
// Balls only die by falling past the juggler (a missed catch); perfect play
// never removes any while the spawn timer keeps firing every ~200/sqrt(
// difficulty) ticks regardless of how many are already alive, so count grows
// unbounded over a long session (~16 reached within about a minute).
#define JUGGLINGACT_MAX_BALL_COUNT 512
JugglingactBall[JUGGLINGACT_MAX_BALL_COUNT] jugglingactBalls;
int jugglingactBallIndex;
float jugglingactNextBallSpawnTime;

int jugglingactFindNearestBallIndex() {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(jugglingactBalls, i) {
    ASSIGN_ARRAY_ITEM(jugglingactBalls, i, JugglingactBall, ball);
    SKIP_IS_NOT_ALIVE(ball);
    if (ball->state != JUGGLINGACT_BALL_FALLING) {
      continue;
    }
    float distance = fabs(ball->pos.y - jugglingactJuggler.pos.y);
    if (best < 0 || distance < bestDist) {
      best = i;
      bestDist = distance;
    }
  }
  return best;
}

void jugglingactUpdate() {
  Collision scratch;
  // Never reads a Collision result anywhere in this file - catching is a
  // direct y-band + fabs() distance check against the juggler's position
  // (see "ball->pos.y > jugglingactJuggler.pos.y - 8" below), so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&jugglingactJuggler.pos, 50, 90);
    jugglingactJuggler.velX = 0;
    jugglingactJuggler.state = JUGGLINGACT_STATE_READY;
    INIT_UNALIVED_ARRAY_FAST(jugglingactBalls);
    jugglingactBallIndex = 0;
    jugglingactNextBallSpawnTime = 0;
  }
  float sd = sqrt(difficulty);
  if (input.isJustPressed) {
    play(SELECT);
    jugglingactJuggler.state = JUGGLINGACT_STATE_CATCH;
  } else if (input.isJustReleased) {
    jugglingactJuggler.state = JUGGLINGACT_STATE_READY;
  }
  if (!input.isPressed) {
    int nbi = jugglingactFindNearestBallIndex();
    if (nbi >= 0) {
      JugglingactBall* nearestBall = &jugglingactBalls[nbi];
      float moveDirection;
      if (nearestBall->pos.x > jugglingactJuggler.pos.x) {
        moveDirection = 1;
      } else {
        moveDirection = -1;
      }
      jugglingactJuggler.velX += moveDirection * JUGGLINGACT_JUGGLER_VELOCITY;
    }
  }
  jugglingactJuggler.velX *= 0.85;
  jugglingactJuggler.pos.x += jugglingactJuggler.velX * sd;
  if ((jugglingactJuggler.pos.x < 5 && jugglingactJuggler.velX < 0) ||
      (jugglingactJuggler.pos.x > 95 && jugglingactJuggler.velX > 0)) {
    jugglingactJuggler.velX *= -0.5;
  }
  color = BLUE;
  float handOffset;
  if (jugglingactJuggler.state == JUGGLINGACT_STATE_CATCH) {
    handOffset = -5;
  } else {
    handOffset = 0;
  }
  box(jugglingactJuggler.pos.x - 5, jugglingactJuggler.pos.y + handOffset, 3, 3, &scratch);
  box(jugglingactJuggler.pos.x + 5, jugglingactJuggler.pos.y + handOffset, 3, 3, &scratch);
  if (jugglingactJuggler.velX > 0) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", jugglingactJuggler.pos.x, jugglingactJuggler.pos.y, &scratch);
  color = RED;
  FOR_EACH(jugglingactBalls, i) {
    ASSIGN_ARRAY_ITEM(jugglingactBalls, i, JugglingactBall, ball);
    SKIP_IS_NOT_ALIVE(ball);
    vectorAdd(&ball->pos, ball->vel.x * sd, ball->vel.y * sd);
    vectorMul(&ball->vel, 0.99);
    if ((ball->pos.x < 0 && ball->vel.x < 0) || (ball->pos.x > 100 && ball->vel.x > 0)) {
      ball->vel.x *= -1;
    }
    if (ball->state == JUGGLINGACT_BALL_FALLING) {
      ball->vel.y += JUGGLINGACT_GRAVITY * sd;
      if (jugglingactJuggler.state == JUGGLINGACT_STATE_CATCH &&
          ball->pos.y > jugglingactJuggler.pos.y - 8 && ball->pos.y < jugglingactJuggler.pos.y) {
        if (fabs(ball->pos.x - jugglingactJuggler.pos.x) < 10) {
          play(JUMP);
          ball->state = JUGGLINGACT_BALL_RISING;
          ball->vel.y = -JUGGLINGACT_THROW_VELOCITY;
          ball->vel.x = (ball->pos.x - jugglingactJuggler.pos.x) / 10 + rnd(0, 0.1) * RNDPM();
          COUNT_IS_ALIVE(jugglingactBalls, aliveBallCount);
          addScore(aliveBallCount, ball->pos.x, ball->pos.y);
        }
      }
    } else {
      ball->vel.y += JUGGLINGACT_GRAVITY * sd;
      if (ball->vel.y >= 0) {
        ball->state = JUGGLINGACT_BALL_FALLING;
      }
    }
    thickness = 3;
    arc(ball->pos.x, ball->pos.y, 2, 0, CGLP_PI * 2, &scratch);
    if (ball->pos.y > 100) {
      play(CLICK);
      ball->isAlive = false;
      continue;
    }
  }
  jugglingactNextBallSpawnTime -= sd;
  if (jugglingactNextBallSpawnTime <= 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(jugglingactBalls, jugglingactBallIndex, JugglingactBall, nb);
    vectorSet(&nb->pos, rnd(10, 90), 0);
    vectorSet(&nb->vel, 0, 0);
    nb->state = JUGGLINGACT_BALL_FALLING;
    nb->isAlive = true;
    jugglingactBallIndex = cgl_wrap(jugglingactBallIndex + 1, 0, JUGGLINGACT_MAX_BALL_COUNT);
    jugglingactNextBallSpawnTime = 200;
  }
  COUNT_IS_ALIVE(jugglingactBalls, aliveBallCount2);
  if (aliveBallCount2 == 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJugglingact() {
  addGame(jugglingactTitle, jugglingactDescription, jugglingactCharacters,
          jugglingactCharactersCount, &jugglingactOptions, false,
          &jugglingactUpdate);
}
