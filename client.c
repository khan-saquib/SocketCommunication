#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>



#define NAMELENGTH 20
#define MESSGSIZE 80 
#define BUFFERSIZE 2500
#define MAX_KNOWN_USERS 100
#define MAX_MESSAGES 10






void printData(char* data);





//This function sends all the data repeatedly untill the entire data is sent
int sendAllData(int s, char *buf, int len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = len; // how many we have left to send
	int n;
	char packetLength[10];
	char temp[BUFFERSIZE]={};
	packetLength[0]='\0';
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








int main(int argc, char *argv[])
{
	char hostname[100];
	char buf[BUFFERSIZE] = {};
	char name[20]={}, message[MESSGSIZE]={};
	int option=1;
	char optionStr[5];
	int sd;
	int port;
	int count;
	struct sockaddr_in pin;
	struct hostent *hp;

	/* check for command line arguments */
	if (argc != 3)
	{
		printf("Usage: client host port\n");
		exit(1);
	}

	/* get hostname and port from argv */
	strcpy(hostname, argv[1]);
	port = atoi(argv[2]);

	/* create an Internet domain stream socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating socket");
		exit(1);
	}

	/* lookup host machine information */
	if ((hp = gethostbyname(hostname)) == 0) {
		perror("Error on gethostbyname call");
		exit(1);
	}

	/* fill in the socket address structure with host information */
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(port); /* convert to network byte order */


	printf("Connecting to %s:%d\n\n", hostname, port); 

   /* connect to port on host */
   if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("Error on connect call");
		exit(1);
	}

	printf("\nEnter your name: "); 
	fgets(name, NAMELENGTH, stdin);
	name[strlen(name)-1] = '\0';

	/* send the user's name to the server */ 
	if ( (count = sendAllData(sd, name, strlen(name))) == -1) 
	{
		perror("Error on write call");
		exit(1);
	}

	/* read login status from the server */
	if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
	{
		perror("Error on read call");
		exit(1);
	}
	/*Check the login status and exit if login failed.*/
	if (strcmp(buf,"LOGIN_FAIL") == 0)
	{
		printf("\nYou are already logged from another terminal. Please logout from that terminal and try again.\n");
		close(sd);
		exit(1);
	}
	
	while(option !=7)
	{
		printf("\n\n1. Display the names of all known users.");
		printf("\n2. Display the names of all currently connected users.");
		printf("\n3. Send a text message to a particular user.");
		printf("\n4. Send a text message to all currently connected users.");
		printf("\n5. Send a text message to all known users.");
		printf("\n6. Get my messages.");
		printf("\n7. Exit.\n");
		printf("Enter your choice: ");
		fgets(optionStr, 5, stdin);
		optionStr[strlen(optionStr)-1] = '\0';
		option = atoi(optionStr);
		
		
		if(option>7 || option <1)
		{
			printf("Invalid option selected.\n");
			continue;
		}
		
		/* Send the option selected to the server */
		sprintf(buf,"%d",option);
		if((count = sendAllData(sd, buf, strlen(buf))) == -1)
		{
			perror("Error on write call");
			exit(1);
		}
		
		/* Perform the operation requested*/
		switch(option)
		{
			case 1:
				/* Receive the response by server for that option */		
				if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
				{
					perror("Error on read call");
					exit(1);
				}
				printf("\nKnown users:\n");
				printData(buf);
				break;
			case 2:
				/* Receive the response by server for that option */		
				if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
				{
					perror("Error on read call");
					exit(1);
				}
				printf("\nCurrently connected users:\n");
				printData(buf);
				break;
			case 3:
				printf("\nEnter the user name: ");
				fgets(name, NAMELENGTH, stdin);
				name[strlen(name)-1] = '\0';
				printf("\nEnter your message: ");
				fgets(message, MESSGSIZE, stdin);
				message[strlen(message)-1] = '\0';
				sprintf(buf,"%s|%s", name, message);
				if((count = sendAllData(sd, buf, strlen(buf))) == -1)
				{
					perror("Error on write call");
					exit(1);
				}
				break;
			case 4:
				printf("\nEnter your message: ");
				fgets(message, MESSGSIZE, stdin);
				message[strlen(message)-1] = '\0';
				if((count = sendAllData(sd, message, strlen(message))) == -1)
				{
					perror("Error on write call");
					exit(1);
				}
				break;
			case 5:
				printf("\nEnter your message: ");
				fgets(message, MESSGSIZE, stdin);
				message[strlen(message)-1] = '\0';		
				if((count = sendAllData(sd, message, strlen(message))) == -1)
				{
					perror("Error on write call");
					exit(1);
				}
				break;
			case 6:
				/* Receive the response by server for that option */		
				if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
				{
					perror("Error on read call");
					exit(1);
				}
				if(strcmp(buf,"")==0)
				{
					printf("\nYou do not have any unread messages.\n");
				}
				else
				{
					printf("\nYour messages:\n");
					printData(buf);
				}
				break;
			case 7:
			default:
				if((count = receiveAllData(sd, buf, BUFFERSIZE)) == -1)
				{
					perror("Error on read call");
					exit(1);
				}
				if(strcmp(buf, "LOGGED_OUT")==0)
					printf("\nYou have successfully logged out\n");
				option = 7;
				break;
		}
	}


	/* close the socket */
	close(sd);
}




void printData(char* data)
{
	char* temp;
	int count = 1;
	temp = strtok (data,"|");
	while (temp != NULL)
	{
		printf("%d. %s\n",count,temp);
		count++;
		temp = strtok (NULL, "|");
		//printf("%d. %s\n",count,temp);
	}	
	printf("\n\n");
}
