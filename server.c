#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "sqlite/sqlite3.h"
/* portul folosit */
#define PORT 2024

/* codul de eroare returnat de anumite apeluri */

void createTables(){
	char* err;
	sqlite3* db;
	//sqlite3_stmt* stmt;
	sqlite3_open("DataBase.db", &db);
	int rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS Users(name varchar(100), password varchar(100));", NULL, NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS NewMessages(name varchar(100), nameFrom varchar(100), message varchar(100), messageNo int);", NULL, NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS RepliedMessages(name varchar(100), nameToReply varchar(100), message varchar(100));", NULL ,NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS Messages(name1 varchar(100), name2 varchar(100), nameFrom varchar(100), nameTo varchar(100), message varchar(100), messageNo int);", NULL ,NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
}

void addNewUser(char* name, char* password){
	sqlite3 *db;
	sqlite3_stmt * st;
	printf("--name:%s strlen(name):%ld\n",name,strlen(name));
	printf("--password:%s strlen(password):%ld\n",password,strlen(password));
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK)
	{
		char* sql = "INSERT INTO Users (name, password) VALUES (?, ?);";
		int rc = sqlite3_prepare(db, sql, -1, &st, NULL);
		if (rc == SQLITE_OK)
		{
			sqlite3_bind_text(st, 1, name, strlen(name), SQLITE_TRANSIENT);
			sqlite3_bind_text(st, 2, password, strlen(password),  SQLITE_TRANSIENT);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	}
}

int existsUserInUsersTable(char* name){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int foundUser = 0;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT name, password FROM Users WHERE name=?;",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name,-1,NULL);
		while(sqlite3_step(stmt)!=SQLITE_DONE)
		{
			char* nameFound=sqlite3_column_text(stmt,0);
			printf("--nameFound:%s strlen(nameFound):%ld\n",nameFound,strlen(nameFound));
			if(strcmp(name,nameFound)==0){
				printf("\nDa, gasit nume:%s\n",nameFound);
				foundUser=1;
			}
		}
		sqlite3_finalize(stmt);
	}
	return foundUser;
}

int validAuthentication(char* name, char* password){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int valid = 0;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT name, password FROM Users WHERE name=?;",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name,-1,NULL);
		while(sqlite3_step(stmt)!=SQLITE_DONE)
		{
			char* nameFound=sqlite3_column_text(stmt,0);
			char* passwordFound=sqlite3_column_text(stmt,1);
			if(strcmp(name,nameFound)==0 && strcmp(password,passwordFound)==0){
				printf("\nDa, nume gasit:%s\n",nameFound);
				printf("\nDa, parola gasita:%s\n",passwordFound);
				valid=1;
			}
		}
		sqlite3_finalize(stmt);
	}
	return valid;
}

void sendMessageFromPerson1ToPerson2(char* name1, char* name2, char* message){
	sqlite3 *db;
	sqlite3_stmt * st;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK)
	{
		char* sql = "INSERT INTO NewMessages(name, nameFrom, message, messageNo) VALUES (?, ?, ?, ?);";
		int rc = sqlite3_prepare(db, sql, -1, &st, NULL);
		if (rc == SQLITE_OK)
		{
			sqlite3_bind_text(st, 1, name2, strlen(name2), SQLITE_TRANSIENT);
			sqlite3_bind_text(st, 2, name1, strlen(name1),  SQLITE_TRANSIENT);
			sqlite3_bind_text(st, 3, message, strlen(message),  SQLITE_TRANSIENT);
			int messageNo = getMaximumIDValueMsgBetweenPerson1AndPerson2(name1, name2) + 1;
			printf("\n\n###IN SEND MESSAGE: insert new id(+1):%d\n",messageNo);
			printf("MAX_ID in send message=%d\n",messageNo);
			sqlite3_bind_int(st,4,messageNo);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	}
}

int existsNewMessages(char *name, char fromUsers[100][100]){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int usersNo=0;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT DISTINCT name, nameFrom FROM NewMessages WHERE name=?;",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name,-1,NULL);
		while(sqlite3_step(stmt)!=SQLITE_DONE)
		{
			char* nameFound=sqlite3_column_text(stmt,1);
			printf("--nameNewMessageFound:%s strlen(nameFound):%ld\n",nameFound,strlen(nameFound));
			strcpy(fromUsers[usersNo],nameFound);
			usersNo++;
		}
		sqlite3_finalize(stmt);
	}
	return usersNo;
}

