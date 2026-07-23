#include "../cglp.h"

char* divarrTitle = "DIVARR";
char* divarrDescription = "[Tap]\n Fire / Split";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] divarrCharacters = {
    {
        "yy    ",
        " yy   ",
        "llllll",
        "llllll",
        " yy   ",
        "yy    ",
    },
    {
        "  rr  ",
        "  ll  ",
        " llll ",
        "  ll  ",
        "y ll y",
        "yyyyyy",
    },
    {
        "   rr ",
        "  p   ",
        " llll ",
        "ll lll",
        "l llll",
        " llll ",
    },
    {
        "      ",
        "  yy  ",
        " yggy ",
        "  bb  ",
        " y  y ",
        "      ",
    }};
int divarrCharactersCount = 4;

Options divarrOptions = {100, 100, 3, false};

struct DivarrShot {
  Vector pos;
  int angle;
  int cc;  // Collision counter
  bool isAlive;
};

#define DIVARR_MAX_SHOT_COUNT 32
DivarrShot[DIVARR_MAX_SHOT_COUNT] divarrShots;
int divarrShotIndex;

struct DivarrFalling {
  Vector pos;
  Vector vel;
  bool isHuman;
  bool isAlive;
};

#define DIVARR_MAX_FALLING_COUNT 64
DivarrFalling[DIVARR_MAX_FALLING_COUNT] divarrFallings;
int divarrFallingIndex;

int divarrFallingAddingTicks;
int divarrHumanAddingCount;
int divarrFirstHumanCount;

void divarrUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(divarrShots);
    divarrShotIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(divarrFallings);
    divarrFallingIndex = 0;
    divarrFallingAddingTicks = 0;
    divarrHumanAddingCount = 3;
    divarrFirstHumanCount = 120;
  }

  float df = sqrt(difficulty);
  rect(0, 90, 99, 9, &scratch);
  characterOptions.rotation = 0;
  character("b", 50, 87, &scratch);

  if (input.isJustPressed) {
    COUNT_IS_ALIVE(divarrShots, sc);
    if (sc == 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(divarrShots, divarrShotIndex, DivarrShot, s);
      s->pos.x = 50;
      s->pos.y = 85;
      s->angle = 0;
      s->cc = 0;
      s->isAlive = true;
      divarrShotIndex = cgl_wrap(divarrShotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
    } else {
      play(SELECT);
      COUNT_IS_ALIVE(divarrShots, sc2);

      bool[DIVARR_MAX_SHOT_COUNT] splitShot;
      memset(splitShot, false, sizeof(splitShot));
      int splitCount = 0;

      FOR_EACH(divarrShots, i) {
        ASSIGN_ARRAY_ITEM(divarrShots, i, DivarrShot, s);
        if (s->isAlive) {
          splitShot[i] = true;
          splitCount++;
        }
      }

      FOR_EACH(divarrShots, i) {
        ASSIGN_ARRAY_ITEM(divarrShots, i, DivarrShot, s);
        if (!splitShot[i]) continue;

        Vector pos = s->pos;
        int angle = s->angle;

        s->isAlive = false;

        if (sc2 < 16) {
          ASSIGN_ARRAY_ITEM(divarrShots, divarrShotIndex, DivarrShot, ns1);
          vectorSet(&ns1->pos, pos.x, pos.y);
          ns1->angle = cgl_wrap(angle - 1, 0, 4);
          ns1->cc = 0;
          ns1->isAlive = true;
          divarrShotIndex = cgl_wrap(divarrShotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);

          ASSIGN_ARRAY_ITEM(divarrShots, divarrShotIndex, DivarrShot, ns2);
          vectorSet(&ns2->pos, pos.x, pos.y);
          ns2->angle = cgl_wrap(angle + 1, 0, 4);
          ns2->cc = 0;
          ns2->isAlive = true;
          divarrShotIndex = cgl_wrap(divarrShotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
        } else {
          ASSIGN_ARRAY_ITEM(divarrShots, divarrShotIndex, DivarrShot, ns);
          vectorSet(&ns->pos, pos.x, pos.y);
          ns->angle = cgl_wrap(angle + 1, 0, 4);
          ns->cc = 0;
          ns->isAlive = true;
          divarrShotIndex = cgl_wrap(divarrShotIndex + 1, 0, DIVARR_MAX_SHOT_COUNT);
        }
      }
    }
  }

  // Update shots
  FOR_EACH(divarrShots, i) {
    ASSIGN_ARRAY_ITEM(divarrShots, i, DivarrShot, s);
    SKIP_IS_NOT_ALIVE(s);

    // Direction vectors: 0=up, 1=right, 2=down, 3=left
    Vector[4] w = {
      {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };

    vectorAdd(&s->pos, w[s->angle].x * df, w[s->angle].y * df);

    if (s->pos.x < -2 || s->pos.y < -2 ||
        s->pos.x > 104 || s->pos.y > 90) {
      s->isAlive = false;
      continue;
    }

    characterOptions.rotation = (s->angle + 3) % 4;
    Collision coll;
    character("a", s->pos.x, s->pos.y, &coll);
    characterOptions.rotation = 0;
    if (coll.isColliding.character['a']) {
      s->cc++;
      if (s->cc > 8) {
        s->isAlive = false;
        continue;
      }
    } else {
      s->cc = 0;
    }
  }

  // Update fallings
  divarrFallingAddingTicks--;
  if (divarrFallingAddingTicks < 0) {
    divarrHumanAddingCount--;
    ASSIGN_ARRAY_ITEM(divarrFallings, divarrFallingIndex, DivarrFalling, f);
    f->pos.x = rnd(9, 90);
    f->pos.y = -3;
    vectorSet(&f->vel, 0, 0);
    f->isHuman = divarrHumanAddingCount < 0;
    f->isAlive = true;

    if (f->isHuman && divarrFirstHumanCount > 0) {
      f->pos.x = 25;
    }

    Vector target;
    target.x = clamp(f->pos.x + rnd(-clamp(df * 10, 0, 50), clamp(df * 10, 0, 50)), 9, 90);
    target.y = 90;

    float angle = angleTo(&f->pos, target.x, target.y);
    float speed = (1 + rnd(0, df)) * 0.1;
    if (f->isHuman) speed *= 1.6;

    addWithAngle(&f->vel, angle, speed);

    divarrFallingIndex = cgl_wrap(divarrFallingIndex + 1, 0, DIVARR_MAX_FALLING_COUNT);

    if (divarrHumanAddingCount < 0) {
      divarrHumanAddingCount = rndi(3, 15);
    }

    divarrFallingAddingTicks += rnd(60, 80) / difficulty;
  }

  FOR_EACH(divarrFallings, i) {
    ASSIGN_ARRAY_ITEM(divarrFallings, i, DivarrFalling, f);
    SKIP_IS_NOT_ALIVE(f);

    vectorAdd(&f->pos, f->vel.x, f->vel.y);

    if (f->isHuman && (divarrFirstHumanCount > 0 || f->pos.y < 8)) {
      divarrFirstHumanCount--;
      text("Don't", f->pos.x - 10, f->pos.y + 6, &scratch);
      text("hit me !", f->pos.x - 15, f->pos.y + 12, &scratch);
    }

    if (f->isHuman && f->pos.y < 20) {
      color = LIGHT_BLACK;
      character("d", f->pos.x, f->pos.y, &scratch);
      color = BLACK;
    } else {
      Collision coll;
      if (f->isHuman) {
        character("d", f->pos.x, f->pos.y, &coll);
      } else {
        character("c", f->pos.x, f->pos.y, &coll);
      }
      if (coll.isColliding.character['a']) {
        if (f->isHuman) {
          play(RANDOM);
          color = RED;
          text("X", f->pos.x, f->pos.y, &scratch);
          color = BLACK;
          gameOver();
        } else {
          play(HIT);
          color = RED;
          particle(f->pos.x, f->pos.y, 20, 2, 0, M_PI * 2);
          color = BLACK;
          COUNT_IS_ALIVE(divarrShots, sc3);
          addScore(sc3, f->pos.x, f->pos.y);
        }
        f->isAlive = false;
        continue;
      }
    }

    if (f->pos.y > 88) {
      if (f->isHuman) {
        play(POWER_UP);
        addScore(100, f->pos.x, f->pos.y);
      } else {
        play(EXPLOSION);
        color = RED;
        text("X", f->pos.x, f->pos.y, &scratch);
        color = BLACK;
        gameOver();
      }
      f->isAlive = false;
      continue;
    }
  }
}

void addGameDivarr() {
  addGame(divarrTitle, divarrDescription, divarrCharacters, divarrCharactersCount,
          &divarrOptions, false, &divarrUpdate);
}
