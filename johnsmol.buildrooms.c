/*
Name: Molly Johnson
ONID: johnsmol
CS 344 Fall 2018
Assignment 2
Due: 10/25/18
All information used to create this code is adapted from the OSU CS 344 Fall 2018
lectures and assignment instructions/hints unless otherwise specifically indicated.
Code for building the rooms adapted from the pseudocode provided by the instructor
(2.2 Program Outlining in Program 2). For my version of this program, room files 
will be named the room name of the room they're associated with for simplicity.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>

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

//create Room struct
struct Room
{
	char *roomType;
	int numConnections;
	char *roomName;
	char *connections[MAX_CONNECTIONS];
};

//provide function declarations so don't have to make sure all functions
//declared in perfect order where each function only calls other functions
//already defined
int IsGraphFull(struct Room roomArray[]);
void AddRandomConnection(struct Room roomArray[]);
struct Room InitializeRandomRoom(char *nameChoices[]);
int CanAddConnectionFrom(struct Room roomArray[], int index);
int ConnectionAlreadyExists(struct Room roomArray[], int index1, int index2);
void ConnectRoom(struct Room roomArray[], int index1, int index2);
int IsSameRoom(struct Room roomArray[], int index1, int index2);
char *GetFileName();
int GetRandomNum(int minNum, int maxNum);
void RandomStartEnd(struct Room roomArray[NUM_ROOMS], int *startIn, int *endIn);
void InitializeAllRooms(struct Room roomArray[], char *nameChoices[]);
void Output(struct Room roomArray[], int index, char *typeRoom, char *directoryName);

/*
NAME
IsGraphFull-figure out if the graph is full or not
SYNOPSIS
Returns true (1) if all rooms have less than  6 outbound connections, otherwise
returns false (0).
DESCRIPTION
cycle thru all room connections and see if they have 6 or less, return true if have 6 otherwise false
*/
int IsGraphFull(struct Room roomArray[])
{
	//int to use as a "bool" since raw C doesn't allow bool varaibles. 1=true, 0=false
	//initialize to start with as false 
	int boolVal = 0;
	int fullRoomCounter=0;

	int i;
	for (i = 0; i < NUM_ROOMS; i++)
	{
		if ((roomArray[i].numConnections < 0) || (roomArray[i].numConnections > MAX_CONNECTIONS))
		{
			printf("error, too many room connections!\n");
			//break out of loop
			i = NUM_ROOMS + 100;
		}
		else if ((roomArray[i].numConnections <= MAX_CONNECTIONS) && (roomArray[i].numConnections >= MIN_CONNECTIONS))
		{
			//if this room is full, increment room counter
			fullRoomCounter++;
		}
	}

	//if full room counter == NUM_ROOMS (7), means graph is full. otherwise, is not.
	//change bool to true if graph is full
	if (fullRoomCounter == NUM_ROOMS)
	{
		boolVal = 1;
	}
	return boolVal;
}

/*
NAME
addrandomconnection
SYNOPSIS
adds a random conneection to a room (and connects that room back to itself)
DESCRIPTION
checks to make sure random room connection is valid (other room isn't
already connected, can't add a connection from it, or if it's the same room)
*/
void AddRandomConnection(struct Room roomArray[])
{
	//get random room index 1
	int room1Index; 

	//initialize bool value to false
	int canAddRoom1 = 0;
	
	while (canAddRoom1 == 0)
	{
		room1Index = GetRandomNum(0, NUM_ROOMS - 1);
		//check if you can add a connection from first room
		canAddRoom1 = CanAddConnectionFrom(roomArray, room1Index);
	}

	//get random room index 2
	int room2Index;

	//use do while loop to keep getting random second room 
	//until a valid one is found (i.e. one that can have a connection added
	//is not same room as room1, and doesn't already have a connection
	//with room 1)
	do
	{
		//get random room index 2
		room2Index = GetRandomNum(0, NUM_ROOMS - 1);

	}while(CanAddConnectionFrom(roomArray, room2Index) == 0 || IsSameRoom(roomArray, room1Index, room2Index) == 1 || 
		ConnectionAlreadyExists(roomArray, room1Index, room2Index) == 1);

	//connect rooms
	ConnectRoom(roomArray, room1Index, room2Index);
}

/*
NAME
initializerandomroom
SYNOPSIS
sets a random room up with default values
DESCRIPTION
creates a struct of a room, gives some default values (a random name,
sets it as "mid room" type, and 0 connections). returns the struct.
*/
struct Room InitializeRandomRoom(char *nameChoices[])
{
	struct Room randRoom;

	//get random index
	int randIndex = GetRandomNum(0, NUM_NAME_OPTIONS - 1);

	//initialize all room types to MID_ROOM (one start and one end will be randomized after initialization)
	randRoom.roomType = MID_ROOM;

	//initialize num connections to 0 to start with since no rooms are connected yet
	randRoom.numConnections = 0;

	//initialize room name to a random name (of the ten names available) based on the random index
	randRoom.roomName = nameChoices[randIndex];

