/*
 * ReadFiles.cpp
 *
 *  Created on: Aug 23, 2015
 *      Author: eric
 */
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

int checkTracks(vector<int> inputTracks, vector<int> outputTracks, int mp,
		char * id);
vector<vector<int> > parseSongTempo(xmlNodePtr parent);
MP parseMP(xmlNodePtr parent);

Measure parseMeasure(xmlNodePtr parent);

Song readFiles(string songPath, int *numInputTracks, int *numOutputTracks) {
	Song song;

	string path;
	vector<int> inputTracks;
	vector<int> outputTracks;
	int mpTrack;

	// start reading data.xml
	// file contains data as to which tracks are needed
	path = songPath + "/data.xml";
	xmlDocPtr d;
	xmlNodePtr c;

	printf("Parsing data.xml file\n");
	d = xmlParseFile(path.c_str());
	if (d == NULL) {
		fprintf(stderr, "data.xml not parsed successfully\n");
		exit(1);
	}

	c = xmlDocGetRootElement(d);
	if (c == NULL) {
		fprintf(stderr, "Empty data.xml file\n");
		xmlFree(d);
		exit(1);
	}

	c = c->children;
	while (c != NULL) {
		char *name = (char *) c->name;

		if (strcmp(name, "numberInputs") == 0) {
			*numInputTracks = atoi((char *) c->children->content);

		} else if (strcmp(name, "numberOutputs") == 0) {
			*numOutputTracks = atoi((char *) c->children->content);

		} else if (strcmp(name, "inputTrack") == 0) {
			inputTracks.push_back(atoi((char *) c->children->content));

		} else if (strcmp(name, "outputTrack") == 0) {
			outputTracks.push_back(atoi((char *) c->children->content));

		} else if (strcmp(name, "mpTrack") == 0) {
			mpTrack = atoi((char *) c->children->content);

		}

		free(name);
		c = c->next;
	}

	xmlFree(d);
	xmlFree(c);

	printf("Starting to read gp.xml\n");

	path = songPath + "/gp.xml";
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(path.c_str());
	if (doc == NULL) {
		fprintf(stderr, "gp.xml not parsed successfully\n");
		exit(1);
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, "Empty gp.xml file\n");
		xmlFree(doc);
		exit(1);
	}

	vector<vector<int> > tempo;

	// start parsing doc
	cur = cur->children;
	while (cur != NULL) {
		char *name = (char *) cur->name;

		// get part data if needed
		if (strcmp(name, "part") == 0) {

			// get ID of current part
			char *id = (char *) xmlGetProp(cur, (xmlChar *) "id");
			int track = checkTracks(inputTracks, outputTracks, mpTrack, id);

			// check if track is needed
			if (track < 0) {
				if (track == -1) {

					// get tempo from P1
					tempo = parseSongTempo(cur);

				} else if (track == -2) {

					// get measure/parts track

				}
			} else {

			}
		}

		free(name);
		cur = cur->next;
	}

	xmlFree(cur);
	xmlFree(doc);

	return song;
}

// checks if track is needed
int checkTracks(vector<int> inputTracks, vector<int> outputTracks, int mp,
		char *id) {
	int ret = -3;

	// check input tracks
	for (unsigned int i = 0; i < inputTracks.size(); i++) {
		string partName = "P"
				+ static_cast<ostringstream *>(&(ostringstream()
						<< inputTracks.at(i)))->str();

		if (strcmp(id, partName.c_str()) == 0) {
			ret = i;
			return ret;
		}
	}

	// check output tracks
	for (unsigned int i = 0; i < outputTracks.size(); i++) {
		string partName = "P"
				+ static_cast<ostringstream *>(&(ostringstream()
						<< outputTracks.at(i)))->str();

		if (strcmp(id, partName.c_str()) == 0) {
			ret = i + 16;
			return ret;
		}
	}

	// check if measure/parts track
	string partName = "P"
			+ static_cast<ostringstream *>(&(ostringstream() << mp))->str();

	if (strcmp(id, partName.c_str()) == 0) {
		ret = -2;
		return ret;
	}

	// check for P1 for tempo only
	partName = "P"
			+ static_cast<ostringstream *>(&(ostringstream() << 1))->str();

	if (strcmp(id, partName.c_str()) == 0) {
		ret = -1;
		return ret;
	}

	// track not needed
	return ret;
}

