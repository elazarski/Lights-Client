#ifndef READFILES_H_INCLUDED
#define READFILES_H_INCLUDED

#include <fstream>
#include <vector>
#include <string.h>
#include "Obj.h"

using namespace std;

vector<Event> readFiles(string songPath, int *numInputTracks, int *numOutputTracks);

#endif // READFILES_H_INCLUDED