	//initialize the max number of connections for the room all to "empty" since
	//no rooms have been connected yet
	int i = 0;
	for(i = 0; i < MAX_CONNECTIONS; i++)
	{
		randRoom.connections[i] = EMPTY;
	}

	return randRoom;
}

/*
NAME
canaddconnectionfrom
SYNOPSIS
checks if ou can add a connection from a room
DESCRIPTION
checks if the max num of connections is already reached. if it is, return false. otherwise true;
*/
int CanAddConnectionFrom(struct Room roomArray[], int index)
{
	//int to use as a "bool" since raw C doesn't allow bool variables. 1=true, 0=false
	//initialize to true
	int boolVal = 1;

	if(roomArray[index].numConnections >= MAX_CONNECTIONS)
	{
		//if max number of connections is already reached, set to false
		//since can't add any more
		boolVal = 0;
	}

	return boolVal;
}

/*
NAME
connectionalreadyexists
SYNOPSIS
checks to see if a connection between two rooms already exists
DESCRIPTION
checks all connections and sees if the two rooms are alread connected. if connected
returns true, otherwise false
*/
int ConnectionAlreadyExists(struct Room roomArray[], int index1, int index2)
{
	//int to use as a "bool" since raw C doesn't allow bool varaibles. 1=true, 0=false
	//initialize to false
	int boolVal = 0;

	int i;
	for(i = 0; i < MAX_CONNECTIONS; i++)
	{
		//check if the rooms are connected by using strcmp to compare each element of the "connections" array.
		//0 means the connection matched.
		if(strcmp(roomArray[index1].connections[i], roomArray[index2].roomName) == 0)
		{
			//set boolval to true since the connection already exists
			boolVal = 1;

			//break out of loop
			i = MAX_CONNECTIONS + 100;
		}
	}
	return boolVal;
}

/*
NAME
connectroom
SYNOPSIS
connects two rooms
DESCRIPTION
finds the next empty connection for a room and connects it to another room
*/
void ConnectRoom(struct Room roomArray[], int index1, int index2)
{
	int m;
	for (m = 0; m < MAX_CONNECTIONS; m++)
	{
		//make connection at "empty" connection for the room
		if(strcmp(roomArray[index1].connections[m], EMPTY) == 0)
		{
			roomArray[index1].connections[m] = roomArray[index2].roomName;	

			//break out of loop
			m = MAX_CONNECTIONS + 100;
			roomArray[index1].numConnections++;
		}
	}

	for (m = 0; m < MAX_CONNECTIONS; m++)
	{
		//make connection at "empty" connection for the room
		if(strcmp(roomArray[index2].connections[m], EMPTY) == 0)
		{
			roomArray[index2].connections[m] = roomArray[index1].roomName;	
			//break out of loop
			m = MAX_CONNECTIONS + 100;
			roomArray[index2].numConnections++;
		}
	}
}

/*
NAME
issameroom
SYNOPSIS
checks if two rooms are the same
DESCRIPTION
checks room names to see if two rooms are the same. if so returns true, otherwise false
*/
int IsSameRoom(struct Room roomArray[], int index1, int index2)
{
	//int to use as a "bool" since raw C doesn't allow bool varaibles. 1=true, 0=false
	//initialize to false
	int boolVal = 0;

	//use strcmp to compare the name strings (return of 0 means the strings were equal)
	if (strcmp(roomArray[index1].roomName, roomArray[index2].roomName) == 0)
	{
		boolVal = 1;
	}

	return boolVal;
}

/*
NAME
getfilename
SYNOPSIS
provides the name of a folder 
DESCRIPTION
gets my onid and process id (pid) to add to the folder name for the rooms files

getting process id and creating a folder name with it adapted from:
http://man7.org/linux/man-pages/man2/getpid.2.html
converting an int to a string in C adapted from:
https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
*/
char *GetFolderName()
{
	//use static variable for folder name so can be returned and exist after 
	//the function exits
	static char folderName[] = "johnsmol.rooms.";

	//get process id (an int)
	int pid = getpid();

	//convert int pid into string pid
	int length = snprintf( NULL, 0, "%d", pid );
	char *stringPID = malloc( length + 1 );
	snprintf( stringPID, length + 1, "%d", pid );
	char *copyStringPID = stringPID;

	//concatenate the folder name with the string version of the pid
	strcat(folderName, copyStringPID);
	free(stringPID);

	return folderName;
}

/*
NAME
getrandomnum
SYNOPSIS
gets a random num
DESCRIPTION
uses rand() to get a random num between a min value and max value passed in. returns it.
*/
int GetRandomNum(int minNum, int maxNum)
{
	int randNum = (rand() % (maxNum - minNum + 1) + minNum);

	return randNum;
}

/*
NAME
randomstartend
SYNOPSIS
provides a random start room and end room
DESCRIPTION
gives a random start room index and end room index for the rooms
*/
void RandomStartEnd(struct Room roomArray[NUM_ROOMS], int *startIn, int *endIn)
{
	int randStart = GetRandomNum(0, NUM_ROOMS - 1);
	int randEnd;
	do{
		randEnd = GetRandomNum(0, NUM_ROOMS - 1);
	}while(randStart == randEnd);

	roomArray[randStart].roomType = START_ROOM;
	roomArray[randEnd].roomType = END_ROOM;
	*startIn = randStart;
	*endIn = randEnd;
}

