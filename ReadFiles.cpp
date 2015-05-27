// reads data from multiple files and returns data usable by the ALSA backend
#include <fstream>
#include <vector>
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <sstream>
#include "Obj.h"
#include <algorithm>

using namespace std;

// forward declarations
bool sortFunction(Event a, Event b);
vector<Event> sortArray(vector<Event> fullArray);
int parsePitch(char *step, char *alter, char *octave);
vector<Event> parseMeasure(xmlNodePtr cur);
vector<Event> parsePart(xmlNodePtr cur, int track);
int checkTracks(vector<vector<int> > inputTracks, vector<int> mp, vector<vector<int> > outputTracks, xmlChar *id);

// called by main
// reads all files and returns vector<Event> of all events to be handled, sorted by time.
vector<Event> readFiles(string songPath, int *numTracks) {
	string path;
	ifstream file; // read data file for info on MIDI file
	int num;

	// fullArray[0] reserved for status messages
	vector<Event> fullArray(1);
	 Event er;
	 er.type = META;

	// start reading /data file
	/* structure:
	 * 		how many inputs
	 * 		how many outputs
	 * 		how many tracks to read for key1
	 * 		first key1 track second key1 track...
	 * 		key 2...
	 * 		how many tracks for measures & parts
	 * 		mp1 mp2...
	 * 		how many tracks to read for output
	 * 		out1 out2...
	 */

	path = songPath + "/data.txt";
	file.open(path.c_str());
	if (!file.is_open()) { // check if file unsuccessfully opened
		// file was unable to be opened
		 er.num = -1;

		 vector<Event> errorVect;
		 errorVect.push_back(er);
		 return errorVect;
	}

	// 1st number is for how many input channels there are
	file >> num;
	int numKeyChannels = num;
	*numTracks = num;

	// 2nd number is for how many output channels there are
	file >> num;
	int numOutChannels = num;
	*numTracks += num;

	// read numKeyChannels tracks
	vector<vector<int> > inputTracks;
	for (int i = 0; i < numKeyChannels; i++) {
		file >> num;
		int currentKeyTracks = num; // number of tracks for current input
		vector<int> currentInput;
		for (int j = 0; j < currentKeyTracks; j++) {
			file >> num;
			currentInput.push_back(num);
		}
		inputTracks.push_back(currentInput);
	}

	// parts and measures
	file >> num;
	vector<int> mp(num);
	for (int i = 0; i < num; i++) {
		file >> mp.at(i);
	}

	// read numOutChannels tracks
	vector<vector<int> > outputTracks;
	for (int i = 0; i < numOutChannels; i++) {
		file >> num;
		int currentOutTracks = num;
		vector<int> currentOutput;
		for (int j = 0; j < currentOutTracks; j++) {
			file >> num;
			currentOutput.push_back(num);
		}
		outputTracks.push_back(currentOutput);
	}

	// close file
	file.close();

	// start reading XML file
	printf("Starting to read XML...\n");

	// psuedo code

	/* use Guitar Pro exported MusicXML files
	 * we have the id's of the parts needed for everything
	 * 1. start reading the XML file read P1 <attributes>
	 * 1.5 if we are in <measure="1"> of P1 there will be <attributes> that contain <sound...tempo="x">
	 * 	   save tempo to metadata event in array
	 * 2. read until we reach a <part> node (if this <part> is one of the ones specified above, parse
	 * 3. next node is <measure>, parse <note>'s from it
	 * 4. next node could be <barline> will contain <repeat>
	 * 		<repeat direction="forward/backward" times="X" />
	 * 		will have to save vector of from forward to backward in a vector and append X times to fullArray
	 * 		if no forward, repeat fullArray on this channel X times
	 * 5. parse <note>
	 * 6. next node could be <chord/> or <rest/>
	 * 		if <rest/> get length and add ticks to position of next note
	 * 		if <chord/> do not add ticks to next note
	 */

	path = songPath + "/gp.xml";
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(path.c_str());
	if (doc == NULL) {
		fprintf(stderr, "Document not parsed successfully\n");
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, "Empty document\n");
		xmlFreeDoc(doc);
	}

	// start parsing document
	// look for <part>
	cur = cur->children;
	while (cur != NULL) {
		if (strcmp((char *)cur->name, "part") == 0) {

				// get part id
				xmlChar *id = xmlGetProp(cur, (xmlChar *)"id"); // get the current attribute
				int track = checkTracks(inputTracks, mp, outputTracks, id);

				// check for P1
				if (strcmp((char *)id, ("P" + static_cast<ostringstream*>(
					&(ostringstream() << 1))->str()).c_str()) == 0) {

					// at the very least, get tempo from P1
					vector<Event> part;
					part = parsePart(cur, -1);

					// add to fullArray
					fullArray.push_back(part.at(0));
				} else if (track >= 0) {

					// check for what we need from XML as far as parts go
					if (track == 0) track = 32;
					vector<Event> part = parsePart(cur, track);

					for (unsigned int i = 0; i < part.size(); i++) {
                        fullArray.push_back(part.at(i));
					}
				}

				xmlFree(id);
		}
		// move to next part
		cur = cur->next;
	}

	xmlFree(doc);

	// sort fullArray based on time
	vector<Event> finalArray = sortArray(fullArray);

	return finalArray;
}

