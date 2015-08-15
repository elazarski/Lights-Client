// reads data from multiple files and returns data usable by the ALSA backend
#include <fstream>
#include <vector>
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include "Obj.h"

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
vector<Event> readFiles(string songPath, int *numInputTracks, int *numOutputTracks) {
	string path;
	ifstream file; // read data file for info on MIDI file

	// fullArray[0] reserved for status messages
	vector<Event> fullArray(1);
	vector<int> mp;
	vector<vector<int> > inputTracks;
	vector<vector<int> > outputTracks;

	path = songPath + "/data.xml";

	// start reading data.xml
	xmlDocPtr data;
	xmlNodePtr current;

	printf("Parsing data.xml file...\n");
	data = xmlParseFile(path.c_str());
	if (data == NULL) {
		fprintf(stderr, "data.xml not parsed successfully\n");
		vector<Event> errorVect(1);
		errorVect.at(1).num = -1;
		return errorVect;
	}

	current = xmlDocGetRootElement(data);
	if (current == NULL) {
		fprintf(stderr, "Empty data.xml file\n");
		xmlFree(data);
		vector<Event> errorVect(1);
		errorVect.at(1).num = -1;
		return errorVect;
	}

	int currentInput = 0;
	int currentOutput = 0;

	current = current->children;
	while (current != NULL) {

		if (strcmp((char *)current->name, "numberInputs") == 0) {
			*numInputTracks = atoi((char *)current->children->content);
		} else if (strcmp((char *)current->name, "numberOutputs") == 0) {
			*numOutputTracks = atoi((char *)current->children->content);
		} else if (strcmp((char *)current->name, "mpTrack") == 0) {
			mp.push_back(atoi((char *)current->children->content));
		} else if (strcmp((char *)current->name, "inputTrack") == 0) {

			int numTracks = 1;
			int currentTrack = 0;
			xmlNodePtr inputTrack = current->children;


			while (inputTrack != NULL) {
				if (strcmp((char *)inputTrack->name, "numTracks") == 0) {
					numTracks = atoi((char *)inputTrack->children->content);
					inputTracks.push_back(vector<int>(numTracks));
				} else if (strcmp((char *)inputTrack->name, "track") == 0) {
					inputTracks.at(currentInput).at(currentTrack) = (atoi((char *)inputTrack->children->content));
					currentTrack++;
				}
				inputTrack = inputTrack->next;
			}

			xmlFree(inputTrack);
			currentInput++;
		} else if (strcmp((char *)current->name, "outputTrack") == 0) {

			int numTracks = 1;
			int currentTrack = 0;
			xmlNodePtr outputTrack = current->children;

			while (outputTrack != NULL) {
				if (strcmp((char *)outputTrack->name, "numTracks") == 0) {
					numTracks = atoi((char *)outputTrack->children->content);
					outputTracks.push_back(vector<int>(numTracks));
				} else if (strcmp((char *)outputTrack->name, "track") == 0) {
					outputTracks.at(currentOutput).at(currentTrack) = (atoi((char *)outputTrack->children->content));
					currentTrack++;
				}
				outputTrack = outputTrack->next;
			}

			xmlFree(outputTrack);
			currentOutput++;
		}

		current = current->next;
	}

	xmlFree(current);
	xmlFree(data);

	for (unsigned int i = 0; i < inputTracks.size(); i++) {
		for (unsigned int j = 0; j < inputTracks.at(i).size(); j++) {
			printf("Input: %d\n", inputTracks.at(i).at(j));
		}
	}

	for (unsigned int i = 0; i < outputTracks.size(); i++) {
		for (unsigned int j = 0; j < outputTracks.at(i).size(); j++) {
			printf("Output: %d\n", outputTracks.at(i).at(j));
		}
	}

	// start reading XML file
	printf("Starting to read gp.xml...\n");

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

	xmlFree(cur);
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
			return 32;
		}
	}

	// check output tracks
	for (unsigned int i = 0; i < outputTracks.size(); i++) {
		// each track individually
		for (unsigned int j = 0; j < outputTracks.at(i).size(); j++) {
			string partName = "P" +
					static_cast<ostringstream*>( &(ostringstream()
					<< outputTracks.at(i).at(j)))->str();

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
	float lastDur = 0.0;

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

		xmlFree(m1);
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

						/* chord:		type = CHORD, time = duration, num = noteNum, channel = 1
						 * note:		type = NOTE, time = duration, num = noteNum, channel = 0
						 */

						if (currentMeasure.at(i).channel == 1) { 			// chord
							part.push_back(currentMeasure.at(i));
							part.at(part.size() - 1).time = lastTime;
							part.at(part.size() - 1).channel = track;
							lastDur = currentMeasure.at(i).time;
						} else {											// normal note
							part.push_back(currentMeasure.at(i));

							// check if previous note was chord
							if (part.size() > 1 && part.at(part.size() - 2).type == CHORD) {
								// change lastTime so that it is when the previous chord ends
								lastTime = lastTime + lastDur * timeSig/divisions/4;
							}
							part.at(part.size() - 1).time = lastTime;

							// check if next note is chord
							if (currentMeasure.size() > i + 1 && currentMeasure.at(i + 1).type != CHORD) {

								// if we are here we are not parsing a chord
								lastTime = lastTime + currentMeasure.at(i).time * timeSig/divisions/4;
							} else {
								// we are in a chord, so change the type to a CHORD
								part.at(part.size() - 1).type = CHORD;
							}
							part.at(part.size() - 1).channel = track;
						}
					}


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
//			printf("Time: %f\n", i->time);
			i++;
		}
	}

	for (unsigned int i = 0; i < part.size(); i++) {
        part.at(i).channel = track;
        //printf("Channel: %d, Time: %f\n", track, part.at(i).time);
	}

	xmlFree(currentPart);

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

			xmlFree(attr);
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

			xmlFree(bar);
		} else if (strcmp((char *)cur->name, "note") == 0) { // gather note data

			xmlNodePtr note = cur->children; // in <note>

			bool tie = false;
			Event n;
			n.channel = 0;

			while (note != NULL) {
				// check for different types of data that can be found in <note>

				if (strcmp((char *)note->name, "pitch") == 0) {

					xmlNodePtr pitch = note->children;
					xmlChar *alter, *step, *octave;
					alter = 0;
					step = 0;
					octave = 0;

					if (n.channel != 1 && n.type != CHORD) {
						n.channel = 0;
						n.type = NOTE;
					}

					while (pitch != NULL) {

						if (strcmp((char *)pitch->name, "step") == 0) {
							step = pitch->children->content;
							//printf("Have step\n");
						} else if (strcmp((char *)pitch->name, "alter") == 0) {
							alter = pitch->children->content;
							//printf("Have alter\n");
						} else if (strcmp((char *)pitch->name, "octave") == 0) {
							octave = pitch->children->content;
							//printf("Have octave\n");
						}

						pitch = pitch->next;
					}

					n.num = parsePitch((char *)step, (char *)alter, (char *)octave);

					if (alter != NULL) {
						xmlFree(alter);
					}

					if (step != NULL) {
						xmlFree(step);
					}

					if (octave != NULL) {
						xmlFree(octave);
					}

					xmlFree(pitch);

				} else if (strcmp((char *)note->name, "chord") == 0) {
					n.channel = 1;
					n.type = CHORD;
				} else if (strcmp((char *)note->name, "duration") == 0) {
					n.time = atoi((char *)note->children->content);
				} else if (strcmp((char *)note->name, "rest") == 0) {
					n.type = META;
					n.channel = 4;
				} else if (strcmp((char *)note->name, "tie") == 0) {
					xmlChar *tieChar = xmlGetProp(note, (xmlChar *)"type");

					if (strcmp((char *)tieChar, "stop") == 0) {
						tie = true;
					}
				}
				note = note->next;
			}

			// if tied note is ending, make current note a rest
			// currently, the end of a note does not matter, just the beginning
			if (tie) {
				n.type = META;
				n.channel = 4;

				tie = false;
			}

			events.push_back(n);
			xmlFree(note);
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
		fprintf(stderr, "ERROR READING PITCH\n");
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
