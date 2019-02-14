/*
Name: Molly Johnson
ONID: johnsmol
CS 344 Winter 2019
Assignment 2
Due: 2/13/19
All information used to create this code is adapted from the OSU CS 344 Winter 2019 
lectures and assignment instructions/hints unless otherwise specifically indicated.
Also adapted from my own work previously created on 10/25/18 (I took the class
previously in the Fall 2018 term but am retaking for a better grade).
*/

//include all header files
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>

//constant macro definitions method adapted from:
//https://www.techonthenet.com/c_language/constants/create_define.php
#define NUM_ROOMS 7
#define NUM_NAME_OPTIONS 10
#define MIN_CONNECTIONS 3
#define MAX_CONNECTIONS 6
#define NUM_ROOM_TYPES 3
#define START_ROOM "START_ROOM"
#define END_ROOM "END_ROOM"
#define MID_ROOM "MID_ROOM"
#define EMPTY "EMPTY"

//create mutex (created global mutex since mutex wouldn't be accessible for locking/unlocking
//in the TimeKeep function unless either the mutex was passed in as an argument or made global
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

//create Room struct w/ room type , num of connections, name, all connection names,
//and the struct array count (i.e. index of this room in the array of room structs)
struct Room
{
	char *roomType;
	int connectionsCount;
	char *roomName;
	char *connectionNames[MAX_CONNECTIONS];
	int structArrayCt;
};

//provide function declarations so don't have to make sure all functions
//declared in perfect order where each function only calls other functions
//already defined
char *GetDirName();
void InitializeRooms(char *directoryNameIn, struct Room roomArray[]);
void CallocRooms(struct Room roomArray[]);
void FreeRooms(struct Room roomArray[]);
int GetStartIndex(struct Room roomArray[]);
int GetEndIndex(struct Room roomArray[]);
void InitializeUserStart(struct Room roomArray[], int userIdx, int startIdx);
void ResetEmptyConnections(struct Room roomArray[], int roomIndex);
void PlayGame(struct Room roomArray[], int startIdx, int endIdx, int userIdx);
void CurRoomPrint(struct Room roomArray[], int userIndex);
int InputValidation(struct Room roomArray[], int userIndex, char *userStringInput);
void SetNewLocation(struct Room roomArray[], int  userIndex, char *userStringInput);
void* TimeKeep(void* argument);

/*
NAME
timekeep
SYNOPSIS
gets current date/time, prints to file
DESCRIPTION
locks mutex, gets current date/time and formats this information according to
assignment specs, prints this information to file "currentTime.txt", unlocks mutex.
void* return type and argument used to match lecture example and so the function can
be easily reused regardless of argument type or type of return value.
*/
void* TimeKeep(void* argument)
{
	//lock mutex (this call will block if called when mutex is locked)
	pthread_mutex_lock(&myMutex);

	//use strftime to get current time in proper format.
	//strftime use adapted from:
	//https://linux.die.net/man/3/strftime and
	//https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm and
	//https://apidock.com/ruby/DateTime/strftime

	//create time_t and struct
	time_t rawtime;
	struct tm *info;

	//create buffer for strftime results and memset with null term chars
	char buffer[256];
	memset(buffer, '\0', sizeof(buffer));

	//get the raw (unformatted) time
	time(&rawtime);

	//use time pointed by the rawtime to fill the struct with the local time values
	info = localtime(&rawtime);

	//put the formatted local time into the buffer
	//%l gives the hour and removes a preceding zero if the hour is in single digits.
	//%M will give the minutes
	//%P gives am or pm in lower-case
	//%A gives full day name
	//%B gives full month name
	//%d give day of the month (decimal form)
	//%Y gives the year
	strftime(buffer, 256, "%l:%M%P, %A, %B %d, %Y", info);

	//file output adapted from (in addition to lectures):
	//https://www.cs.bu.edu/teaching/c/file-io/intro/ and
	//https://www.tutorialspoint.com/cprogramming/c_file_io.htm
	//file permissions information from:
	//https://dineshbhopal.wordpress.com/2014/12/03/php-r-r-w-w-file-handling/

	//create output file with w+ so it can be created if doesn't exist or overwritten
	//if does exist. name it currentTime.txt
	FILE *outputFile;
	outputFile = fopen("currentTime.txt", "w+");

	//check that file was opened correctly (output == NULL means couldn't be opened correctly)
	if(outputFile == NULL)
	{
		perror("output file would not open.");
		exit(1);
	}

	//print the properly formatted time and date to the file currentTime.txt
	fprintf(outputFile, buffer);	

	//close the output file
	fclose(outputFile);

	//unlock mutex
	pthread_mutex_unlock(&myMutex);

	//return null as return type can be any (void *) but there's no value needed to return since time was output to a file
	return NULL;
}