// checks if current track needs to be parsed, if it does returns that track number. if not, returns -1
int checkTracks(vector<vector<int> > inputTracks, vector<int> mp, vector<vector<int> > outputTracks, xmlChar *id) {

	// check input tracks
	for (unsigned int i = 0; i < inputTracks.size(); i++) {
		// each track individually
		for (unsigned int j = 0; j < inputTracks.at(i).size(); j++) {
			string partName = "P" +
					static_cast<ostringstream*>( &(ostringstream()
					<< inputTracks.at(i).at(j)))->str();

			if (strcmp((char *)id, partName.c_str()) == 0) {
				return i;
			}
		}
	}

	// measures & parts
	for (unsigned int i = 0; i < mp.size(); i++) {
		string partName = "P" +
				static_cast<ostringstream*>( &(ostringstream()
				<< mp.at(i)))->str();

		if (strcmp((char *)id, partName.c_str()) == 0) {
			return 0;
		}
	}

	// check output tracks
	for (unsigned int i = 0; i < outputTracks.size(); i++) {
		// each track individually
		for (unsigned int j = 0; j < outputTracks.at(i).size(); j++) {
			string partName = "P" +
					static_cast<ostringstream*>( &(ostringstream()
					<< inputTracks.at(i).at(j)))->str();

			if (strcmp((char *)id, partName.c_str()) == 0) {
				return i + 16;
			}
		}
	}

	// current track not needed
	return -1;
}

