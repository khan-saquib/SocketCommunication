#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NAMELENGTH 20
#define MESSGSIZE 80 
#define BUFFERSIZE 2500
#define MAX_KNOWN_USERS 100
#define MAX_MESSAGES 10


/**********************The user data base system***************************/
struct user
{
	char name[NAMELENGTH];
	int status;
	char messages[MAX_MESSAGES][MESSGSIZE];
	int messageCount;
	time_t messsageReceiveTime[MAX_MESSAGES];
	char senderUsername[MAX_MESSAGES][NAMELENGTH];
}userData[MAX_KNOWN_USERS];

int totalRecordCount = 0;


/***************************FUNCTION PROTOTYPES*****************************/
//Function of the individual threads
void* handleClient(void *);

//To send and receive data using sockets
int sendAllData(int s, char *buf, int len);
int receiveAllData(int s, char *buf, int len);
 
//Check the "status" of the userName if it exists, 
//creates the entry in the structure if it doesn't exist and sets the status to "1".
int login(char* userName);

//Sets the status of the user to "0".
void logout(int userID);

//Initialises the blank records of all users.
void initialiseDatabase();

//To create records into the database
int addUser(char* name, int status);
int addMessageToUserByID(char* message, char* senderUserName, int userID);
void addMessageToUserByName(char* message, char* senderUserName, char* userName);//If the userName does not exist, create a record and print it.
void addMessageToConnectedUsers(char* message, char* senderUserName);
void addMessageToKnownUsers(char* message, char* senderUserName);

//This function is used by client to split data string into a array of strings.
char* splitStringData(char* bufferData);


int main(int argc, char *argv[])
{
	char     host[80];
	int      sd, sd_current;
	int      port;
	int      *sd_client;
	int      addrlen;
	struct   sockaddr_in sin;
	struct   sockaddr_in pin;
	pthread_t tid;
	pthread_attr_t attr;

	/* check for command line arguments */
	if (argc != 2)
	{
		printf("Usage: server port\n");
		exit(1);
	}

	/* get port from argv */
	port = atoi(argv[1]);


	/* create an internet domain stream socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating socket");
		exit(1);
	}

	/* complete the socket structure */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;  /* any address on this host */
	sin.sin_port = htons(port);        /* convert to network byte order */

	/* bind the socket to the address and port number */
	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		perror("Error on bind call");
		exit(1);
	}

	/* initialise the databse of users to the default values */
	initialiseDatabase();

	/* set queuesize of pending connections */ 
	if (listen(sd, 10) == -1) {
		perror("Error on listen call");
		exit(1);
	}

	/* announce server is running */
	gethostname(host, 80); 
	printf("\nServer is running on %s:%d\n", host, port);


	/* wait for a client to connect */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); /* use detached threads */
	addrlen = sizeof(pin);
	while (1)
	{
		if ((sd_current = accept(sd, (struct sockaddr *)  &pin, (socklen_t*)&addrlen)) == -1) 
		{
			exit(1);
		}

		sd_client = (int*)(malloc(sizeof(sd_current)));
		*sd_client = sd_current;

		pthread_create(&tid, &attr, handleClient, sd_client);
	}

	/* close socket */
	close(sd);
}