/*
NAME
resetemptyconnections
SYNOPSIS
resets all connections
DESCRIPTION
resets all connections for a room to "empty" so when the user is reset to a room
w fewer connections than its previous one the count will still be correct
*/
void ResetEmptyConnections(struct Room roomArray[], int roomIndex)
{
	int k;
	
	//loop through all connections
	for(k = 0; k < MAX_CONNECTIONS; k++)
	{
		//copy "EMPTY" for all of the connection names to reset all connections for this room to empty
		strcpy(roomArray[roomIndex].connectionNames[k], EMPTY);
	}

	//reset the number of connections for this room to zero
	roomArray[roomIndex].connectionsCount = 0;
}

/*
NAME
initializeallrooms
SYNOPSIS
initializes all the rooms except the current location
DESCRIPTION
opens the proper directory, then acceses each of the room files in that directory.
goes through each file and parses it string by string, saving the needed parts
to save into the struct for each room. does this for all files and all parameters
(except the user's current location room). 
*/
void InitializeRooms(char *directoryNameIn, struct Room roomArray[])
{
	//some parts of opening a directory and viewing files adapted from:
	//directory manipulation lecture code by instroctor Brewster as well as
	//stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
	
	//create directory stream and dirent struct (which keeps the file serial num
	//and name of entry in the struct)
	DIR *newestDir;
	struct dirent *currentFile;

	//open the directory name passed into the function
	newestDir = opendir(directoryNameIn);
	
	//keep track of which file is being read
	int fileNum = 0;

	//if directory was able to be opened (if able to be opened will be > 0)	
	if(newestDir > 0)
	{
		//while there are still files in the directory to be checked (will == NULL when there are
		//no more files to be checked).
		while ((currentFile = readdir(newestDir)) != NULL)
		{
			//this will make sure files only the room files get opened and not any "." or ".." hidden files
			//(currentFile->d_name is the dirent struct file name)
			if((strcmp(currentFile->d_name, ".") != 0) && (strcmp(currentFile->d_name, "..") != 0))
			{
				//create directory name with ./ preceding it so can be opened
				char directory[] = "./";
				strcat(directory, directoryNameIn);

				//create file path (./directoryNameIn)
				const char *filePath = directory;

				//change directory to the current file path
				chdir(filePath);

				//set the room name to the current file's name
				char *roomFileName = currentFile->d_name;

				//create file stream and open the current room file (a+ to append to the file and update it)
				FILE *roomFile;
				roomFile= fopen(roomFileName, "a+");

				//check if file was opened correctly. if not will return NULL.
				if(roomFile == NULL)
				{
					printf("\nFile would not open.\n");
					exit(1);
				}
				else //if room file opened correctly
				{
					//fgets information adapted from:
					//http://www.cplusplus.com/reference/cstdio/fgets/
					//strstr information adapted from:
					//https://www.geeksforgeeks.org/strstr-in-ccpp/
					//removing first two chars adapted from:
					//https://stackoverflow.com/questions/4761764/how-to-remove-first-three-characters-from-string-with-c
					//removing trailing newline character adapted from:
					//https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
					char myString[100];

					//: is the token to be searched for using strtok
					char *token = ":";
					char *index;
					int lineCount = 0;
					int connectionsCounter = 0;
					while(fgets(myString, 100, roomFile)) //check all lines of the file
					{
						//find the index of the token
						index = strstr(myString, token); 

						//remove the two preceding spaces that come after the ":" in the files but before file type, room name, or connection names
						index +=2;

						//remove the newline char fgets adds using strtok
						char *savedString = strtok(index, "\n"); 

						//if reading the first line of the file (i.e. room name)
						if(lineCount == 0)
						{
							//copy the room name from the file to the current room struct's room name
							strcpy(roomArray[fileNum].roomName,savedString);
						}
						//if reading a room type line in the file
						else if((strcmp(savedString, END_ROOM) == 0) || (strcmp(savedString, MID_ROOM) == 0) || (strcmp(savedString, START_ROOM) == 0))
						{
							//copy the room type from the file to the current room struct's  rooom type
							strcpy(roomArray[fileNum].roomType, savedString);
						}
						else //else it's reading a connection name from the file
						{
							//copy the connection from the file to the current room struct's next connection
							strcpy(roomArray[fileNum].connectionNames[connectionsCounter], savedString);

							//increment connections counter
							connectionsCounter++;
						}
						//increment line counter (keeps track of which line in the file's being read)
						lineCount++;
					}
					//set the current room struct's num of connections to the num of connections that have been added from
					//the input file.
					roomArray[fileNum].connectionsCount = connectionsCounter;

					//close the current room file
					fclose(roomFile);
				}

				//the current room struct's count (i.e. position in the array of room structs) is the file num
				roomArray[fileNum].structArrayCt = fileNum;

				//increment the file num
				fileNum++;
			}
		}
		//close the open directory
		closedir(newestDir);
	}

	//change current directory back one directory (".." means to go back one one directory),
	//so that the currentTime.txt file created later will be created in the proper directory
	//(the same directory the game itself is in)
	chdir("..");
}