// parse XML track, returns vector of parsed events
vector<Event> parsePart(xmlNodePtr cur, int track) {
	vector<Event> part;
	xmlNodePtr currentPart = cur->children->next;
	int divisions;
	float lastTime = 0.0;
	int forwardRepeatIndex = 0;
	int timeSig;

	if (track < 0) { // do not parse for part, merely tempo

		// just find <sound> in measure 1
		xmlNodePtr m1 = currentPart->children;
		while (m1 != NULL) {
			if (strcmp((char *)m1->name, "sound") == 0) {
				xmlChar *tempo = xmlGetProp(m1, (xmlChar *)"tempo");

				Event ev;
				ev.type = META;
				ev.num = atoi((char *)tempo);
				ev.channel = 0;
				ev.time = 0;

				part.push_back(ev);

				xmlFree(tempo);
			}
			m1 = m1->next;
		}
	} else { // get note data for part
		while (currentPart != NULL) {
			if (strcmp((char *)currentPart->name, "measure") == 0) { // get data measure by measure
				vector<Event> currentMeasure = parseMeasure(currentPart->children);

				// append currentMeasure to part
				for (unsigned int i = 0; i < currentMeasure.size(); i++) {

					// check for META events
					if (currentMeasure.at(i).type == META) {

						/* divisions: 	type = META, num = divisions, channel = 3, time = 0
						 * repeat: 		type = META, channel == 2
						 * 	  forward: 		time = -1
						 * 	  backward:		time = 1, num = number of repeats
						 * rest:		type = META, time = duration, channel = 4
						 * time sig		type = META, channel = 5, num = timeSig
						 */

						if (currentMeasure.at(i).channel == 3) { 			// divisions per quarter note
							divisions = currentMeasure.at(i).num;
						} else if (currentMeasure.at(i).channel == 2) { 	// repeat
							if (currentMeasure.at(i).time == -1) { 			// forward

								// add 1 to the current size so that
								// the repeat starts on the next note
								// in the array
								forwardRepeatIndex = part.size();
							} else { 										// backward

								// find how many events to repeat
								int repSize = part.size() - forwardRepeatIndex;

								// append from the repeat index repSize notes
								for (int k = 0; k < repSize+1; k++) {
                                    float timeBegin = part.at(forwardRepeatIndex+k).time;
                                    float timeEnd = part.at(forwardRepeatIndex+k+1).time;
                                    float dur = timeEnd - timeBegin;

                                    Event e = part.at(forwardRepeatIndex + k);
                                    e.time = lastTime;

                                    lastTime = lastTime + dur;

                                    part.push_back(e);
								}
							}
						} else if (currentMeasure.at(i).channel == 4) {		// rest
							part.push_back(currentMeasure.at(i));
							part.at(part.size() - 1).time = lastTime;
							lastTime = lastTime + currentMeasure.at(i).time * timeSig/divisions/4;
						} else if (currentMeasure.at(i).channel == 5) {		// time sig
							timeSig = currentMeasure.at(i).num;
						}
					} else {

						/* chord:		type = NOTE, time = duration, num = noteNum, channel = 1
						 * note:		type = NOTE, time = duration, num = noteNum, channel = 0
						 */

						if (currentMeasure.at(i).channel == 1) { 			// chord
							part.push_back(currentMeasure.at(i));
							part.at(part.size() - 1).time = lastTime;
							if (currentMeasure.at(i + 1).type == NOTE && currentMeasure.at(i + 1).channel != 1) { // change time for next note
								lastTime = lastTime + currentMeasure.at(i).time * timeSig/divisions/4;
							}
							part.at(part.size() - 1).channel = track;
						} else {											// normal note
							part.push_back(currentMeasure.at(i));
							part.at(part.size() - 1).time = lastTime;
							lastTime = lastTime + currentMeasure.at(i).time * timeSig/divisions/4;
							part.at(part.size() - 1).channel = track;
						}
					}

					//printf("%d\n", currentMeasure.at(i).time);
				}

			}
			currentPart = currentPart->next;
		}
	}

	// clean part
	// remove forward repeats and other META events that are unnecessary
	//printf("Length: %d\n", part.size());
	int size = part.size();
	for (vector<Event>::iterator i = part.begin(); i != part.end();) {
		if (i->channel == 4) {					// remove rest
            i = part.erase(i);
			size--;

		} else if (i->channel == 2) {			// remove forward repeat
			i = part.erase(i);
			size--;
        } else {                                        // increment i
			//printf("Time: %f\n", i->time);
			i++;
		}
	}

	for (unsigned int i = 0; i < part.size(); i++) {
        part.at(i).channel = track;
	}

	return part;
}

