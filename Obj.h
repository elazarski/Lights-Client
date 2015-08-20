#ifndef OBJ_H_INCLUDED
#define OBJ_H_INCLUDED

using namespace std;

enum Type {NOTE, REST, META, CHORD, TIE};

#ifndef EVENT_H
#define EVENT_H

typedef struct Event {
	char type;
	char num;
	float time;
	char channel;
} EVENT;

#endif

#ifndef THREADSTRUCT_H
#define THREADSTRUCT_H

typedef struct ThreadStruct {
	vector<vector<Event> > notes;
	vector<Event> mp;
	Event tempo;
} THREADSTRUCT;

#endif

#endif // OBJ_H_INCLUDED