/*
NAME
freerooms
SYNOPSIS
frees memory for all rooms
DESCRIPTION
frees allocated memory for all the rooms
*/
void FreeRooms(struct Room roomArray[])
{
	int k;
	int h;
	
	//loop through all rooms in the array of room structs (+1 for player's current room)
	for(k = 0; k < NUM_ROOMS + 1; k++)
	{
		//free room name and type of the room struct and set to null
		free(roomArray[k].roomName);
		roomArray[k].roomName = NULL;
		free(roomArray[k].roomType);
		roomArray[k].roomType = NULL;

		//loop through all 6 possible connections (even empty ones)
		for(h = 0; h < MAX_CONNECTIONS; h++)
		{
			//free connections from the room and set to null
			free(roomArray[k].connectionNames[h]);
			roomArray[k].connectionNames[h] = NULL;
		}
	}
}

/*
NAME
get dir name
SYNOPSIS
gets the directory name for the files 
DESCRIPTION
gets the directory name the files are in and returns this name
*/
char *GetDirName()
{
	//method using stat to get most recent directory name adapted from 
	//(in addition to the manipulating directories lecture code provided by
	//instructor Brewster,section 2.4, which this code was most heavily adapted from):
	//http://codewiki.wikidot.com/c:system-calls:stat and
	//https://linux.die.net/man/2/stat and
	//stackoverflow.com/questions/42170824/use-stat-to-get-most-recently-modified-directory 
	//to keep track of most recent directory created
	int newestDirTime = -1;

	//create prefix (which will be shared for all directories) and create
	//string for the entire directory name
	char targetDirPrefix[50] = "johnsmol.rooms";

	//use static char so function can return it without it being destroyed after
	//the function closes
	static char newestDirName[256];

	//memset fills a block of memory
	memset(newestDirName, '\0', sizeof(newestDirName));

	//for starting directory
	DIR *dirToCheck;
	
	//create struct for current subdirectory of parent (i.e. starting) directory
	//and that directory's (the subdirectory's) attribute stats
	struct dirent *fileInDirectory;
	struct stat directoryAttributes;

	//open the directory currently in when this program was run
	dirToCheck = opendir(".");

	//check that the current directory was openable (>0 means it was open, <= 0
	//means there was an error opening the current directory)
	if(dirToCheck > 0)
	{
		//check each entry in the directory to be checked
		while ((fileInDirectory = readdir(dirToCheck)) != NULL)
		{
			//check if the entry matches the johnsmol.rooms prefix (since all
			//directories we will need to check should all begin with that
			//prefix regardless of the time the directory was created)
			if (strstr(fileInDirectory->d_name, targetDirPrefix) != NULL)
			{
				//get the entry's attributes if it matched the desired prefix
				stat(fileInDirectory->d_name, &directoryAttributes);

				//check if the time for this entry is newer than the
				//current newest directory's time. if so, copy that
				//directory's name into the newest directory's name to save it
				if((int)directoryAttributes.st_mtime > newestDirTime)
				{
					//if this entry newer, set it to new newest dir time
					newestDirTime = (int)directoryAttributes.st_mtime;

					//memset the newest directory name to erase all prev data
					memset(newestDirName, '\0', sizeof(newestDirName));

					//set newest directory to current entry
					strcpy(newestDirName, fileInDirectory->d_name);
				}
			}
		}
		//close the open directory
		closedir(dirToCheck);
	}

	//return the newest directory's name
	return newestDirName;
}