void* handleClient(void *arg)
{
	int count=0, userID, option = 7;
	char buf[BUFFERSIZE];  /* used for incoming string, and outgoing data */
	char userName[NAMELENGTH], receiverUserName[NAMELENGTH], message[MESSGSIZE];
	char *temp;

	int sd = *((int*)arg);  /* get sd from arg */
	free(arg);              /* free malloced arg */
	
	/* read user's name from the client */
	if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
	{
      perror("Error on read call");
      exit(1);
	}
	
	strncpy(userName,buf,NAMELENGTH);
	//Login into the database and retain the userID
	if((userID = login(userName)) == -1)
	{
		temp = "LOGIN_FAIL";
		if((count = sendAllData(sd, temp, strlen(temp))) == -1)
		{
			perror("Error on write call");
			exit(1);
		}
		
	}
	else
	{
		temp = "LOGIN_SUCCESS";
		if((count = sendAllData(sd, temp, strlen(temp))) == -1)
		{
			perror("Error on write call");
			exit(1);
		}




		
		
		
		int flag=1;
		while(flag)
		{


			/* Get the operation code to be performed*/
			if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
			{
			  perror("Error on read call");
			  exit(1);
			}

			option = atoi(buf);
			strcpy(buf,"");
			time_t nowTime;
			char timeStr[20];

			//Perform the operation received.
			switch(option)
			{
				case 1:
					buf[0] = '\0';
					//TODO:Add mutex
					for(count=0;count<totalRecordCount;count++)
					{
						strcat(buf, userData[count].name);
						strcat(buf, "|");
					} 
					buf[strlen(buf)-1] = '\0';
					time(&nowTime);
					strcpy( timeStr ,ctime(&nowTime));
					timeStr[strlen(timeStr)-1] = '\0';
					printf("\n%s, %s displays all known users.",timeStr, userData[userID].name);
					//TODO: finish mutex
					if((count = sendAllData(sd, buf, strlen(buf))) == -1)
					{
						perror("Error on write call");
						exit(1);
					}
					break;

				case 2:
					
					buf[0] = '\0';
					//TODO:Add mutex
					for(count=0;count<totalRecordCount;count++)
					{
						if( userData[count].status == 1 )
						{
							strcat(buf, userData[count].name);
							strcat(buf, "|");
						}
					}
					time(&nowTime);
					strcpy( timeStr ,ctime(&nowTime));
					timeStr[strlen(timeStr)-1] = '\0';
					printf("\n%s, %s displays all connected users.",timeStr, userData[userID].name);
					//TODO: finish mutex
					buf[strlen(buf)-1] = '\0';
					if((count = sendAllData(sd, buf, strlen(buf))) == -1)
					{
						perror("Error on write call");
						exit(1);
					}
					break;

				case 3:
					buf[0] = '\0';
					if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
					{
					  perror("Error on read call");
					  exit(1);
					}
					strncpy( receiverUserName , strtok(buf,"|"),NAMELENGTH);
					strncpy( message, strtok(NULL,"|"), MESSGSIZE);
					
					
					printf("\nThis is the message received: %s\n",message);



					time(&nowTime);
					strcpy( timeStr ,ctime(&nowTime));
					timeStr[strlen(timeStr)-1] = '\0';
					printf("\n%s, %s posts a message for %s.",timeStr, userData[userID].name, receiverUserName);

					//TODO:
					addMessageToUserByName(message, userName, receiverUserName);
					//TODO:
					break;

				case 4:
					buf[0] = '\0';
					if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
					{
					  perror("Error on read call");
					  exit(1);
					}
					strncpy( message , strtok(buf,"|"),MESSGSIZE);
					
					//TODO:Add mutex
					for(count=0;count<totalRecordCount;count++)
					{
						if( userData[count].status == 1 )
						{
							addMessageToUserByID( message, userName, count);
						}
					}
					//TODO: finish mutex
					break;

				case 5:
					buf[0] = '\0';
					if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
					{
					  perror("Error on read call");
					  exit(1);
					}
					strncpy( message , strtok(buf,"|"),MESSGSIZE);
					
					//TODO:Add mutex
					for(count=0;count<totalRecordCount;count++)
						addMessageToUserByID( message, userName, count);
					//TODO: finish mutex

					break;
				
				case 6:
					buf[0] = '\0';
					//TODO:Add mutex
					for(count=0;count<userData[userID].messageCount;count++)
					{
						strcpy(timeStr , ctime(&userData[userID].messsageReceiveTime[count]));
						timeStr[strlen(timeStr)-1] = '\0';
						sprintf(buf,"From %s, %s, %s|",userData[userID].senderUsername, timeStr , userData[userID].messages[count]);
						strcpy(userData[userID].messages[count],"");
					}
					userData[userID].messageCount = 0;

					
					time(&nowTime);
					strcpy( timeStr ,ctime(&nowTime));
					timeStr[strlen(timeStr)-1] = '\0';
					printf("\n%s, %s gets messages.",timeStr, userData[userID].name);

					//TODO: finish mutex
					
					buf[strlen(buf)-1] = '\0';					
					if((count = sendAllData(sd, buf, strlen(buf))) == -1)
					{
						perror("Error on write call");
						exit(1);
					}
					break;
				case 7:
				default:
					//TODO: mutex
					logout(userID);
					//TODO: mutex
					strcpy(buf,"LOGGED_OUT");
					if((count = sendAllData(sd, buf, strlen(buf))) == -1)
						{
							perror("Error on write call");
							exit(1);
						}
					flag=0;					
			}
		}
		
	}
	
	/* close socket */
	close(sd); 
}























/***********************************LOGIN AND LOGOUT***************************************/
int login(char* userName)
{
	int count;
	time_t nowTime;
	char timeStr[20];
	for(count=0;count<totalRecordCount;count++)
	{
		if (strcmp(userData[count].name, userName) == 0)
		{
			if (userData[count].status == 1)
				return -1;
			else
			{
				userData[count].status = 1;
				time(&nowTime);
				strcpy( timeStr ,ctime(&nowTime));
				timeStr[strlen(timeStr)-1] = '\0';
				printf("\n%s, Connection by known user %s.",timeStr, userData[count].name);
				return count;
			}
		}
	}
	//Add the user to the list if it doesnt exist already
	if(count == totalRecordCount)
	{
		return addUser(userName,1);
		time(&nowTime);
		strcpy( timeStr ,ctime(&nowTime));
		timeStr[strlen(timeStr)-1] = '\0';
		printf("\n%s, Connection by unknown user %s.",timeStr, userData[totalRecordCount-1].name);
	}
	return -1;
}