// get tempo from P1
vector<vector<int> > parseSongTempo(xmlNodePtr parent) {

	// data to return
	vector<vector<int> > tempo = vector<vector<int> >(2, vector<int>());

	// declare a Track in order to make parsing repeats
	// easier. this will all be handled in this
	// function
	Track t;

	xmlNodePtr part = parent->children->next;

	while (part != NULL) {
		char *partName = (char *) part->name;

		// get measures
		if (strcmp(partName, "measure") == 0) {
			xmlNodePtr measure = part->children;

			// populate a measure if needed
			// add it to t either way
			Measure m;

			while (measure != NULL) {
				char *measureName = (char *) measure->name;

				if (strcmp(measureName, "sound") == 0) { // tempo
					char *tempoNode = (char *) xmlGetProp(measure,
							(xmlChar *) "tempo");

					m.time.tempo = atoi(tempoNode);

					free(tempoNode);
				} else if (strcmp(measureName, "barline") == 0) { // repeat
					Event e;
					e.type = META;

					xmlNodePtr barline = measure->children;

					while (barline != NULL) {
						char *barName = (char *)barline->name;

						// check if repeat actually happens
						if (strcmp(barName, "repeat") == 0) {
							char *direction = (char *)xmlGetProp(barline, (xmlChar *)"direction");

							// get direction of repeat
							if (strcmp(direction, "forward") == 0) {
								e.time = -1;
							} else {
								// get number of repeats
								char *times = (char *)xmlGetProp(barline, (xmlChar *)"times");

								e.time = 1;
								e.note = atoi(times);

								free(times);
							}
						}

						free(barName);
						barline = barline->next;
					}

					xmlFree(barline);
					m.voiceOne.push_back(e);
				}

				free(measureName);
				measure = measure->next;
			}

			t.measures.push_back(m);
		}

		free(partName);
		part = part->next;
	}

	/*
	 xmlNodePtr measure = parent->children->next->children;

	 while (measure != NULL) {
	 char *name = (char *)measure->name;

	 if (strcmp(name, "sound") == 0) {
	 char *tempoNode = (char *)xmlGetProp(measure, (xmlChar *)"tempo");

	 tempo = atoi(tempoNode);
	 free(tempoNode);
	 }

	 free(name);
	 measure = measure->next;
	 }

	 xmlFree(measure);

	 */

	xmlFree(part);
	return tempo;
}

// using a separate function for MP because it has to be parsed slightly differently
MP parseMP(xmlNodePtr parent) {
	MP mp;

	xmlNodePtr part = parent->children->next;
	int forwardRepeatIndex = 0;
	int currentMeasure = 0;

	while (part != NULL) {
		char *name = (char *) part->name;

		if (strcmp(name, "measure") == 0) {
			Measure measure = parseMeasure(part);

			currentMeasure++;
		}

		free(name);
		part = part->next;
	}

	xmlFree(part);
	return mp;
}

// returns Measure of parsed Events
Measure parseMeasure(xmlNodePtr parent) {
	Measure measure;

	xmlNodePtr event = parent->children;

	while (event != NULL) {
		char *name = (char *) event->name;

		if (strcmp(name, "attributes") == 0) {

			// divisions are here
			xmlNodePtr attr = event->children;

			while (attr != NULL) {
				char *name = (char *) attr->name;

				if (strcmp(name, "divisions") == 0) {
					char *div = (char *) attr->children->content;

					Event e;
					e.type = META;
					e.note = 3;
					e.time = atof(div);

					measure.voiceTwo.push_back(e);
				}

				free(name);
				attr = attr->next;
			}

			xmlFree(attr);
		}

		free(name);
		event = event->next;
	}

	xmlFree(event);
	return measure;
}