/*
NAME
callocrooms
SYNOPSIS
allocates room memory
DESCRIPTION
uses calloc to allocate memory for all rooms, initializes variables that don't
need calloc and sets all room connections to empty
*/
void CallocRooms(struct Room roomArray[])
{
	int i;
	int j;

	//loop through all rooms in the array of room structs
	for(i = 0; i < NUM_ROOMS + 1; i++)
	{
		//create dynamic memory for room name and type using calloc (which initializes memory to 0)
		roomArray[i].roomName = calloc(16, sizeof(char));
		roomArray[i].roomType = calloc(16, sizeof(char));

		//set connections count and struct array count (position of room struct in the array of room structs) to zero
		roomArray[i].connectionsCount = 0;
		roomArray[i].structArrayCt = 0;

		//loop through all connections for the room
		for(j = 0; j < MAX_CONNECTIONS; j++)
		{
			//create dynamic memory for connection name
			roomArray[i].connectionNames[j] = calloc(16, sizeof(char));
			
			//set all room connections to empty at first, can be changed later
			strcpy(roomArray[i].connectionNames[j], EMPTY);
		}
	}
}

/*
NAME
getstartindex
SYNOPSIS
gets the start index for the start room
DESCRIPTION
goes thru all rooms til finds room w type start room. returns that index
*/

int GetStartIndex(struct Room roomArray[])
{
	int k;
	int startIndex;

	//loop thru all rooms
	for (k = 0; k < NUM_ROOMS; k++)
	{
		//if the room type of the current room is start room, the current intex (k) is the index of the start room
		if(strcmp(roomArray[k].roomType, START_ROOM) == 0)
		{
			//start index found, can be assigned and returned
			startIndex = k;

			//break out of loop since start found
			k = NUM_ROOMS + 100;
		}
	}

	return startIndex;	
}

/*
NAME
getendindex
SYNOPSIS
gets the end index for the end room
DESCRIPTION
goes thru all rooms til finds room w type end room. returns that index
*/
int GetEndIndex(struct Room roomArray[])
{
	int k;
	int endIndex;

	//loop thru all rooms
	for (k = 0; k < NUM_ROOMS; k++)
	{
		//if the room type of the current room is end room, the current intex (k) is the index of the end room
		if(strcmp(roomArray[k].roomType, END_ROOM) == 0)
		{
			//end index found, can be assigned and returned
			endIndex = k;

			//break out of loop since end found
			k = NUM_ROOMS + 100;
		}
	}

	return endIndex;
}

/*
NAME
initializeuserstart
SYNOPSIS
initializes the user's starting location
DESCRIPTION
finds the starting room and sets the user's current room to that start room
*/
void InitializeUserStart(struct Room roomArray[], int userIdx, int startIdx)
{
	//set start room name, type, connections count, and array count (i.e. index) as user's room name,
	//type, connections count, and array count so that the user's current room params are the start room params
	strcpy(roomArray[userIdx].roomName, roomArray[startIdx].roomName);
	strcpy(roomArray[userIdx].roomType, roomArray[startIdx].roomType);
	roomArray[userIdx].connectionsCount = roomArray[startIdx].connectionsCount;
	roomArray[userIdx].structArrayCt = userIdx;

	int k;

	//loop through all connections for the start room
	for (k = 0; k < roomArray[startIdx].connectionsCount; k++)
	{
		//copy each connection for the start room as connections in the user's current room
		strcpy(roomArray[userIdx].connectionNames[k], roomArray[startIdx].connectionNames[k]);
	}
}

/*
NAME
isendroom
SYNOPSIS
checks if cur room is end room
DESCRIPTION
checks if end index's room and cur index's room have same, end room type. returns true if yes, false if no
*/
int IsEndRoom(struct Room roomArray[], int endIndex)
{
	//use an int for bool since using raw C. 0 = false, 1 = true. initialize to false.
	int isEndRoom = 0; 	
	int userIndex = 7; //user is always the last struct in the array of 8, i.e. position 7 

	//if user's current room type is end room
	if(strcmp(roomArray[endIndex].roomType, roomArray[userIndex].roomType) == 0) //== 0 means they matched
	{
		//double check user's room type is END
		if(strcmp(roomArray[userIndex].roomType, END_ROOM) == 0)
		{
			//set isEndRoom bool to 1 (since user's current room type is end room so end room has been reached)
			isEndRoom = 1;
		}
		else //catch error where user's room type isn't the end room even tho index is end index somehow
		{
			printf("\nERROR, end room type matches user's room type but user's room type not END_ROOM\n");
			exit(1);
		}
	}

	return isEndRoom;
}

