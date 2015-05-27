/*
 * File for user interaction
 * currently only text based
 * Eventually create UI?
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <string>


using namespace std;

// gets user input on what song to play next
// *output will be the path to the file
char getInput(string *output) {
	char complete = 0;
	char play = 0;
	string input = "";
	string dir = "";
	do {
		cout << "Enter the title of the song to be played, or just press enter to end program: ";
		getline(cin, input);

		if (input.compare("") != 0) {
			cout << "Searching for song data for " << input << "...\n";

			// make input lower case
			string fileName = input;
			for (unsigned int i = 0; i < fileName.size(); ++i) {
				fileName[i] = tolower(input[i]);
			}

			// build path to file
			dir = getenv("HOME");
			dir.append("/PA/Lights Files/");
			dir.append(fileName);

			// check for folder
			struct stat info;
			if (stat(dir.c_str(), &info) != 0) { // no folder found on disk for user input
				cout << "Folder does not exist for song" << endl;
				cout << "Please try again" << endl;
			} else if (info.st_mode & S_IFDIR) { // folder found, can continue
				cout << "Found data for " << input << "." << endl;
				cout << "Loading..." << endl;
				complete = 1;
				play = 1;
			} else  { // something else went wrong
				cout << "Please try again" << endl;
			}
		} else { // empty input -> return empty string for main to exit
			complete = 1;
			dir = "";
			play = 0;
		}
	} while (complete != 1); // empty string will result in graceful program end

	// give *output value (if needed)
	*output = dir;
	return play;
}
