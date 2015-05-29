/*
 * This file deals with creating the ALSA I/O ports, listening to them, progressing through note vectors, sending signals,
 * and closing aforementioned ports
 */

#include <alsa/asoundlib.h>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include "Obj.h"
#include <pthread.h>

using namespace std;

// global variables
snd_seq_t *handle;
int inPort, outPort, phoneOut;

// thread stuff
vector<pthread_t> inputThreads;
vector<pthread_t> outputThreads;

// forward declarations
void closeALSA();
void *inThreadFunc(void *threadStruct);
void *outThreadFunc(void *threadStruct);
char openALSA();
void playSong(vector<Event> fullArray, int numTracks);

// called by main
// open ALSA Sequencer as well as Input & Output ports
// returns char value based upon success/fail
char openALSA() {

	// open ALSA Sequencer
	if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_DUPLEX, 0) > 0) {
		handle = NULL;
		return 1;
	}

	snd_seq_set_client_name(handle, "Lights Client");

	// open ALSA input port
	if ((inPort = snd_seq_create_simple_port(handle, "Lights In",
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
		closeALSA();
		return 2;
	}

	// open ALSA output port
	if ((outPort = snd_seq_create_simple_port(handle, "Lights Out",
			SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
			SND_SEQ_PORT_TYPE_APPLICATION|SND_SEQ_PORT_TYPE_MIDI_GENERIC)) < 0) {
		closeALSA();
		return 3;
	}

	// open output port for phone
	if ((phoneOut = snd_seq_create_simple_port(handle, "Phone Out",
			SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
			SND_SEQ_PORT_TYPE_APPLICATION|SND_SEQ_PORT_TYPE_MIDI_GENERIC)) < 0) {
		closeALSA();
		return 3;
	}

	// there was no error, so return 0
	return 0;
}

// called by main
// function to play song
// takes vector of all loaded information
void playSong(vector<Event> fullArray, int numTracks) {
	Event tempo;
	vector<Event> mp;

    // sort fullArray into vectors based on channel
    vector<vector<Event> > sortedVector(32, vector<Event>(0));
    for (unsigned int i = 0; i < fullArray.size(); i++) {
    	if (fullArray.at(i).type == META) {
    		tempo = fullArray.at(i);
    	} else if (fullArray.at(i).channel == 32) { // assume MP for now
    		mp.push_back(fullArray.at(i));
    	} else {
    		sortedVector.at(fullArray.at(i).channel).push_back(fullArray.at(i));
    	}
    }

    for (unsigned int i = 0; i < mp.size(); i++) {
    	mp.at(i).type = META;
    }

    // create structs for threads
    vector<ThreadStruct> inputThreadStructs;
    vector<ThreadStruct> outputThreadStructs;

    // populate structs
    for (int i = 0; i < 32; i++) {
    	bool used = false;
    	for (unsigned int j = 0; j < sortedVector.at(i).size(); j++) {
    		if (sortedVector.at(i).at(j).type != META) {
    			used = true;
    			break;
    		}
    	}

    	if (used) {
    		ThreadStruct current;
    		current.notes = sortedVector.at(i);
    		current.mp = mp;
    		current.tempo = tempo;

    		// check for input vs output and append to appropriate vector
    		if (current.notes.at(0).channel < 16) { // input thread
    			inputThreadStructs.push_back(current);
    		} else { // output
    			outputThreadStructs.push_back(current);
    		}
    	}
    }

    // create input threads
    for (unsigned int i = 0; i < inputThreadStructs.size(); i++) {
    	pthread_t current;
    	inputThreads.push_back(current);

    	int threadCreateRet = pthread_create(&inputThreads.at(i), NULL, inThreadFunc, &inputThreadStructs.at(i));

    	if (threadCreateRet != 0) fprintf(stderr, "Error creating input thread %d\n", i);
    }

    // create output threads
    for (unsigned int i = 0; i < outputThreadStructs.size(); i++) {
    	pthread_t current;
    	outputThreads.push_back(current);

    	int threadCreateRet = pthread_create(&outputThreads.at(i), NULL, outThreadFunc, &outputThreadStructs.at(i));

    	if (threadCreateRet != 0) fprintf(stderr, "Error creating output thread %d\n", i);
    }


}

// function for thread to execute for MIDI keyboard in
void *inThreadFunc(void *threadStruct) {
	ThreadStruct *data = (ThreadStruct *)threadStruct;

	printf("Input channel %d ready\n", (int)data->notes.at(0).channel);

	return NULL;
}

// function for thread to execute for MIDI out
void *outThreadFunc(void *threadStruct) {
	ThreadStruct *data = (ThreadStruct *)threadStruct;

	printf("Output channel %d ready\n", (int)data->notes.at(0).channel);

	return NULL;
}

// close ALSA Sequencer
void closeALSA() {
    snd_seq_close(handle);
	handle = NULL;
}