/*
NAME
inputvalidation
SYNOPSIS
validates user input for next room
DESCRIPTION
checks if the user's input is valid (matches one of the connection room options). if yes, returns true
otherwise returns false
*/
int InputValidation(struct Room roomArray[], int userIndex, char *userStringInput)
{
	//use int since raw c doesn't have bool. 0 = false, 1 = true.
	//initialize to false.
	int inputBool = 0;
	int k;
	
	//loop through all connections for user's current room
	for (k = 0; k < roomArray[userIndex].connectionsCount; k++)
	{
		//if the user's input exactly matches a room name the user's current room is connected to, input is valid
		if(strcmp(roomArray[userIndex].connectionNames[k], userStringInput) == 0) //== 0 means strings matched
		{
			//set bool to true and break out of loop since input matched a valid connection available to user
			inputBool = 1;

			//break out of loop since input has been found to be valid
			k = roomArray[userIndex].connectionsCount + 100;
		}
	}

	return inputBool;
}

/*
NAME
setnewlocation
SYNOPSIS
sets cur room to a new room
DESCRIPTION
sets all params of users current room to the new room user is moving to
*/
void SetNewLocation(struct Room roomArray[], int  userIndex, char *userStringInput)
{
	//search for which location is the new one
	int k;
	int newLocIndex; //will be index of the user's new location

	//loop through all rooms
	for(k = 0; k < NUM_ROOMS; k++)
	{
		//if the user's input matches a room's name, that index becomes the user's new index
		if(strcmp(userStringInput, roomArray[k].roomName) == 0) //0 means names match
		{	
			//set current index to user's new index
			newLocIndex = k;
			
			//break out of loop
			k = NUM_ROOMS + 100;
		}
	}

	//reset all connections (to account for case of new room having fewer connections than the current one)
	ResetEmptyConnections(roomArray, userIndex);
	
	//set all attributes of current room to new room, name type and connections count
	strcpy(roomArray[userIndex].roomName, roomArray[newLocIndex].roomName);
	strcpy(roomArray[userIndex].roomType, roomArray[newLocIndex].roomType);
	roomArray[userIndex].connectionsCount = roomArray[newLocIndex].connectionsCount;

	int m;

	//loop through all connections of the new location
	for(m = 0; m < roomArray[newLocIndex].connectionsCount; m++)
	{
		//set all connections from the new location as the user's current connections
		strcpy(roomArray[userIndex].connectionNames[m], roomArray[newLocIndex].connectionNames[m]);
	}
	
}

