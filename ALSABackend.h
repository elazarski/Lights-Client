#ifndef ALSABACKEND_H_INCLUDED
#define ALSABACKEND_H_INCLUDED

#include <alsa/asoundlib.h>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include "Obj.h"

using namespace std;

void closeALSA();
char openALSA();
void playSong(vector<Event> fullArray, int numInputTracks, int numOutputTracks);

#endif // ALSABACKEND_H_INCLUDED
