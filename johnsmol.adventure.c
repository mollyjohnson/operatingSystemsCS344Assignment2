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
//in the TimeKeep function unless either the mutex was passed in as an argument or made global,
//and multiple resources found online mentioned passing mutexes as arguments as bad form, see
//https://stackoverflow.com/questions/34553232/why-is-passing-references-to-the-mutex-class-not-a-good-design/34619777
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

//create Room struct
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
void Print(struct Room roomArray[]);
void InitializeUserStart(struct Room roomArray[], int userIdx, int startIdx);
void ResetEmptyConnections(struct Room roomArray[], int roomIndex);
void PlayGame(struct Room roomArray[], int startIdx, int endIdx, int userIdx);
void CurRoomPrint(struct Room roomArray[], int userIndex);
int InputValidation(struct Room roomArray[], int userIndex, char *userStringInput);
void SetNewLocation(struct Room roomArray[], int  userIndex, char *userStringInput);
void* TimeKeep(void* argument);
char *ReplaceString(char *str, char *orig, char *rep);

/*
NAME

SYNOPSIS

DESCRIPTION

*/
void* TimeKeep(void* argument)
{
	//lock mutex (this call will block if called when mutex is locked)
	pthread_mutex_lock(&myMutex);

	//strftime use adapted from:
	//https://linux.die.net/man/3/strftime and
	//https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
	time_t rawtime;
	struct tm *info;
	char buffer[256];
	memset(buffer, '\0', sizeof(buffer));
	char formattedBuffer[256];
	memset(formattedBuffer, '\0', sizeof(formattedBuffer));

	time(&rawtime);
	info = localtime(&rawtime);
	strftime(buffer, 256, "%I:%M%p, %A, %B %d, %Y ", info);

	//check if time string has a leading zero (shouldn't according to the given example)
	if(buffer[0] == '0')
	{
		//shift chars to the left to eliminate leading zero (as that's the format in given example)
		int k;
		k = 0;
		while(buffer[k] !='\0')
		{
			buffer[k] = buffer[k + 1];
			k++;
		}

	}

	//replace upper case AM or PM with lower case am or pm (as that's the format in given example)
	//check if is AM with strstr (if no "AM" in string, strstr will return null)
	if(strstr(buffer, "AM") != NULL)
	{
		char *tempAM = ReplaceString(buffer, "AM", "am");
		strcpy(formattedBuffer, tempAM);
	}
	//check if is PM with strstr (if no "PM" in string, strstr will return null)
	else if(strstr(buffer, "PM") != NULL)
	{
		char *tempPM = ReplaceString(buffer, "PM", "pm");
		strcpy(formattedBuffer, tempPM);
	}
	else
	{
		perror("time not given in am or pm, error!");
		exit(1);
	}

	//getting current directory adapted from:
	//https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
	char cwd[500];
	if(getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd() error");
		exit(1);
	}

	//change directory back one step from rooms directory (to directory with game files)
	chdir("..");
		
	//file output adapted from (in addition to lectures):
	//https://www.cs.bu.edu/teaching/c/file-io/intro/ and
	//https://www.tutorialspoint.com/cprogramming/c_file_io.htm
	//file permissions information from:
	//https://dineshbhopal.wordpress.com/2014/12/03/php-r-r-w-w-file-handling/
	//"w" used instead of "a" so that only the most updated date/time is written
	//to the file (i.e. a new file is created or an old one is overwritten) instead
	//of appended to.
	FILE *outputFile;
	outputFile = fopen("currentTime.txt", "w+");

	if(outputFile == NULL)
	{
		perror("output file would not open.");
		exit(1);
	}

	//print the properly formatted time and date to the file currentTime.txt
	fprintf(outputFile, formattedBuffer);
	
	//add newline character so if user enters "time" again the new time will
	//go on the next line
	//fprintf(outputFile, "\n");

	fclose(outputFile);

	//change directory back to room directory
	//chdir(cwd);
	//char newCwd[500];
	//if(getcwd(newCwd, sizeof(newCwd)) == NULL)
	//{
	//	perror("getcwd() error");
	//	exit(1);
	//}

	//unlock mutex
	pthread_mutex_unlock(&myMutex);

	return NULL;
}