/*
NAME
initializeallrooms
SYNOPSIS
initializes all the rooms 
DESCRIPTION
initializes each room by calling initializerandom room , then checks that
it's not a duplicate
*/
void InitializeAllRooms(struct Room roomArray[], char *nameChoices[])
{
	int j = 0;
	int k = 0;
	for (j = 0; j < NUM_ROOMS; j++)
	{
		//initialize each room randomly
		roomArray[j] = InitializeRandomRoom(nameChoices);

		//if more than one room has been created, check that the current room name hasn't already been used
		if (j > 0)
		{
			for (k = 0; k < j; k++)
			{
				//check if is a duplicate room (1 == true, is same room)
				if (IsSameRoom(roomArray, j, k) == 1)
				{
					//if the room is a duplicate, break out of loop and set index j back one so a new
					//(non-duplicate) random room name can be selected.
					k = j + 100;
					j = j - 1;
				}
			}
		}
	}
}

/*
NAME
output
SYNOPSIS
outputs all room info
DESCRIPTION
outputs all of the room info to their appropriate files and in the appropriate directory
*/
void Output(struct Room roomArray[], int index, char *typeRoom, char *directoryName)
{
	//output room contents to their appropriate files in the newly created directory
	//file output adapted from (in addition to lectures):
	//https://www.cs.bu.edu/teaching/c/file-io/intro/ and
	//https://www.tutorialspoint.com/cprogramming/c_file_io.htm
	int k;
	FILE *outputFile;
	
	//choose array size guaranteed to fit directory name plus pid plus max file name
	//of 8 letters (as specified in the assignment instructions)
	char fileName[100];
	strcpy(fileName, directoryName);
	char slash[] = "/";
	strcat(fileName, slash);
	strcat(fileName, roomArray[index].roomName);

	//create file name
	outputFile = fopen(fileName, "a+"); 
	if(outputFile == NULL)
	{
		printf("\nFile would not open.\n");
	}
	else
	{
		//output room name
		fprintf(outputFile, "ROOM NAME: ");
		fprintf(outputFile, "%s\n", roomArray[index].roomName);

		//output all room connections
		for (k = 0; k < roomArray[index].numConnections; k++)
		{
			fprintf(outputFile, "CONNECTION %d: ", k+1);
			fprintf(outputFile, "%s\n", roomArray[index].connections[k]);
		}
	
		//output room type
		fprintf(outputFile, "ROOM TYPE: ");
		fprintf(outputFile, "%s\n", typeRoom);

		fclose(outputFile);
	}
}

/*
NAME
main
SYNOPSIS
is the main function that calls other functions
DESCRIPTION
calls other functions, creates a new directory for the rooms, provides rand time seed,
has the main loop to see if the graph is full or not and keep adding connections til
it is, and calls the output functions to get all the room info to files.
*/
int main ()
{
	//creating new directory in C method adapted from:
	//https://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c and
	//https://www.quora.com/How-do-I-create-directories-in-C-programming and
	//https://linux.die.net/man/3/mkdir. 0777 is an octal # to indicate the file has
	//r, w, and x permissions.
	char *directoryName = GetFolderName();
	mkdir(directoryName, 0777);

	//seed srand one time, then use rand() with mod later to get rand nums
	//random number formula used after seeding: 
	//randNum = (rand() % (maxRandNum - minRandNum + 1) + minRandNum) 
	//method for obtaining random numbers adapted from:
	///https:stackoverflow.com/questions/822323/how-to-generate-a-random-int-in-c
	srand(time(NULL));

	//create array of 10 room name options
	char *nameOptions[NUM_NAME_OPTIONS] = { "KIRSTYS", "MOLLYS", "SOPHIES",
			"BILLYS", "OTIES", "PERCYS", "MUNCHS", "MIKAS", "MAXS", "CHLOES" };

	//create array of structs for the 7 rooms
	struct Room room[NUM_ROOMS];

	//initialize all rooms with starting values
	InitializeAllRooms(room, nameOptions);

	//pick one room to be the start room randomly 
	//and one room to be the end room randomly.
	//also keep track of start and ending index using pointers
	int startIndex, endIndex;
	RandomStartEnd(room, &startIndex, &endIndex);
	
	//create all connections in graph. 0=false in the while loop condition (since raw
	//C doesn't allow bool variables)
	while (IsGraphFull(room) == 0)
	{
		AddRandomConnection(room);
	}

	//print starting room
	Output(room, startIndex, START_ROOM, directoryName);

	//print middle rooms
	int n;
	for (n = 0; n < NUM_ROOMS; n++)
	{
		if ((n != startIndex) && (n != endIndex)) 
		{
			Output(room, n, MID_ROOM, directoryName);
		}
	}
	
	//print ending room
	Output(room, endIndex, END_ROOM, directoryName);
	
	return 0;
}
