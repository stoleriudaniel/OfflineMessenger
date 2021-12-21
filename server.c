 /* servTcpConc.c - Exemplu de server TCP concurent
    Asteapta un nume de la clienti; intoarce clientului sirul
    "Hello nume".
    */

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
	int rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS Users(name varchar(100), password varchar(100))", NULL, NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS NewMessages(name varchar(100), nameFrom varchar(100), idMessage INT, message varchar(100))", NULL, NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS RepliedMessages(name varchar(100), nameToReply varchar(100), idMessage INT, message varchar(100))", NULL ,NULL, &err);
	if(rc!=SQLITE_OK)
	{
		printf("eroare: %s", err);
	}
	rc=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS Messages(name varchar(100), idMessage INT, message varchar(100))", NULL ,NULL, &err);
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

			char mesaj[100];
			char comanda[100];		//mesajul primit de la client
			strcpy(mesaj,"Introduceti comanda:");
			if (write (client, mesaj, 100) <= 0)
			{
				perror ("[server]Eroare la write() catre client.\n");
				continue;		/* continuam sa ascultam */
			}
			else
				printf ("[server]Mesajul a fost trasmis cu succes.\n");
			int loggedIn=0;
			while(1){

				/* s-a realizat conexiunea, se astepta mesajul */
				bzero (comanda, 100);
				printf ("[server]Asteptam comanda...\n");
				fflush (stdout);
				/* citirea mesajului */
				if (read (client, comanda, 100) <= 0)
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
					char msgrasp[100]=" ";        //mesaj de raspuns pentru client
					char nume[100];
					char parola[100];
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,100);
					bzero(nume,100);
					bzero(parola,100);
					strcpy(msgrasp,"Introduceti numele de utilizator:");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

					if (read (client, nume, 100) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					strcpy(msgrasp,"Introduceti parola:");
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

					if (read (client, parola, 100) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					if(existsUserInUsersTable(nume)){
						strcpy(msgrasp,"Inregistrare esuata! Utilizatorul exista deja. Introduceti comanda:");	
					}
					else{
						addNewUser(nume,parola);
						strcpy(msgrasp,"Inregistrat cu succes! Introduceti comanda:");
					}
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

				}//--------------------------------------------------------------LOGIN--------------------------------------------------------------
				else if(strcmp(comanda,"LOGIN")==0){
					printf("da, login");
					char msgrasp[100]=" ";        //mesaj de raspuns pentru client
					char nume[100];
					char parola[100];
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,100);
					bzero(nume,100);
					bzero(parola,100);
					strcpy(msgrasp,"Introduceti numele de utilizator:");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

					if (read (client, nume, 100) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					strcpy(msgrasp,"Introduceti parola:");
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

					if (read (client, parola, 100) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					if(validAuthentication(nume, parola)){
						strcpy(msgrasp,"Autentificat cu succes! Introduceti comanda:");
						loggedIn=1;
					}
					else {
						strcpy(msgrasp,"Autentificare esuata! Introduceti comanda:");
					}
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");

				}////--------------------------------------------------------------EXIT--------------------------------------------------------------
				else if(strcmp(comanda,"EXIT")==0){
					printf("da, login");
					char msgrasp[100]=" ";        //mesaj de raspuns pentru client
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,100);
					strcpy(msgrasp,"exit");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");
				}
				////--------------------------------------------------------------INVALID--------------------------------------------------------------
				else {
					char msgrasp[100]=" ";        //mesaj de raspuns pentru client
					printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

					bzero(msgrasp,100);
					strcat(msgrasp,"Comanda invalida. Incercati din nou");
					/* returnam mesajul clientului */
					if (write (client, msgrasp, 100) <= 0)
					{
						perror ("[server]Eroare la write() catre client.\n");
						continue;		/* continuam sa ascultam */
					}
					else
						printf ("[server]Mesajul a fost trasmis cu succes.\n");
				}
				while(loggedIn){
					bzero (comanda, 100);
					printf ("[server]Asteptam comanda...\n");
					fflush (stdout);
					/* citirea mesajului */
					if (read (client, comanda, 100) <= 0)
					{
						perror ("[server]Eroare la read() de la client.\n");
						close (client);	/* inchidem conexiunea cu clientul */
						continue;		/* continuam sa ascultam */
					}
					////--------------------------------------------------------------SEND_MESSAGE--------------------------------------------------------------
					if(strcmp(comanda,"SEND_MESSAGE")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"send_message");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------NEW_MESSAGES--------------------------------------------------------------
					else if(strcmp(comanda,"NEW_MESSAGES")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"new_messages");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------SHOW_NEW_MESSAGES--------------------------------------------------------------
					else if(strcmp(comanda,"SHOW_NEW_MESSAGES")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"show_new_messages");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------REPLY_MESSAGE--------------------------------------------------------------
					else if(strcmp(comanda,"REPLY_MESSAGE")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"reply_message");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------SHOW_CONVERSATION_HISTROY--------------------------------------------------------------
					else if(strcmp(comanda,"SHOW_CONVERSATION_HISTROY")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"show_conversation_history");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------LOGOUT--------------------------------------------------------------
					else if(strcmp(comanda,"LOGOUT")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"Deconectarea a avut succes! Introduceti comanda:");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
						loggedIn=0;
					}
					////--------------------------------------------------------------EXIT--------------------------------------------------------------
					else if(strcmp(comanda,"EXIT")==0){
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcpy(msgrasp,"exit");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
					////--------------------------------------------------------------INVALID--------------------------------------------------------------
					else {
						char msgrasp[100]=" ";        //mesaj de raspuns pentru client
						printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);

						bzero(msgrasp,100);
						strcat(msgrasp,"Comanda invalida. Incercati din nou");
						/* returnam mesajul clientului */
						if (write (client, msgrasp, 100) <= 0)
						{
							perror ("[server]Eroare la write() catre client.\n");
							continue;		/* continuam sa ascultam */
						}
						else
							printf ("[server]Mesajul a fost trasmis cu succes.\n");
					}
				}
			}
    		/* am terminat cu acest client, inchidem conexiunea */
    		close (client);
    		exit(0);
    	}

    }				/* while */
}				/* main */