// parse XML measure, returns vector of parsed events
vector<Event> parseMeasure(xmlNodePtr cur) {
	vector<Event> events;
	int divisions;

	// event for repeat
	Event rep;
	rep.type = META;
	rep.time = -1;
	rep.channel = 2;

	// parse measure
	while (cur != NULL) {

		// divisions will be in attributes
		if (strcmp((char *)cur->name, "attributes") == 0) {
			xmlNodePtr attr = cur->children;
			while (attr != NULL) {
				if (strcmp((char *)attr->name, "divisions") == 0) {
					xmlChar *div = attr->children->content;
					divisions = atoi((char *)div);
					xmlFree(div);

					// create event and append to events
					Event ev;
					ev.type = META;
					ev.num = divisions;
					ev.channel = 3;
					ev.time = 0;

					events.push_back(ev);
				} else if (strcmp((char *)attr->name, "time") == 0) {
					xmlNodePtr time = attr->children;
					while (time != NULL) {
						if (strcmp((char *)time->name, "beat-type") == 0) {
							Event ev;
							ev.type = META;
							ev.num = atoi((char *)time->children->content);
							ev.channel = 5;
							ev.time = 0;

							events.push_back(ev);
						}
						time = time->next;
					}
					xmlFree(time);
				}
				attr = attr->next;
			}
		} else if (strcmp((char *)cur->name, "barline") == 0) {
			// check for <repeat>
			xmlNodePtr bar = cur->children;
			while (bar != NULL) {
				if (strcmp((char *)bar->name, "repeat") == 0) {
					// forwards or backwards?
					xmlChar *direction = xmlGetProp(bar, (xmlChar *)"direction");

					if (strcmp((char *)direction, "forward") == 0) {
						rep.time = -1;
						events.push_back(rep);
					} else {
						xmlChar *times = xmlGetProp(bar, (xmlChar *)"times");
						rep.time = 1;
						rep.num = (atoi((char *)times));
						xmlFree(times);
						// check for rep.num == 1 later to rep to events rep.time times at end
					}

					xmlFree(direction);
				}
				bar = bar->next;
			}
		} else if (strcmp((char *)cur->name, "note") == 0) { // gather note data
			xmlNodePtr note = cur->children; // in <note>

			Event n;

			while (note != NULL) {
				// check for different types of data that can be found in <note>

				if (strcmp((char *)note->name, "pitch") == 0) {
					xmlNodePtr pitch = note->children;
					char *alter, *step, *octave;
					n.channel = 0;

					while (pitch != NULL) {
						if (strcmp((char *)pitch->name, "step") == 0) {
							step = (char *)pitch->children->content;
						} else if (strcmp((char *)pitch->name, "alter") == 0) {
							alter = (char *)pitch->children->content;
						} else if (strcmp((char *)pitch->name, "octave") == 0) {
							octave = (char *)pitch->children->content;
						}
						pitch = pitch->next;
					}

					n.num = parsePitch(step, alter, octave);
					n.type = NOTE;
				} else if (strcmp((char *)note->name, "chord") == 0) {
					n.channel = 1;
				} else if (strcmp((char *)note->name, "duration") == 0) {
					n.time = atoi((char *)note->children->content);
				} else if (strcmp((char *)note->name, "rest") == 0) {
					n.type = META;
					n.channel = 4;
				}
				note = note->next;
			}
			events.push_back(n);
		}
		cur = cur->next;
	}

	if (rep.time != -1) {
		for (int i = 0; i < rep.num; i++) {
			events.push_back(rep);
		}
	}

	return events;
}

// convert XML values into numerical note value
int parsePitch(char *step, char *alter, char *octave) {
	// return note num based on step, alter, and octave values
	int ret = 0;

	switch((int)*step) {
	case 'C':
		ret = 0;
		break;
	case 'D':
		ret = 2;
		break;
	case 'E':
		ret = 4;
		break;
	case 'F':
		ret = 5;
		break;
	case 'G':
		ret = 7;
		break;
	case 'A':
		ret = 9;
		break;
	case 'B':
		ret = 11;
		break;
	default:
		fprintf(stderr, "ERROR READING FILE\n");
		break;
	}

	ret = ret + atoi(octave) * 12;

    if (alter != NULL) {
        ret = ret + atoi(alter);
    }

	return ret;
}


// sorts fullArray based on time
vector<Event> sortArray(vector<Event> fullArray) {

    vector<Event> sortedVector = fullArray;

    sort(sortedVector.begin(), sortedVector.end(), sortFunction);

    return sortedVector;

}

// function used for sort command
bool sortFunction(Event a, Event b) {
    return (a.time < b.time);
}
