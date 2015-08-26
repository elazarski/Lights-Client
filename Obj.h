#ifndef OBJ_H_INCLUDED
#define OBJ_H_INCLUDED

using namespace std;

enum Type {NOTE, REST, META, CHORD, TIE};

#ifndef EVENT_H
#define EVENT_H
typedef struct Event {
	char type;
	char note;
	float time;
} EVENT;
#endif

#ifndef MEASURE_H
#define MEASURE_H
typedef struct Measure {
	vector<Event> voiceOne;
	vector<Event> voiceTwo;
} MEASURE;
#endif

#ifndef TRACK_H
#define TRACK_H
typedef struct Track {
	int channel;
	vector<Measure> measures;
} TRACK;
#endif

#ifndef MP_H
#define MP_H
typedef struct MP {
	vector<int> measures;
	vector<int> parts;
} MP;
#endif

#ifndef SONG_H
#define SONG_H
typedef struct Song {
	int tempo;
	vector<Track> tracks;
	MP mp;
} SONG;
#endif

#ifndef THREADSTRUCT_H
#define THREADSTRUCT_H
typedef struct ThreadStruct {
	Track data;
	Track mp;
} THREADSTRUCT;
#endif

#endif // OBJ_H_INCLUDED
