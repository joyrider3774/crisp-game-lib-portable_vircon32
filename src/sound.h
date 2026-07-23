#ifndef SOUND_H
#define SOUND_H

extern bool isSoundEnabled;
extern bool isPlayingBgm;
void initSound(int* title, int* description, int soundSeed);
void updateSound();
void playSoundEffect(int type);

#endif