int getNewMessages(char* name, char *nameFrom, char messages[100][100], int idRows[100]){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int messagesNo=0;
	printf("function_name=%s_nameFrom=%s\n",name,nameFrom);
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT message, messageNo FROM NewMessages WHERE name=? AND nameFrom=?;",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name,-1,NULL);
		sqlite3_bind_text(stmt,2,nameFrom,-1,NULL);
		while(sqlite3_step(stmt)!=SQLITE_DONE)
		{
			char* message=sqlite3_column_text(stmt,0);
			idRows[messagesNo]=sqlite3_column_int(stmt,1);
			printf("function_message:%s\n",message);
			printf("--NewMessageFound:%s strlen(message):%ld\n",message,strlen(message));
			strcpy(messages[messagesNo],message);
			messagesNo++;
		}
		sqlite3_finalize(stmt);
	}
	printf("_function_messagesNo=%d\n",messagesNo);
	return messagesNo;
}

void deleteNewMessages(char* name, char* nameFrom){
	sqlite3 *db;
	sqlite3_stmt * st;
	printf("--name:%s strlen(name):%ld\n",name,strlen(name));
	printf("--nameFrom:%s strlen(nameFrom):%ld\n",nameFrom,strlen(nameFrom));
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK)
	{
		char* sql = "DELETE FROM NewMessages WHERE name=? AND nameFrom=?;";
		int rc = sqlite3_prepare(db, sql, -1, &st, NULL);
		if (rc == SQLITE_OK)
		{
			sqlite3_bind_text(st,1,name,-1,NULL);
			sqlite3_bind_text(st,2,nameFrom,-1,NULL);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	}
}

int getMaximumIDValueMsgBetweenPerson1AndPerson2(char* name1, char* name2){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int rowsNo;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM Messages WHERE ((name1=? AND name2=?) OR (name1=? AND name2=?));",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name1,-1,NULL);
		sqlite3_bind_text(stmt,2,name2,-1,NULL);
		sqlite3_bind_text(stmt,3,name2,-1,NULL);
		sqlite3_bind_text(stmt,4,name1,-1,NULL);
		sqlite3_step(stmt);
		rowsNo=sqlite3_column_int(stmt,0);
		//printf("--nameNewMessageFound:%s strlen(nameFound):%ld\n",nameFound,strlen(nameFound));
		//strcpy(fromUsers[usersNo],nameFound);
		//usersNo++;
		sqlite3_finalize(stmt);
	}
	printf("###IN GET MAXIMUM VALUE:return %d\n\n\n",(rowsNo/2));
	return rowsNo/2;
}