/*
NAME
playgame
SYNOPSIS
primary gameplay function
DESCRIPTION
keeps track of number of steps and rooms visited, takes in user input, has primary gamplay loop that goes until
max steps reached or user finds end room. prints message to user upon finding end room, num steps taken, and 
path taken. uses mutexes and threads and gets current time/date if user enters "time".
*/
void PlayGame(struct Room roomArray[], int startIdx, int endIdx, int userIdx)
{
	//info for steps to use mutexes and threads in this assignment adapted from
	//(in addition to the lecture) psuedocode provided by the instructor/TAs on
	//a Piazza post from the Fall 2018 term.
	
	//lock mutex
	pthread_mutex_lock(&myMutex);

	//create time thread
	pthread_t threads[2];

	//create int to check if thread creation successful
	int result_code;

	//create thread and get return value in result_code (to check for success or not).
	//call TimeKeep function 
	result_code = pthread_create(&threads[0], NULL, TimeKeep, NULL);

	//make sure result code == 0 (thread creation successful)
	assert(0 == result_code);

	//max steps will be 100 based on OSU cs344 slack posts about
	//what the TAs recommended be the max step count
	int numSteps = 0;
	int maxSteps = 100;
	
	//bool to keep track of whether user entered time or valid room
	int isTime = 0;

	//create array to hold the names of rooms visited.
	char **roomsVisited;
	roomsVisited = malloc((maxSteps + 1) * sizeof(char*));
	
	//check that malloc is successful (NULL means unsuccessful)
	if (roomsVisited == NULL)
	{
		printf("ROOMS VISITED ARRAY ERROR, UNABLE TO ALLOCATE\n");
		exit(1);
	}

	//create string for user's input
	char *userInput;
	userInput = (char *)malloc(32 * sizeof(char));

	//check that malloc was successful (NULL means unsuccessful)
	if (userInput == NULL)
	{
		printf("STRING ERROR, UNABLE TO ALLOCATE\n");
		exit(1);
	}	

	//main gameplay loop, loops while current num steps is less than max and while
	//the current room is not the end room (i.e. IsEndRoom function returns false)
	while((numSteps < maxSteps) && (IsEndRoom(roomArray, endIdx) == 0))
	{ 
		//get next room to go to as input from the user, and validate the input	
		do{  
			//if user entered a valid room (i.e. isTime is false)
			if(isTime == 0)
			{
				//print current room interface/prompt
				CurRoomPrint(roomArray, userIdx);
			}			
			//else the user entered "time" instead of a room. 
			else
			{
				//since user entered time, print "where to" prompt instead of full game prompt (as shown
				//in example for the assignment)
				printf("WHERE TO? >");
				
				//reset isTime to false
				isTime = 0;
			}
			
			//getline information adapted from:
			//https://c-for-dummies.com/blog/?p=1112

			//create buffer and buffer size and chars. malloc buffer.
			char *buffer;
			size_t bufsize = 32;
			size_t characters;
			buffer = (char *)malloc(bufsize * sizeof(char));

			//check that malloc was successful (NULL means unsuccessful)
			if (buffer == NULL)
			{
				printf("BUFFER ERROR, UNABLE TO ALLOCATE\n");
				exit(1);
			}

			//use getline to get characters of buffer size into buffer from standard input and print a newline
			characters = getline(&buffer, &bufsize, stdin);
			printf("\n");
			
			//getline adds a newline to end of string, remove newline w strtok
			//UNLESS only a newline (i.e. "enter") was entered

			//if the user didn't enter just "enter"
			if(strcmp(buffer, "\n") != 0)
			{
				//remove newline from buffer w strtok
				char *bufferNoNewLine = strtok(buffer, "\n");

				//copy the buffer w newline removed into a variable that will still
				//exist outside of this loop. even if bad data is entered, userInput
				//will only hold the final (and thus valid) input.
				//check with input validation again so that case where just enter is hit
				//and causes errors with strcpy is prevented
				if((InputValidation(roomArray, userIdx, bufferNoNewLine) == 1) || (strcmp(bufferNoNewLine, "time") == 0))
				{
					//set user input to the buffer w newline removed
					strcpy(userInput, bufferNoNewLine);
				}			
				else
				{
					//in case user put in invalid input so do while loop doesn't crash
					strcpy(userInput, "INVALID");

					//print error message to the user
					printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
				}
			}
			else
			{
				//in case user put in only \n, set userInput to something but do
				//nothing else (can't check InputValidation with newline removed)
				strcpy(userInput, "INVALID");

				//print error message to the user
				printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			}

			//free buffer memory and set to NULL
			free(buffer);
			buffer = NULL;			

		//do while loop conditions, this do while loop to get user input will continue looping and re-prompting
		//the user as long as the input validation is false (i.e. user put in invalid connecting room input), 
		//and doesn't enter "time", 
		}while((InputValidation(roomArray, userIdx, userInput) == 0) && (strcmp(userInput, "time") != 0)); //== 0 means input was not valid or that user entered "time"
		
		//now that have obtained valid user input, need to set current room to new room location
		if(strcmp(userInput, "time") != 0) //if user didn't enter "time"
		{
			//set user's current location to the new connection location designated by the user
			SetNewLocation(roomArray, userIdx, userInput);

			//increment steps since have moved to a new room successfully
			//(as noted in the assignment instructions, the number of STEPS taken is being
			//counted, NOT the number of rooms visited
			numSteps++;

			//malloc memory for the rooms visited array
			roomsVisited[numSteps - 1] = malloc(32 * sizeof(char));

			//copy the visited room name to the roomsVisited array to keep track of visited rooms
			strcpy(roomsVisited[numSteps - 1], roomArray[userIdx].roomName);
		}
		else //if user entered "time"
		{
			//unlock mutex
			pthread_mutex_unlock(&myMutex);

			//join time thread (this thread dies after finishes executing)
			result_code = pthread_join(threads[0], NULL);
		
			//make sure result code == 0 (thread join successful)
			assert(0 == result_code);
			
			//lock mutex again
			pthread_mutex_lock(&myMutex);

			//create another time thread
			result_code = pthread_create(&threads[1], NULL, TimeKeep, NULL);

			//make sure result code == 0 (thread creation successful)
			assert(0 == result_code);
			
			//set isTime bool to true
			isTime = 1;

			//read in the date/time info from the currentTime.txt file
			//fopen use adapted from my own work created 10/25/18 as well as
			//https://www.tutorialspoint.com/c_standard_library/c_function_fopen.htm

			//create file stream
			FILE *timeFile;

			//open the time file to be read
			//"r" means read only
			timeFile = fopen("currentTime.txt", "r");
			
			if(timeFile == NULL) //if time file couldn't be opened
			{
				perror("could not open currentTime.txt file correctly");
				exit(1);
			}

			//create string for the time string found in the time file
			char timeFileString [1000];

			//using fgets to read a file adapted from:
			//http://www.cplusplus.com/reference/cstdio/fgets/ and my own work (10/25/18)
			if(fgets(timeFileString, 1000, timeFile) == NULL) //check that there's a string in the time file to be read. NULL means empty/error
			{
				perror("could not open currentTime.txt file correctly to read the time string");
				exit(1);
			}
			
			//remove trailing \n that fgets adds and set to null terminator instead
			timeFileString[strcspn(timeFileString, "\n")] = '\0';

			//print time to user plus two newlines
			printf("%s\n\n", timeFileString);

			//close the time file
			fclose(timeFile);	
		}
	}

	//check if user's step count is greater than/equal to the max step count
	if(numSteps >= maxSteps) //if game ended because user took too many steps looking for end room
	{
		//print error message to the user
		printf("GAME OVER. Sorry, you exceeded the max number of steps (100) without finding\n");
		printf("the end room and lost the game. Please play again to try and find the end room.\n\n");
	}
	else //else the game ended because user has found the end room and won the game
	{
		//print winning message to the user
		printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
		printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);

		int k;

		//looop through the number of steps (i.e. num of rooms visited
		for(k = 0; k < numSteps; k++)
		{
			//print each room visited
			printf("%s\n", roomsVisited[k]);

			//after printing, free the visited room
			free(roomsVisited[k]);
		}
	}

	//free rooms visited and user's input
	free(roomsVisited);
	free(userInput);
}