/*
NAME
replacestring
SYNOPSIS
replaces a string with a new string
DESCRIPTION
replaces specified substring of a string with a new substring.
Adapted from my own work 11/14/18 and:
https://stackoverflow.com/questions/32413667/replace-all-occurrences-of-a-substring-in-a-string-in-c and
https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/
*/
char *ReplaceString(char *str, char *orig, char *rep)
{
	char *result;
	int i = 0;
	int cnt = 0;

	//save lengths of the replacement substring and the original substring
	int newWlen = strlen(rep);
	int oldWlen = strlen(orig);

	//go through each char in the original long string, to check for occurrences of the original substring
	for(i = 0; str[i] != '\0'; i++)
	{
		if(strstr(&str[i], orig) == &str[i])
		{
			cnt++;
			i += oldWlen - 1;
		}
	}

	result = (char *)malloc(i + cnt *(newWlen - oldWlen) + 1);
	i = 0;

	//replace each occurrence of the orig substring with the new substring
	while (*str)
	{
		if(strstr(str, orig) == str)
		{
			strcpy(&result[i], rep);
			i += newWlen;
			str += oldWlen;
		}
		else
		{
			result[i++] = *str++;
		}
	}

	//set result to a new string to be returned so result memory can be freed
	result[i] = '\0';
	static char returnStr[256];
	memset(returnStr, '\0', sizeof(returnStr));
	strcpy(returnStr, result);

	//free memory
	free(result);
	result = NULL;

	//return newly expanded string
	return returnStr;
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
	for(k = 0; k < MAX_CONNECTIONS; k++)
	{
		strcpy(roomArray[roomIndex].connectionNames[k], EMPTY);
	}
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
	DIR *newestDir;
	struct dirent *currentFile;
	newestDir = opendir(directoryNameIn);
	
	//keep track of which file is being read
	int fileNum = 0;

	//if directory was able to be opened	
	if(newestDir > 0)
	{
		//while there are still files in the directory to be checked
		while ((currentFile = readdir(newestDir)) != NULL)
		{
			//fseek() information obtained/adapted from"
			//https://www.tutorialspoint.com/c_standard_library/c_function_fseek.htm and
			//https://www.geeksforgeeks.org/fseek-in-c-with-example/
			//strcmp == 0 means the strings matched
			if((strcmp(currentFile->d_name, ".") != 0) && (strcmp(currentFile->d_name, "..") != 0))
			{
				char directory[] = "./";
				strcat(directory, directoryNameIn);
				const char *filePath = directory;
				chdir(filePath);
				char *roomFileName = currentFile->d_name;
				int file_descriptor;
				file_descriptor = open(roomFileName, O_RDWR); 

				if (file_descriptor < 0)
				{
					fprintf(stderr, "could not open %s\n", roomFileName);
					exit(1);
				}
				else
				{
					FILE *roomFile;
					roomFile= fopen(roomFileName, "a+");

					if(roomFile == NULL)
					{
						printf("\nFile would not open.\n");
						exit(1);
					}
					else
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
						char *token = ":";
						char *index;
						int lineCount = 0;
						int connectionsCounter = 0;
						while(fgets(myString, 100, roomFile))
						{
							index = strstr(myString, token);
							index +=2;
							char *savedString = strtok(index, "\n"); 
							if(lineCount == 0)
							{
								strcpy(roomArray[fileNum].roomName,savedString);
							}
							else if((strcmp(savedString, END_ROOM) == 0) || (strcmp(savedString, MID_ROOM) == 0) || (strcmp(savedString, START_ROOM) == 0))
							{
								strcpy(roomArray[fileNum].roomType, savedString);
							}
							else
							{
								strcpy(roomArray[fileNum].connectionNames[connectionsCounter], savedString);
								connectionsCounter++;
							}
							lineCount++;
						}
						roomArray[fileNum].connectionsCount = connectionsCounter;
						fclose(roomFile);
					}
					close(file_descriptor);
					roomArray[fileNum].structArrayCt = fileNum;
					fileNum++;
				}
			}
		}
		//close the open directory
		closedir(newestDir);
	}
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
	for(k = 0; k < NUM_ROOMS + 1; k++)
	{
		free(roomArray[k].roomName);
		roomArray[k].roomName = NULL;
		free(roomArray[k].roomType);
		roomArray[k].roomType = NULL;
		for(h = 0; h < MAX_CONNECTIONS; h++)
		{
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
gets the directory name the file are in and returns this name
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
					newestDirTime = (int)directoryAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDirectory->d_name);
				}
			}
		}
		//close the open directory
		closedir(dirToCheck);
	}
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
	for(i = 0; i < NUM_ROOMS + 1; i++)
	{
		roomArray[i].roomName = calloc(16, sizeof(char));
		roomArray[i].roomType = calloc(16, sizeof(char));
		roomArray[i].connectionsCount = 0;
		roomArray[i].structArrayCt = 0;
		for(j = 0; j < MAX_CONNECTIONS; j++)
		{
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
	for (k = 0; k < NUM_ROOMS; k++)
	{
		if(strcmp(roomArray[k].roomType, START_ROOM) == 0)
		{
			//end index found, can be assigned and returned
			startIndex = k;

			//break out of loop
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
	for (k = 0; k < NUM_ROOMS; k++)
	{
		if(strcmp(roomArray[k].roomType, END_ROOM) == 0)
		{
			//end index found, can be assigned and returned
			endIndex = k;

			//break out of loop
			k = NUM_ROOMS + 100;
		}
	}

	return endIndex;
}
/*
NAME
temp print funct for testing
SYNOPSIS
used for testing all rooms
DESCRIPTION
prints all parameters for all rooms
*/
void Print(struct Room roomArray[])
{
	int i = 0;
	int j = 0;
	for (i = 0; i < NUM_ROOMS + 1; i++)
	{
		printf("%s\n", roomArray[i].roomName);
		printf("%s\n", roomArray[i].roomType);
		printf("%d\n", roomArray[i].connectionsCount);
		printf("%d\n", roomArray[i].structArrayCt);

		for(j = 0; j < MAX_CONNECTIONS; j++)
		{
			printf("%s\n", roomArray[i].connectionNames[j]);
		}
		printf("\n");
	}
}

/*
NAME
initializeuserstard
SYNOPSIS
initializes the user's starting location
DESCRIPTION
finds the starting room and sets the user's current room to that start room
*/
void InitializeUserStart(struct Room roomArray[], int userIdx, int startIdx)
{
	strcpy(roomArray[userIdx].roomName, roomArray[startIdx].roomName);
	strcpy(roomArray[userIdx].roomType, roomArray[startIdx].roomType);
	roomArray[userIdx].connectionsCount = roomArray[startIdx].connectionsCount;
	roomArray[userIdx].structArrayCt = userIdx;

	int k;
	for (k = 0; k < roomArray[startIdx].connectionsCount; k++)
	{
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
	int userIndex = 7; //user is the last struct in the array of 8, i.e. position 7 

	if(strcmp(roomArray[endIndex].roomType, roomArray[userIndex].roomType) == 0) //== 0 means they matched
	{
		//double check user's room type is END
		if(strcmp(roomArray[userIndex].roomType, END_ROOM) == 0)
		{
			isEndRoom = 1;
		}
		else
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
	
	//check 
	for (k = 0; k < roomArray[userIndex].connectionsCount; k++)
	{
		if(strcmp(roomArray[userIndex].connectionNames[k], userStringInput) == 0) //== 0 means strings matched
		{
			//set bool to true and break out of loop
			inputBool = 1;
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
	int newLocIndex;
	for(k = 0; k < NUM_ROOMS; k++)
	{
		if(strcmp(userStringInput, roomArray[k].roomName) == 0) //names match
		{	
			newLocIndex = k;
			
			//break out of loop
			k = NUM_ROOMS + 100;
		}
	}

	//reset all connections (in case of new room having fewer than the current one)
	ResetEmptyConnections(roomArray, userIndex);
	
	//set all attributes of current room to new room
	strcpy(roomArray[userIndex].roomName, roomArray[newLocIndex].roomName);
	strcpy(roomArray[userIndex].roomType, roomArray[newLocIndex].roomType);
	roomArray[userIndex].connectionsCount = roomArray[newLocIndex].connectionsCount;

	int m;
	for(m = 0; m < roomArray[newLocIndex].connectionsCount; m++)
	{
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
path taken
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
	int result_code;
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
	//char *roomsVisited[maxSteps + 1];
	char **roomsVisited;
	roomsVisited = malloc((maxSteps + 1) * sizeof(char*));

	char *userInput;
	userInput = (char *)malloc(32 * sizeof(char));
	if (userInput == NULL)
	{
		printf("STRING ERROR, UNABLE TO ALLOCATE\n");
		exit(1);
	}	

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
			//else the user entered "time" instead of a room. reset isTime to false
			else
			{
				printf("WHERE TO? >");
				isTime = 0;
			}
			//getline information adapted from:
			//https://c-for-dummies.com/blog/?p=1112
			char *buffer;
			size_t bufsize = 32;
			size_t characters;
			buffer = (char *)malloc(bufsize * sizeof(char));
			if (buffer == NULL)
			{
				printf("BUFFER ERROR, UNABLE TO ALLOCATE\n");
				exit(1);
			}
			characters = getline(&buffer, &bufsize, stdin);
			printf("\n");
			
			//getline adds a newline to end of string, remove newline w strtok
			//UNLESS only a newline (i.e. "enter") was entered
			if(strcmp(buffer, "\n") != 0)
			{
				char *bufferNoNewLine = strtok(buffer, "\n");

				//copy the buffer w newline removed into a variable that will still
				//exist outside of this loop. even if bad data is entered, userInput
				//will only hold the final (and thus valid) input.
				//check with input validation again so that case where just enter is hit
				//and causes errors with strcpy is prevented
				if((InputValidation(roomArray, userIdx, bufferNoNewLine) == 1) || (strcmp(bufferNoNewLine, "time") == 0))
				{
					strcpy(userInput, bufferNoNewLine);
				}			
				else
				{
					//in case user put in invalid input so do while loop doesn't crash
					strcpy(userInput, "INVALID");
					printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
				}
			}
			else
			{
				//in case user put in only \n, set userInput to something but do
				//nothing else (can't check InputValidation with newline removed)
				strcpy(userInput, "INVALID");
				printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			}

			free(buffer);
			buffer = NULL;			
		}while((InputValidation(roomArray, userIdx, userInput) == 0) && (strcmp(userInput, "time") != 0)); //== 0 means input was not valid or that user entered "time"
		
		//now that have obtained valid user input, need to set current room to new room location
		if(strcmp(userInput, "time") != 0) //if user didn't enter "time"
		{
			SetNewLocation(roomArray, userIdx, userInput);
			//increment steps since have moved to a new room successfully
			//(as noted in the assignment instructions, the number of STEPS taken is being
			//counted, NOT the number of rooms visited
			numSteps++;
			roomsVisited[numSteps - 1] = malloc(32 * sizeof(char));
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
			FILE *timeFile;

			//"r" means read only
			timeFile = fopen("currentTime.txt", "r");
			
			if(timeFile == NULL)
			{
				perror("could not open currentTime.txt file correctly");
				exit(1);
			}

			char timeFileString [1000];

			//using fgets to read a file adapted from:
			//http://www.cplusplus.com/reference/cstdio/fgets/ and my own work (10/25/18)
			if(fgets(timeFileString, 1000, timeFile) == NULL)
			{
				perror("could not open currentTime.txt file correctly");
				exit(1);
			}
			
			//remove trailing \n that fgets adds and set to null terminator instead
			timeFileString[strcspn(timeFileString, "\n")] = '\0';

			//print time to user
			printf(" %s\n\n", timeFileString);

			fclose(timeFile);	
		}
	}

	if(numSteps >= maxSteps) //if game ended because user took too many steps looking for end room
	{
		printf("GAME OVER. Sorry, you exceeded the max number of steps (100) without finding\n");
		printf("the end room and lost the game. Please play again to try and find the end room.\n\n");
	}
	else //else the game ended because user has found the end room
	{
		printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
		printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);
		int k;
		for(k = 0; k < numSteps; k++)
		{
			printf("%s\n", roomsVisited[k]);
			free(roomsVisited[k]);
		}
	}
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
	printf("CURRENT LOCATION: %s\n", roomArray[userIndex].roomName);
	printf("POSSIBLE CONNECTIONS:");

	int k;
	//print each connection for the curr room
	for(k = 0; k < roomArray[userIndex].connectionsCount; k++)
	{
		if(k != (roomArray[userIndex].connectionsCount - 1))
		{
			printf(" %s,", roomArray[userIndex].connectionNames[k]);
		}
		else
		{
			printf(" %s.\n", roomArray[userIndex].connectionNames[k]);
		}
	}
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

	//temporary print function just to check that output of all rooms is correct
	//Print(room);

	//begin gameplay
	PlayGame(room, startIndex, endIndex, userIndex);
	
	//temporary print function just to check that output of all rooms is correct
	//Print(room);
	
	//free memory allocated for the rooms
	FreeRooms(room);

	return 0;
}
