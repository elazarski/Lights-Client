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
vector<pthread_t *> inputThreads;
vector<pthread_t *> outputThreads;

// forward declarations
void closeALSA();
void *inThreadFunc(void *vect);
void *outThreadFunc(void *vect);
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

    // create threads with appropriate vectors
    for (int i = 0; i < 32; i++) {
    	// check if current channel is used
    	bool used = false;
    	for (unsigned int j = 0; j < sortedVector.at(i).size(); j++) {
    		if (sortedVector.at(i).at(j).type != META) {
    			used = true;
    			break;
    		}
    	}

    	if (used) {

    		// create vector for this channel
    		vector<vector<Event> > currentChannel;
    		currentChannel.push_back(sortedVector.at(i));
    		currentChannel.push_back(mp);
    		currentChannel.push_back(vector<Event> (1, tempo));


    		// append new pthread_t to appropriate vector and start it
    		pthread_t *currentThread;
    		int threadCreateError = 0;

    		if (currentChannel.at(0).at(0).channel < 16) { // input thread
    			inputThreads.push_back(currentThread);
    			threadCreateError = pthread_create(currentThread, NULL, inThreadFunc, (void *)&currentChannel);
    		} else {
    			outputThreads.push_back(currentThread);
    			threadCreateError = pthread_create(currentThread, NULL, outThreadFunc, (void *)&currentChannel);
    		}


    		if (threadCreateError != 0) {
    			fprintf(stderr, "THREAD CREATE ERROR!");
    			return;
    		}
    	}
    }
}

// function for thread to execute for MIDI keyboard in
void *inThreadFunc(void *vect) {
	printf("Input Thread\n");

	return 0;
}

// function for thread to execute for MIDI out
void *outThreadFunc(void *vect) {
	printf("Output Thread\n");

	return 0;
}

// close ALSA Sequencer
void closeALSA() {
    snd_seq_close(handle);
	handle = NULL;
}