/*
NAME
curroomprint
SYNOPSIS
prints cur room info
DESCRIPTION
prints the cur room info in the program's specified format
*/
void CurRoomPrint(struct Room roomArray[], int userIndex)
{
	
	//print the user's current location (i.e. room name)
	printf("CURRENT LOCATION: %s\n", roomArray[userIndex].roomName);

	//print possible connections
	printf("POSSIBLE CONNECTIONS:");

	int k;
	
	//print each connection for the curr room. loop through all connections in user current room
	for(k = 0; k < roomArray[userIndex].connectionsCount; k++)
	{
		//if haven't reached last connection
		if(k != (roomArray[userIndex].connectionsCount - 1))
		{
			//print connection name
			printf(" %s,", roomArray[userIndex].connectionNames[k]);
		}
		//if last connection
		else
		{
			//print connection name plus a period and newline
			printf(" %s.\n", roomArray[userIndex].connectionNames[k]);
		}
	}

	//print input prompt message
	printf("WHERE TO? >");
}

/*
NAME
main
SYNOPSIS
main function, calls other functions
DESCRIPTION
creates room struct, calls other functions, sets start and end room indices, frees room memory
*/
int main ()
{
	//get most recent directory's name
	char *directoryName = GetDirName();

	//create array of structs for the 7 rooms (plus the user's current room)
	struct Room room[NUM_ROOMS + 1];
	
	//allocate memory for the rooms (including current user's location)
	CallocRooms(room);

	//initalize rooms with info from the input files
	InitializeRooms(directoryName, room);

	//get starting and ending indices (i.e. array index of the room that's 
	//the starting room and the room that's the ending room)
	int startIndex = GetStartIndex(room);
	int endIndex = GetEndIndex(room);
	int userIndex = NUM_ROOMS; //user's room is last in the array of structs

	//initialize the user's room
	InitializeUserStart(room, userIndex, startIndex); 

	//begin gameplay
	PlayGame(room, startIndex, endIndex, userIndex);
	
	//free memory allocated for the rooms
	FreeRooms(room);

	return 0;
}
