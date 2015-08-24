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

int checkTracks(vector<int> inputTracks, vector<int> outputTracks, int mp, char * id);
int parseSongTempo(xmlNodePtr parent);

Song readFiles (string songPath, int *numInputTracks, int *numOutputTracks) {
	Song data;

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
		char *name = (char *)c->name;

		if (strcmp(name, "numberInputs") == 0) {
			*numInputTracks = atoi((char *)c->children->content);

		} else if (strcmp(name, "numberOutputs") == 0) {
			*numOutputTracks = atoi((char *)c->children->content);

		} else if (strcmp(name, "inputTrack") == 0) {
			inputTracks.push_back(atoi((char *)c->children->content));

		} else if (strcmp(name, "outputTrack") == 0) {
			outputTracks.push_back(atoi((char *)c->children->content));

		} else if (strcmp(name, "mpTrack") == 0) {
			mpTrack = atoi((char *)c->children->content);

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

	// start parsing doc
	int songTempo;
	cur = cur->children;
	while (cur != NULL) {
		char *name = (char *)cur->name;

		// get part data if needed
		if (strcmp(name, "part") == 0) {

			// get ID of current part
			char *id = (char *)xmlGetProp(cur, (xmlChar *)"id");
			int track = checkTracks(inputTracks, outputTracks, mpTrack, id);

			// check if track is needed
			if (track < 0) {
				if (track == -1) {

				}
			}
		}

		free(name);
		cur = cur->next;
	}

	xmlFree(cur);
	xmlFree(doc);

	return data;
}

// checks if track is needed
int checkTracks(vector<int> inputTracks, vector<int> outputTracks, int mp, char *id) {
	int ret = -2;

	// check input tracks
	for (unsigned int i = 0; i < inputTracks.size(); i++) {
		string partName = "P" +
				static_cast<ostringstream *>( &(ostringstream()
				<< inputTracks.at(i)))->str();

		if (strcmp(id, partName.c_str()) == 0) {
			ret = i;
			return ret;
		}
	}

	// check output tracks
	for (unsigned int i = 0; i < outputTracks.size(); i++) {
		string partName = "P" +
				static_cast<ostringstream *>( &(ostringstream()
				<< outputTracks.at(i)))->str();

		if (strcmp(id, partName.c_str()) == 0) {
			ret = i + 16;
			return ret;
		}
	}

	// check if measure/parts track
	string partName = "P" +
			static_cast<ostringstream *>( &(ostringstream()
			<< mp))->str();

	if (strcmp(id, partName.c_str()) == 0) {
		ret = 32;
		return ret;
	}

	// check for P1 for tempo only
	partName = "P" +
			static_cast<ostringstream *>( &(ostringstream()
			<< 1))->str();

	if (strcmp(id, partName.c_str()) == 0) {
		ret = -1;
		return ret;
	}

	// track not needed
	return ret;
}

int parseSongTempo(xmlNodePtr parent) {
	int tempo;

	return tempo;
}
