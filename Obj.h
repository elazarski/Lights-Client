#ifndef OBJ_H_INCLUDED
#define OBJ_H_INCLUDED

using namespace std;

enum Type {NOTE, REST, META};

#ifndef EVENT_H
#define EVENT_H

typedef struct Event{
	char type;
	char num;
	float time;
	char channel;
} EVENT;

#endif

// helps with blocking threads
static enum { IN, OUT } status = OUT;

#endif // OBJ_H_INCLUDED