void addTheNewMessagesToHistory(char* name, char *nameFrom){
	//"CREATE TABLE IF NOT EXISTS NewMessages(name varchar(100), nameFrom varchar(100), message varchar(100))"
	//"CREATE TABLE IF NOT EXISTS Messages(name1 varchar(100), name2 varchar(100), nameFrom varchar(100), nameTo varchar(100), message varchar(100), messageNo int)"
	char messages[100][100]; 
	int idRows[100];
	int messagesNo;
	messagesNo = getNewMessages(name, nameFrom, messages, idRows);
	printf("in add to history function: name:%s nameFrom:%s\n", name, nameFrom);
	printf("messagesNo=%d, MESSAGES::::::\n", messagesNo);
	for(int indexMessage=0; indexMessage<messagesNo; indexMessage++){
		printf("msg: %s\n",messages[indexMessage]);
	}
	sqlite3 *db;
	sqlite3_stmt * st1;
	sqlite3_stmt * st2;
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK)
	{
		printf("in add to history function: name:%s nameFrom:%s\n", name, nameFrom);
		for(int indexMessage=0; indexMessage<messagesNo; indexMessage++){
			int messageIDNo=getMaximumIDValueMsgBetweenPerson1AndPerson2(name,nameFrom);
			int newMessageIDNoINT = messageIDNo + 1;
			printf("###IN ADD THE NEW MESSAGES:\n");
			printf("idrows[indexMessage]=%d > messageIDNo=%d\n\n\n",idRows[indexMessage],messageIDNo);
			if(idRows[indexMessage]>messageIDNo){
				char* sql1 = "INSERT INTO Messages(name1, name2, nameFrom, nameTo, message, messageNo) VALUES (?, ?, ?, ?, ?, ?);";
				int rc = sqlite3_prepare(db, sql1, -1, &st1, NULL);
				if (rc == SQLITE_OK)
				{
					sqlite3_bind_text(st1, 1, name, strlen(name), SQLITE_TRANSIENT);
					sqlite3_bind_text(st1, 2, nameFrom, strlen(nameFrom), SQLITE_TRANSIENT);
					sqlite3_bind_text(st1, 3, nameFrom, strlen(nameFrom),  SQLITE_TRANSIENT);
					sqlite3_bind_text(st1, 4, "-", strlen("-"),  SQLITE_TRANSIENT);
					sqlite3_bind_text(st1, 5, messages[indexMessage], strlen(messages[indexMessage]),  SQLITE_TRANSIENT);
					sqlite3_bind_int(st1, 6, idRows[indexMessage]);
					sqlite3_step(st1);
					sqlite3_finalize(st1);
				}
				char* sql2 = "INSERT INTO Messages(name1, name2, nameFrom, nameTo, message, messageNo) VALUES (?, ?, ?, ?, ?, ?);";
				int rc1 = sqlite3_prepare(db, sql2, -1, &st2, NULL);
				if (rc1 == SQLITE_OK)
				{
					sqlite3_bind_text(st2, 1, nameFrom, strlen(nameFrom), SQLITE_TRANSIENT);
					sqlite3_bind_text(st2, 2, name, strlen(name), SQLITE_TRANSIENT);
					sqlite3_bind_text(st2, 3, "-", strlen("-"), SQLITE_TRANSIENT);
					sqlite3_bind_text(st2, 4, name , strlen(name),  SQLITE_TRANSIENT);
					sqlite3_bind_text(st2, 5, messages[indexMessage], strlen(messages[indexMessage]),  SQLITE_TRANSIENT);
					sqlite3_bind_int(st2, 6, idRows[indexMessage]);
					sqlite3_step(st2);
					sqlite3_finalize(st2);
				}
			}
		}
	}
	printf("END FCT ADDNEWMESSAGES\n");
}

int getMessagesHistoryBetweenName1AndName2(char* name1, char* name2, char messages[50][100]){
	sqlite3 *db;
	sqlite3_stmt * stmt;
	int messagesNo=0;
	printf("Hello, world!!! function_name11=%s_name22=%s\n",name1,name2);
	if (sqlite3_open("DataBase.db", &db) == SQLITE_OK){
		sqlite3_prepare_v2(db,"SELECT message, nameFrom, nameTo, rowid FROM Messages WHERE name1=? AND name2=?;",-1,&stmt,0);
		sqlite3_bind_text(stmt,1,name1,-1,NULL);
		sqlite3_bind_text(stmt,2,name2,-1,NULL);
		while(sqlite3_step(stmt)!=SQLITE_DONE)
		{
			char* message=sqlite3_column_text(stmt,0);
			char* nameFrom=sqlite3_column_text(stmt,1);
			char* nameTo=sqlite3_column_text(stmt,2);
			char* rowId=sqlite3_column_text(stmt,3);
			printf("function_message:%s\n",message);
			printf("--NewMessageFound:%s strlen(message):%ld\n",message,strlen(message));
			if(strcmp(name2,nameFrom)==0){
				strcpy(messages[messagesNo],"[");
				strcat(messages[messagesNo],nameFrom);
				strcat(messages[messagesNo],"; ");
			} else if(strcmp(name2,nameTo)==0){
				strcpy(messages[messagesNo],"[");
				strcat(messages[messagesNo],"Me");
				strcat(messages[messagesNo],"; ");
			}
			strcat(messages[messagesNo],"IDmsg:");
			strcat(messages[messagesNo],rowId);
			strcat(messages[messagesNo],"]");
			strcat(messages[messagesNo], message);
			printf("************Message:%s\n",message);
			messagesNo++;
		}
		sqlite3_finalize(stmt);
	}
	printf("_function_messagesNo=%d\n",messagesNo);
	return messagesNo;
}

