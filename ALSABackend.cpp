/*
 * This file deals with creating the ALSA I/O ports, listening to them, progressing through note vectors, sending signals,
 * and closing aforementioned ports
 */

#include <alsa/asoundlib.h>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include <pthread.h>
#include <cmath>
#include <queue>
#include "Obj.h"

using namespace std;

// global variables

// ALSA
snd_seq_t *handle;
int inPort, outPort, phoneOut;

// Thread stuff
pthread_mutex_t inputDataLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t outputDataLock = PTHREAD_MUTEX_INITIALIZER;

ThreadStruct inputData;
ThreadStruct outputData;

vector<bool> inputReady;
vector<bool> outputReady;

vector<queue<Event> > inputEvents;

// forward declarations
void closeALSA();
void *inThreadFunc(void *channel);
void *outThreadFunc(void *channel);
char openALSA();
void playSong(vector<Event> fullArray, int numInputTracks, int numOutputTracks);

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
void playSong(vector<Event> fullArray, int numInputTracks, int numOutputTracks) {

	inputData.notes = vector<vector<Event> >(numInputTracks, vector<Event>(0));
	outputData.notes = vector<vector<Event> >(numOutputTracks, vector<Event>(0));

	inputEvents = vector<queue<Event> >(numInputTracks);

    // sort fullArray
    for (unsigned int i = 0; i < fullArray.size(); i++) {
    	if (fullArray.at(i).type == META) { // tempo Event
    		inputData.tempo = fullArray.at(i);
    		outputData.tempo = fullArray.at(i);
    	} else if (fullArray.at(i).channel == 32) { // measures & parts track
    		fullArray.at(i).type = META;
    		inputData.mp.push_back(fullArray.at(i));
    		outputData.mp.push_back(fullArray.at(i));
    	} else { // either input or output
    		if (fullArray.at(i).channel < 16) { // input
    			inputData.notes.at(fullArray.at(i).channel).push_back(fullArray.at(i));
    		} else { // output
    			fullArray.at(i).channel = abs(fullArray.at(i).channel - 16);
    			outputData.notes.at(fullArray.at(i).channel).push_back(fullArray.at(i));
    		}
    	}
    }

    vector<pthread_t> inputThreads(numInputTracks);
    vector<pthread_t> outputThreads(numOutputTracks);

    // create input threads
    for (int i = 0; i < numInputTracks; i++) {
    	int *track = new int;
    	*track = i;
    	pthread_t current;
    	inputThreads.at(i) = current;

    	inputReady.push_back(false);

    	int ret = pthread_create(&inputThreads.at(i), NULL, inThreadFunc, (void *)track);
    	if (ret != 0) fprintf(stderr, "Error creating input thread %d\n", i);

    }

    // create output threads
    for (int i = 0; i < numOutputTracks; i++) {
    	int *track = new int;
    	*track = i;
    	pthread_t current;
    	outputThreads.at(i) = current;

    	outputReady.push_back(false);

    	int ret = pthread_create(&outputThreads.at(i), NULL, outThreadFunc, (void *)track);
    	if (ret != 0) fprintf(stderr, "Error creating output thread %d\n", i);
    }

    bool ready = false;
    while (!ready) {
    	ready = true;

    	// check input threads
    	pthread_mutex_lock(&inputDataLock);
    	for (int i = 0; i < numInputTracks; i++) {
    		if (inputReady.at(i) == false) ready = false;
    	}
    	pthread_mutex_unlock(&inputDataLock);

    	// check output threads
    	pthread_mutex_lock(&outputDataLock);
    	for (int i = 0; i < numOutputTracks; i++) {
    		if (outputReady.at(i) == false) ready = false;
    	}
    	pthread_mutex_unlock(&outputDataLock);
    }

    printf("All threads ready!\n");

    // reset ready vectors, use these to wake output threads
    fill(inputReady.begin(), inputReady.end(), false);
    fill(outputReady.begin(), outputReady.end(), false);


}

// function for thread to execute for MIDI keyboard in
void *inThreadFunc(void *channel) {
	int *convertedPTR = (int *)channel;
	int track = *convertedPTR;


	// lock input dataLock
	pthread_mutex_lock(&inputDataLock);
	vector<Event> notes;
	vector<Event> mp;
	Event tempo;

	for (unsigned int i = 0; i < inputData.notes.at(track).size(); i++) {
		notes.push_back(inputData.notes.at(track).at(i));
	}

	for (unsigned int i = 0; i < inputData.mp.size(); i++) {
		mp.push_back(inputData.mp.at(i));
	}

	tempo = inputData.tempo;

	inputReady.at(track) = true;

	// unlock dataLock
	pthread_mutex_unlock(&inputDataLock);
	printf("Input channel %d ready\n", track);


	return NULL;
}

// function for thread to execute for MIDI out
void *outThreadFunc(void *channel) {
	int *convertedPTR = (int *)channel;
	int track = *convertedPTR;

	// lock output datalock
	pthread_mutex_lock(&outputDataLock);
	vector<Event> signals;
	vector<Event> mp;
	Event tempo;

	for (unsigned int i = 0; i < outputData.notes.at(track).size(); i++) {
		signals.push_back(outputData.notes.at(track).at(i));
	}

	for (unsigned int i = 0; i < outputData.mp.size(); i++) {
		mp.push_back(outputData.notes.at(track).at(i));
	}

	tempo = outputData.tempo;

	outputReady.at(track) = true;

	// unlock output datalock
	pthread_mutex_unlock(&outputDataLock);
	printf("Output channel %d ready\n", track);

	return NULL;
}

// close ALSA Sequencer
void closeALSA() {
    snd_seq_close(handle);
	handle = NULL;
}
