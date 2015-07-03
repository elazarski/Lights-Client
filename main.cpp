
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "UserInterface.h"
#include "ALSABackend.h"
#include "ReadFiles.h"
#include "Obj.h"

using namespace std;

// main
int main(int argc, char *argv[]) {

	char ALSAerr = openALSA();

	switch (ALSAerr) {
	case 0:
		printf("ALSA Sequencer successfully opened\n");
		break;
	case 1:
		printf("Error opening ALSA Sequencer\n");
		printf("Exiting...\n");
		exit(1);
		break;
	case 2:
		printf("Error creating ALSA input port\n");
		printf("Exiting\n");
		closeALSA();
		exit(1);
		break;
	case 3:
		printf("Error creating ALSA output port\n");
		printf("Exiting...\n");
		closeALSA();
		exit(1);
		break;
	default:
		printf("Unexpected value received when opening ALSA Sequencer. Program might crash, but continuing anyways\n");
		break;
	}

	// get user input for current song to run
	string songPath;
	char play = 0;
	do {
		// get user input
		play = (getInput(&songPath));

		if (play == 1) { // 1 means that user entered correct song and wants to play a song

			int numInputTracks, numOutputTracks;
			vector<Event> fullArray = readFiles(songPath, &numInputTracks, &numOutputTracks);

			// check for errors
			if (fullArray.at(0).num == -1) {
				printf("There was an error reading the data file\n");
			} else if (fullArray.at(0).num == -2) {
				printf("Failed to parse XML file\n");
			} else if (fullArray.at(0).num == -3) {
				printf("Unable to open XML file\n");
			} else {
				printf("Song loaded\n");
				playSong(fullArray, numInputTracks, numOutputTracks);
			}
		}
	} while (play != 0); // getInput returns 0 if user entered blank line (to quit)

	printf("Exiting...\n");

	// close ALSA sequencer now that we are done
	closeALSA();

	printf("Program closed\n");

	// eventually record error here and save to a file before exiting
	return 0;
}