int main ()
{
    struct sockaddr_in server;	// structura folosita de server
    struct sockaddr_in from;
    int sd;			//descriptorul de socket

    /* crearea unui socket */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
    	perror ("[server]Eroare la socket().\n");
    	return 1;
    }

    /* pregatirea structurilor de date */
    bzero (&server, sizeof (server));
    bzero (&from, sizeof (from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons (PORT);

    /* atasam socketul */
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
    	perror ("[server]Eroare la bind().\n");
    	return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen (sd, 1) == -1)
    {
    	perror ("[server]Eroare la listen().\n");
    	return errno;
    }

	createTables();

    /* servim in mod concurent clientii... */
    while (1)
    {
    	int client;
    	socklen_t length = sizeof(from);

    	printf ("[server]Asteptam la portul %d...\n",PORT);
    	fflush (stdout);

    	/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    	client = accept (sd, (struct sockaddr *) &from, &length);
        //newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);

    	/* eroare la acceptarea conexiunii de la un client */
    	if (client < 0)
    	{
    		perror ("[server]Eroare la accept().\n");
    		continue;
    	}

    	int pid;
    	if ((pid = fork()) == -1) {
    		close(client);
    		continue;
    	} else if (pid > 0) {
    		// parinte
    		close(client);
    		while(waitpid(-1,NULL,WNOHANG));
    		continue;
    	} else if (pid == 0) {
    		// copil
    		close(sd);

			char mesaj[1000];
			char comanda[1000];		//mesajul primit de la client
			strcpy(mesaj,"Introduceti comanda:");
			if (write (client, mesaj, 1000) <= 0)
			{
				perror ("[server]Eroare la write() catre client.\n");
				continue;		/* continuam sa ascultam */
			}
			else
				printf ("[server]Mesajul a fost transmis cu succes.\n");
			int loggedIn=0; char myUsername[100];
			while(1){

				/* s-a realizat conexiunea, se astepta mesajul */
				bzero (comanda, 1000);
				printf ("[server]Asteptam comanda...\n");
				fflush (stdout);
				/* citirea mesajului */
				if (read (client, comanda, 1000) <= 0)
				{
					perror ("[server]Eroare la read() de la client.\n");
					close (client);	/* inchidem conexiunea cu clientul */
					continue;		/* continuam sa ascultam */
				}

				printf ("[server]Comanda a fost receptionata...%s\n", comanda);

				/*pregatim mesajul de raspuns */
				//--------------------------------------------------------------REGISTER--------------------------------------------------------------
				printf("comanda:%s\n", comanda);
				printf("sizeof(comanda):%ld\n", strlen(comanda));
				if(strcmp(comanda,"REGISTER")==0){
					printf("da, register");
					char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
					char nume[1000];
					char parola[1000];
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,1000);
					bzero(nume,1000);
					bzero(parola,1000);
					strcpy(msgrasp,"Introduceti numele de utilizator:");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

					if (read (client, nume, 1000) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					strcpy(msgrasp,"Introduceti parola:");
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

					if (read (client, parola, 1000) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					if(existsUserInUsersTable(nume) || strlen(nume)>100 || strlen(parola)>100){
						strcpy(msgrasp,"Inregistrare esuata! Utilizatorul exista deja. Introduceti comanda:");	
					}
					else{
						addNewUser(nume,parola);
						strcpy(msgrasp,"Inregistrat cu succes! Introduceti comanda:");
					}
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

				}//--------------------------------------------------------------LOGIN--------------------------------------------------------------
				else if(strcmp(comanda,"LOGIN")==0){
					printf("da, login");
					char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
					char nume[1000];
					char parola[1000];
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,1000);
					bzero(nume,1000);
					bzero(parola,1000);
					strcpy(msgrasp,"Introduceti numele de utilizator:");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

					if (read (client, nume, 1000) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					strcpy(msgrasp,"Introduceti parola:");
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

					if (read (client, parola, 1000) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					if(validAuthentication(nume, parola) && strlen(nume)<=100 && strlen(parola)<=100){
						strcpy(msgrasp,"Autentificat cu succes! Introduceti comanda:");
						loggedIn=1;
						strcpy(myUsername,nume);
					}
					else {
						strcpy(msgrasp,"Autentificare esuata! Introduceti comanda:");
					}
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");

				}////--------------------------------------------------------------EXIT--------------------------------------------------------------
				else if(strcmp(comanda,"EXIT")==0){
					printf("da, login");
					char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,1000);
					strcpy(msgrasp,"exit");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");
				}
				////--------------------------------------------------------------INVALID--------------------------------------------------------------
				else {
					char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,1000);
					strcat(msgrasp,"Comanda invalida. Incercati din nou");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 1000) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost transmis cu succes.\n");
				}
				while(loggedIn){
					bzero (comanda, 1000);
					printf ("[server]Asteptam comanda...\n");
					fflush (stdout);
					/* citirea mesajului */
					if (read (client, comanda, 1000) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					////--------------------------------------------------------------SEND_MESSAGE--------------------------------------------------------------
					if(strcmp(comanda,"SEND_MESSAGE")==0){
						char nameToSend[1000];
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp,"Enter the username to send message:");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
						bzero(nameToSend, 1000);
						if (read (client, nameToSend, 1000) <= 0)
						{
							perror ("[server]Eroare la read() de la client.\n");
							close (client);	/* inchidem conexiunea cu clientul */
							continue;		/* continuam sa ascultam */
						}
						while(!existsUserInUsersTable(nameToSend)){
							bzero(msgrasp,1000);
							strcpy(msgrasp,"Username invalid! Enter the username to send message:");
							/* returnam mesajul clientului */
							if (write (client, msgrasp, 1000) <= 0)
							{
								perror ("[server]Eroare la write() catre client.\n");
								continue;		/* continuam sa ascultam */
							}
							else
								printf ("[server]Mesajul a fost transmis cu succes.\n");
							bzero(nameToSend, 1000);
							if (read (client, nameToSend, 1000) <= 0)
							{
								perror ("[server]Eroare la read() de la client.\n");
								close (client);	/* inchidem conexiunea cu clientul */
								continue;		/* continuam sa ascultam */
							}
						}
						bzero(msgrasp,1000);
						strcpy(msgrasp,"Write the message:");
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
						bzero(msgrasp, 1000);
						if (read (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la read() de la client.\n");
							close (client);	/* inchidem conexiunea cu clientul */
							continue;		/* continuam sa ascultam */
						}
						while(strlen(msgrasp)==0){
							bzero(msgrasp,1000);
							strcpy(msgrasp,"Invalid message! Write the message:");
							if (write (client, msgrasp, 1000) <= 0)
							{
								perror ("[server]Eroare la write() catre client.\n");
								continue;		/* continuam sa ascultam */
							}
							else
								printf ("[server]Mesajul a fost transmis cu succes.\n");
							bzero(msgrasp, 1000);
							if (read (client, msgrasp, 1000) <= 0)
							{
								perror ("[server]Eroare la read() de la client.\n");
								close (client);	/* inchidem conexiunea cu clientul */
								continue;		/* continuam sa ascultam */
							}
						}
						sendMessageFromPerson1ToPerson2(myUsername,nameToSend,msgrasp);
						//int messagesNo=getNewMessages(myUsername,nameFrom,messages);
						//addTheNewMessagesToHistory(nameToSend,myUsername);
						char messages[50][100];
						int idRows[50];
						int messagesNo=getNewMessages(nameToSend,myUsername,messages, idRows);
						addTheNewMessagesToHistory(nameToSend,myUsername);
						printf("getNewMessages()=%d\n",messagesNo);
						/*
						if(messagesNo){
							deleteNewMessages(nameToSend,myUsername);
							bzero(msgrasp,1000);
							for(int indexMessage=0; indexMessage<messagesNo; indexMessage++){
								strcat(msgrasp,"\n");
								strcat(msgrasp,messages[indexMessage]);
							}
							//strcat(msgrasp,"\n");
						}
						*/
						bzero(msgrasp,1000);
						strcpy(msgrasp,"The message has been sent.");
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------NEW_MESSAGES--------------------------------------------------------------
					else if(strcmp(comanda,"NEW_MESSAGES")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						char fromUsers[10][100];
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						int usersNo=existsNewMessages(myUsername,fromUsers);
						if(usersNo==0){
							strcpy(msgrasp,"You don't have new messages.");
						}
						else {
							strcpy(msgrasp,"You have new messages from: ");
							for(int index=0; index<usersNo; index++){
								strcat(msgrasp, fromUsers[index]);
								strcat(msgrasp, " ");
							}
						}
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------SHOW_NEW_MESSAGES--------------------------------------------------------------
					else if(strcmp(comanda,"SHOW_NEW_MESSAGES")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						char messages[50][100];
						char nameFrom[1000];
						int messagesNo=0;
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp, "Enter the username to see new messages:");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
						bzero(nameFrom, 1000);
						if (read (client, nameFrom, 1000) <= 0)
						{
							perror ("[server]Eroare la read() de la client.\n");
							close (client);	/* inchidem conexiunea cu clientul */
							continue;		/* continuam sa ascultam */
						}
						int idRows[50];
						messagesNo=getNewMessages(myUsername,nameFrom,messages, idRows);
						//addTheNewMessagesToHistory(myUsername,nameFrom);
						printf("getNewMessages()=%d\n",messagesNo);
						if(messagesNo){
							deleteNewMessages(myUsername,nameFrom);
							bzero(msgrasp,1000);
							for(int indexMessage=0; indexMessage<messagesNo; indexMessage++){
								strcat(msgrasp,"\n");
								strcat(msgrasp,messages[indexMessage]);
							}
							//strcat(msgrasp,"\n");
						}
						else {
							strcpy(msgrasp,"You don't have new messages from this user.");
						}
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------REPLY_MESSAGE--------------------------------------------------------------
					else if(strcmp(comanda,"REPLY_MESSAGE")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp,"reply_message");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------SHOW_CONVERSATION_HISTORY--------------------------------------------------------------
					else if(strcmp(comanda,"SHOW_CONVERSATION_HISTORY")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						char name2[1000];
						char messages[50][100];
						int messagesNo;
						int rowIds[50];
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp,"Enter the username to see conversation history:");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
						if (read (client, name2, 1000) <= 0)
						{
							perror ("[server]Eroare la read() de la client.\n");
							close (client);	/* inchidem conexiunea cu clientul */
							continue;		/* continuam sa ascultam */
						}
						messagesNo=getMessagesHistoryBetweenName1AndName2(myUsername,name2,messages);
						printf("In ( If Conversation ):\n");
						printf("______messagesNo:%d\n",messagesNo);
						bzero(msgrasp,1000);
						strcpy(msgrasp,"\n");
						for(int indexMessage=0; indexMessage<messagesNo; indexMessage++){
							strcat(msgrasp,"\n");
							strcat(msgrasp,messages[indexMessage]);
						}
						strcat(msgrasp,"\n");
						deleteNewMessages(myUsername,name2);
						printf("&&&&&&&&&&&&mesages[0]=%s\n",messages[0]);
						printf("&&&&&&&&&&&&mesages[1]=%s\n",messages[1]);
						printf("\n\nmsgrasp:\n%s\n\n",msgrasp);
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------LOGOUT--------------------------------------------------------------
					else if(strcmp(comanda,"LOGOUT")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp,"Deconectarea a avut succes! Introduceti comanda:");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
						loggedIn=0;
					}
					////--------------------------------------------------------------EXIT--------------------------------------------------------------
					else if(strcmp(comanda,"EXIT")==0){
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcpy(msgrasp,"exit");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
					////--------------------------------------------------------------INVALID--------------------------------------------------------------
					else {
						char msgrasp[1000]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,1000);
						strcat(msgrasp,"Comanda invalida. Incercati din nou");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 1000) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost transmis cu succes.\n");
					}
				}
			}
    		/* am terminat cu acest client, inchidem conexiunea */
    		close (client);
    		exit(0);
    	}

    }				/* while */
}				/* main */