//Sets the status of the user to "0".
void logout(int userID)
{
	time_t nowTime;
	char timeStr[20];
	userData[userID].status = 0;
	time(&nowTime);
	strcpy( timeStr ,ctime(&nowTime));
	timeStr[strlen(timeStr)-1] = '\0';
	printf("\n%s, %s exits.",timeStr, userData[userID].name);

}












/*******************************DATABASE RELATED FUNCTIONS**************************/

//Initialises the blank records of all users.
void initialiseDatabase()
{
	int count1,count2;
	for(count1=0;count1<MAX_KNOWN_USERS;count1++)
	{
		strcpy(userData[count1].name,"");
		userData[count1].status = 0;
		for(count2=0; count2<MAX_MESSAGES; count2++)
		{
			strcpy(userData[count1].messages[count2],"");
			strcpy(userData[count1].senderUsername[count2],"");
		}
		userData[count1].messageCount = 0;
	}

}

//This function adds the user with the given name and status
int addUser(char* name, int status)
{
	strcpy(userData[totalRecordCount].name , name);
	userData[totalRecordCount].status = status;
	totalRecordCount++;
	return totalRecordCount-1;
}


//This function adds the message to user on the basis of its ID
int addMessageToUserByID(char* message, char* senderUserName, int userID)
{
	int messageID = userData[userID].messageCount;
	if(messageID == MAX_MESSAGES)
		return -1;
	strcpy(userData[userID].messages[messageID] , message);
	strcpy(userData[userID].senderUsername[messageID] , senderUserName);
	time(&userData[userID].messsageReceiveTime[messageID]);
	userData[userID].messageCount = messageID +1;
}

//This function adds the message on the basis of its NAME.
void addMessageToUserByName(char* message, char* senderUserName, char* userName)
{
	int count;
	for(count=0;count<totalRecordCount;count++)
	{
		if(strcmp(userData[count].name , userName) == 0)
		{
			addMessageToUserByID(message , senderUserName, count);
			break;
		}
	}
	//If the userName does not exist, create the record and save the message.
	if(count == totalRecordCount){
		count = addUser(userName,0);
		addMessageToUserByID(message, senderUserName, count);
	}

}
void addMessageToConnectedUsers(char* message, char* senderUserName)
{
	int count;
	for(count=0;count<totalRecordCount;count++)
	{
		if( userData[count].status == 1 )
		{
			addMessageToUserByID(message , senderUserName, count);
		}
	}
}
void addMessageToKnownUsers(char* message, char* senderUserName)
{
	int count;
	for(count=0;count<totalRecordCount;count++)
		addMessageToUserByID(message, senderUserName, count);
}



/*************************************SOCKET RELATED FUNCTIONS************************************/

//This function sends all the data repeatedly untill the entire data is sent
int sendAllData(int s, char *buf, int len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = len; // how many we have left to send
	int n;
	char packetLength[10];
	char temp[BUFFERSIZE]={};
	packetLength[0] = '\0';
	sprintf( packetLength, "%d|", strlen(buf));
	memset(temp,'0',5-strlen(packetLength));
	strcat(temp, packetLength);
	strcpy(packetLength, temp);
	write(s, packetLength, strlen(packetLength));

	while(total < len)
	{
		n = write(s, buf+total, bytesleft);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	len = total; // return number actually sent here

	return n==-1?-1:len; // return -1 on failure, 0 on success
}

//This function receives all the data repeatedly untill the entire data is received. 
//Works for partial sents as well.
int receiveAllData(int s, char *buf, int len)
{
	char packet[BUFFERSIZE]={}; 
	char packetLengthStr[5] = {};
	int packetLength, total=0, n, bytesleft;
	

	for(n=0;n<len;n++)
	{
		*(buf+n) = '\0';
	}

	//Read the size part of the packet which might have the data part as well.
	n = read(s, packet, BUFFERSIZE);
	if(n == -1)	{	return -1;   }

	//Copy the data part of the packet to the buffer.
	char *pos;
	pos = strchr(packet, '|');
	strcat(buf, pos+1);
	total = strlen(buf);

	//Copy the packetLength and convert it into an int
	strncpy(packetLengthStr, packet, pos-packet);
	packetLength = atoi(packetLengthStr);
	bytesleft = packetLength-strlen(buf);

	//Repeatedly read from the socket until the entire data is read
	while(total < packetLength) 
	{
		n = read(s, buf+total, bytesleft);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	len = total;
	
	return n==-1?-1:len; // return -1 on failure, length of packet on success
}