/*
int playSonga(string name) {

	// time struct: for error
	struct timeval tv;
	unsigned long long lastTime = 0;

	/* declare index variables
	 * these numbers will be incremented whenever a correct note is played
	 * or a correct note is understood after factoring for error


	// index of vector during play for notes
	int n = 0;

	// index of vector during play for parts;
	int p = 0;

	// index of current chord
	int c = 0;

	// index of current measure
	int m = 0;

	// index of current note in chord
	int numInChord = 0;

	// index of current signal
	int s = 0;

	// declare variables that will be used to read from the files
	int num;
	ifstream file;
	string fileStr;

	// open 1st file
	// this file determines how many notes there are in the song
	fileStr = name+"/nn";
	file.open(fileStr.c_str());
	checkValidityFile(&file);

	// read from 1st file (number of notes) into num
	file >> num;
	file.close();

	// vector of notes in song - array and int* would not work
	// size is 'num'
	vector<int> notes(num);

	// read from 2nd file (note nums) into array (notes)
	fileStr = name+"/n";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	for (int i = 0; i < num; i++) {
		file >> notes.at(i);
	}
	file.close();

	// read from 3rd file (number of parts)
	// rewrite num because of no need to declare another variable
	fileStr = name+"/np";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	file >> num;
	file.close();

	// vector for parts indices
	vector<int> parts(num);

	// read from 4th file (index of where parts change)
	// when the next index is reached a note will be sent to QLC+ to change
	// the light pattern
	fileStr = "./Lights Files/"+name+"/p";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	for (int i = 0; i < num; i++) {
		file >> parts.at(i);
	}
	file.close();

	// read from 5th file (number of chords)
	fileStr = name+"/nchords";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	file >> num;
	file.close();

	// read from 6th file (indices of chords)
	/* 22 25 50 54
	 * 22 is the beginning index of a chord in the 'n' file
	 * 25 is the end of that chord
	 * 50 begins the next chord
	 * 54 is the end of that chord
	 *
	 * will have to buffer all the notes in the chord and then check to see if
	 * the recieved note is one of them

	fileStr = name+"/chords";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	vector<int> v(2);
	vector<vector<int> > chords(num, v);
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 2; j++) {
			file >> chords.at(i).at(j);
		}
	}
	file.close();

	// read number of measures
	fileStr = name+"/nm";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	file >> num;
	file.close();

	// read measure indecies into vector
	vector<int> measures(num);
	fileStr = name+"/m";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	for (int i = 0; i < num; i++) {
		file >> measures.at(i);
	}
	file.close();

	// read number of signals
	fileStr = name+"/numsig";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	file >> num;
	file.close();

	// 2d vector for signals - structure is the same for that of chords
	vector<vector<int> > sig(num, v);
	fileStr = name+"/locsigind";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 2; j++) {
			file >> sig.at(i).at(j);
		}
	}
	file.close();

	// a vector to tie signals to notes
	fileStr = name+"/sigind";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	vector<int> tie(num);
	for (int i = 0; i < num; i++) {
		file >> tie.at(i);
	}
	file.close();

	// read number of ALSA notes to be sent
	fileStr = name+"/nsig";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	file >> num;
	file.close();

	// read note nums for signals into vector
	fileStr = name+"/sig";
	file.open(fileStr.c_str());
	checkValidityFile(&file);
	vector<int> ALSAsig(num);
	for (int i = 0; i < num; i++) {
		file >> ALSAsig.at(i);
	}
	file.close();

	// number of signals to start the song
	fileStr = name+"/nbeginning";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	file >> num;
	file.close();

	// vector of signals to start off song & set the mood
	fileStr = name+"/beginning";
	file.open(fileStr.c_str());
	checkValidityFile (&file);
	vector<int> startSong(num);
	for (int i = 0; i < num; i++) {
		file >> startSong.at(i);
	}
	file.close();

	// code from internet on how to read notes from ALSA queue
	int npfd = snd_seq_poll_descriptors_count(handle, POLLIN);
	struct pollfd *pfd =
		(struct pollfd*)alloca(npfd * sizeof(struct pollfd));

	snd_seq_poll_descriptors(handle, pfd, npfd, POLLIN);


	// run until closed
	while (1) {
		if (poll(pfd, npfd, 100000) > 0) {

			// run until queue is empty
			do {
				snd_seq_event_input(handle, &ev);

				// channel 0 ought to be the big keyboard
				if ((ev->data.control.channel == 0) &&
					(ev->type == SND_SEQ_EVENT_NOTEON)) {

					// use note data to progress through vectors
					// of notes & parts & chords

					// if we are in a chord
					if ((c<chords.size()) &&
					    (n >= chords[c][0]) && (n <= chords[c][1])) {
						vector<int> curChord(chords[c][1] - chords[c][0] + 1);

						// populate curChord
						for (int i = 0; i < curChord.size(); i++) {
							curChord.at(i) = notes.at(chords[c][0] + i);
						}

						// loop through curChord & check if current note is
						// in the chord
						for (int i = 0; i < curChord.size(); i++) {

							// if note-2 < played note < note+2
							if ((static_cast<int>(ev->data.note.note)
									> curChord.at(i)-2) &
									(static_cast<int>(ev->data.note.note)
									< curChord.at(i)+2)) {

								// send signal to QLC++
								// if we are at the right spot
								if (n == tie.at(s)) {

									// gather notes to be generated & sent
									vector<int> sigs(sig[s][1] - sig[s][0]);
									for (int i = 0; i < sigs.size(); i++) {
										sigs.at(i) = ALSAsig.at(sig[c][0] + i);
									}

									// create MIDI note(s) and output
									for (int i = 0; i < sigs.size(); i++) {
										sendNote(sigs.at(i));
									}

									// move on to the next part
									s++;
								}

								n++;

								// new measure
								if ((m < measures.size()) && (n == measures.at(m))) {
									// act accordingly
									m++;
								}

								// new part
								if ((p < parts.size()) && (n == parts.at(p))) {
									// act accordingly
									p++;
								}

								numInChord++;
								if (numInChord == curChord.size()) {
									c++;
									numInChord = 0;
								}

								// exit chord loop
								break;
							}
						}   // normal notes
					} else if ((static_cast<int>(ev->data.note.note)
						       > notes.at(n)-2) &
					           (static_cast<int>(ev->data.note.note)
					           < notes.at(n)+2)) {

						// get timestamp for note (for error)
						gettimeofday(&tv, NULL);

						unsigned long long currentTime =
							static_cast<unsigned long long>(tv.tv_sec)*1000 +
							static_cast<unsigned long long>(tv.tv_usec)/1000;

						if ((currentTime - lastTime) >= 128) {
							// send signal to QLC++
							// if we are at the right spot
							if (n == tie.at(s)) {

								// gather notes to be generated & sent
								vector<int> sigs(sig[s][1] - sig[s][0]);
								for (int i = 0; i < sigs.size(); i++) {
									sigs.at(i) = ALSAsig.at(sig[c][0] + i);
								}

								// create MIDI note(s) and output
								for (int i = 0; i < sigs.size(); i++) {
									sendNote(sigs.at(i));
								}

								// move on to the next part
								s++;
							}

							n++;

							// new measure
							if ((m < measures.size()) && (n == measures.at(m))) {
								// act accordingly
								m++;
							}

							// new part
							if ((p < parts.size()) && (n == parts.at(p))) {
								// act accordingly
								p++;
							}
						}

						// the last time a note was played is the current time
						lastTime = currentTime;
					}
				} else if ((ev->data.control.channel == 1) &&
				    (ev->type == SND_SEQ_EVENT_NOTEON)) {
					switch (static_cast<int>(ev->data.note.note)) {
						case 0: // start song
							/* send first note to QLC++
							 * this first note for QLC++ will start a beginning
							 * sequence for the song


							for (int i = 0; i < startSong.size(); i++) {
								sendNote(startSong.at(i));
							}
							break;

						case 1: // get back on @ next 'measure' if we got off
							/* move p to the next 'measure of the song
							 * this is for if the band gets off and we need to
							 * get back on
							 * conscious error on keyboard


							// next measure
							m++;
							n = measures.at(m);

							// next part (if it happened)
							while (parts.at(p + 1) < measures.at(m)) {
								p++;
							}

							// send signals if needed
							if (n > tie.at(s+1)) { // signal is needed, send most recent

								// gather notes to be generated & sent
								vector<int> sigs(sig[s][1] - sig[s][0]);
								for (int i = 0; i < sigs.size(); i++) {
									sigs.at(i) = ALSAsig.at(sig[c][0] + i);
								}

								// create MIDI note(s) and output
								for (int i = 0; i < sigs.size(); i++) {
									sendNote(sigs.at(i));
								}

								// move on to the next part
								s++;
							}

							break;

						case 2: // get back on @ next part
							/* for when we are very much so out of sync
							 * will move p to the next part
							 * keys will start again @ next part and will be
							 * synchronized again


							// next part
							p++;
							n = parts.at(p);

							// appropriate measure
							while (measures.at(m + 1) < parts.at(p)) {
								m++;
							}

							// send signals if needed
							if (n > tie.at(s+1)) { // signal is needed, send most recent

								// gather notes to be generated & sent
								vector<int> sigs(sig[s][1] - sig[s][0]);
								for (int i = 0; i < sigs.size(); i++) {
									sigs.at(i) = ALSAsig.at(sig[c][0] + i);
								}

								// create MIDI note(s) and output
								for (int i = 0; i < sigs.size(); i++) {
									sendNote(sigs.at(i));
								}

								// move on to the next part
								s++;
							}

							break;

						case 3: // end song
							/* for when keys do not end song
							 * also used for when a song repeats the end until we
							 * feel like ending
							 * if we end early
							 * if we end late

							n == notes.size();

							// send final signal(s)
							s = tie.size();
							// gather notes to be generated & sent
							vector<int> sigs(sig[s][1] - sig[s][0]);
							for (int i = 0; i < sigs.size(); i++) {
								sigs.at(i) = ALSAsig.at(sig[c][0] + i);
							}

							// create MIDI note(s) and output
							for (int i = 0; i < sigs.size(); i++) {
								sendNote(sigs.at(i));
							}

							break;
					}
				}

				// free event that was just analyzed
				snd_seq_free_event(ev);

			} while (snd_seq_event_input_pending(handle, 0) > 0);
		} // break if end of song reached
		if (n >= notes.size()) {
			// we have reached the end of the song
			// end this loop and wait for user input to start a new one
			return(0);
		}
	}
}

// make sure file is opened
// separate function because I have to use multiple files/song
void checkValidityFile(ifstream *file) {
	if (!file->is_open()) {
		printf("File could not be read\n");
		exit(-1);
	}
}

// send a single note
void sendNote(int note) {
	snd_seq_ev_set_noteon(&out, 0, note, 127);
	snd_seq_ev_set_direct(&out);
	snd_seq_ev_set_source(&out, outPort);
	snd_seq_ev_set_subs(&out);

	snd_seq_event_output (handle, &out);
	snd_seq_drain_output(handle);
}
*/
