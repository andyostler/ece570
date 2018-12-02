/* to compile me in Linux, type:   gcc -o concurrentserver concurrentserver.c -lpthread */

/* server.c - code for example server program that uses TCP */
/* From Computer Networks and Internets by Douglas F. Comer */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void * serverthread(void * parm);       /* thread function prototype    */

pthread_mutex_t  mut;

#define PROTOPORT         8222          /* default protocol port number */
#define QLEN              20            /* size of request queue        */

int visits =  0;                        /* counts client connections     */

/*************************************************************************
 Program:        concurrent server

 Purpose:        allocate a socket and then repeatedly execute the folllowing:
                          (1) wait for the next connection from a client
                          (2) create a thread to handle the connection
                          (3) go back to step (1)

                 The server thread will
                          (1) update a global variable in a mutex
                          (2) read the message from the client
                          (3) send a short message to the client
                          (4) close the connection

 Syntax:         server [ port ]

                            port  - protocol port number to use

 Note:           The port argument is optional. If no port is specified,
                        the server uses the default given by PROTOPORT.

**************************************************************************
*/

main (int argc, char *argv[])
{
     struct   hostent   *ptrh;     /* pointer to a host table entry */
     struct   protoent  *ptrp;     /* pointer to a protocol table entry */
     struct   sockaddr_in sad;     /* structure to hold server's address */
     struct   sockaddr_in cad;     /* structure to hold client's address */
     int      sd, sd2;             /* socket descriptors */
     int      port;                /* protocol port number */
     int      alen;                /* length of address */
     pthread_t  tid;             /* variable to hold thread ID */

     pthread_mutex_init(&mut, NULL);
     memset((char  *)&sad,0,sizeof(sad)); /* clear sockaddr structure   */
     sad.sin_family = AF_INET;            /* set family to Internet     */
     sad.sin_addr.s_addr = INADDR_ANY;    /* set the local IP address */

     /* Check  command-line argument for protocol port and extract      */
     /* port number if one is specfied.  Otherwise, use the default     */
     /* port value given by constant PROTOPORT                          */
    
     if (argc > 1) {                        /* if argument specified     */
                     port = atoi (argv[1]); /* convert argument to binary*/
     } else {
                      port = PROTOPORT;     /* use default port number   */
     }
     if (port > 0)                          /* test for illegal value    */
                      sad.sin_port = htons((u_short)port);
     else {                                /* print error message and exit */
                      fprintf (stderr, "bad port number %s/n",argv[1]);
                      exit (1);
     }

     /* Map TCP transport protocol name to protocol number */
     
     if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)  {
                     fprintf(stderr, "cannot map \"tcp\" to protocol number");
                     exit (1);
     }

     /* Create a socket */
     sd = socket (PF_INET, SOCK_STREAM, ptrp->p_proto);
     if (sd < 0) {
                       fprintf(stderr, "socket creation failed\n");
                       exit(1);
     }

     /* Bind a local address to the socket */
     if (bind(sd, (struct sockaddr *)&sad, sizeof (sad)) < 0) {
                        fprintf(stderr,"bind failed\n");
                        exit(1);
     }

     /* Specify a size of request queue */
     if (listen(sd, QLEN) < 0) {
                        fprintf(stderr,"listen failed\n");
                         exit(1);
     }

     alen = sizeof(cad);

     /* Main server loop - accept and handle requests */
     printf("SERVER:  Up and running; waiting for contact...\n");
     while (1) {

         if (  (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
	                      fprintf(stderr, "accept failed\n");
                              perror("Error in accept system call: System message");
                              exit (1);
	 }
	 pthread_create(&tid, NULL, serverthread, (void *) &sd2 );
         
         printf("SERVER: Spawned thread; waiting for contact...\n");
     }
     close(sd);
}


void * serverthread(void * parm)
{
   int tsd, tvisits;
   int tot, n;
   int i;
   char rcv_buffer[1001];         /* buffer for data to receive */
   char *message_to_archive;      /* pointer to each PDU segment */
   char ncsu[5];                  /* to store NCSU string */
   char string_sid[10];           /* to store string version of student ID */
   int sid;                       /* to store student ID */
   char *string_info;             /* to point to beginning of string information */
   char     buf[100];             /* buffer for string the server sends */
   FILE *fp, *fp_log;                  /* to log the exact received buffer per thread */
   char logfile_name[20];
   char arch_filename[50];
   char header[5000];
   struct   sockaddr_in cad;      /* structure to hold client's address */
   int ad_len;
   struct timeval tv;
   struct tm nowtm;
   char client_ad[20];
   int client_port;

   tsd = *(int *)parm;

   pthread_mutex_lock(&mut);
        tvisits = ++visits;
   pthread_mutex_unlock(&mut);

   message_to_archive = rcv_buffer;

   tot = 0;
   do {
        tot += (n = recv (tsd, message_to_archive+tot, 4-tot, 0));
        if (n < 0) {
           fprintf (stderr, "Thread %d: ERROR reading from socket: %d", tvisits, n);
           printf ("Thread %d: ERROR reading from socket: %d", tvisits, n);
           perror("Error reading first field of 4 bytes; System message");
           close (tsd);
           pthread_exit(0);
        }
   } while (tot < 4);
   memcpy ((void *)&ncsu, (void *)message_to_archive, 4);
   message_to_archive += 4;
   ncsu[4] = 0;
   printf ("Thread %d: preamble: %s\n", tvisits, ncsu);

   tot = 0;
   do {
        tot += (n = recv (tsd, message_to_archive+tot, 9-tot, 0));
        if (n < 0) {
           fprintf (stderr, "Thread %d: ERROR reading from socket: %d", tvisits, n);
           printf ("Thread %d: ERROR reading from socket: %d", tvisits, n);
           perror("Error reading second field of 9 bytes; System message");
           close (tsd);
           pthread_exit(0);
        }
   } while (tot < 9);
   memcpy ((void *)&string_sid, (void *)message_to_archive, 9);
   message_to_archive += 9;
   printf ("Thread %d: SID as string: %s\n", tvisits, string_sid);

   tot = 0;
   do {
        tot += (n = recv (tsd, message_to_archive+tot, 4-tot, 0));
        if (n < 0) {
           fprintf (stderr, "Thread %d: ERROR reading from socket: %d", tvisits, n);
           printf ("Thread %d: ERROR reading from socket: %d", tvisits, n);
           perror("Error reading third field of 4 bytes; System message");
           close (tsd);
           pthread_exit(0);
        }
   } while (tot < 4);
   sid = ntohl (*(int *)message_to_archive);
   message_to_archive += 4;
   printf ("Thread %d: SID as number: %d\n", tvisits, sid);

   tot = 0;
   do {
        tot += (n = recv (tsd, message_to_archive+tot, 1, 0));
        if (n < 0) {
           fprintf (stderr, "Thread %d: ERROR reading from socket: %d", tvisits, n);
           printf ("Thread %d: ERROR reading from socket: %d", tvisits, n);
           perror("Error reading byte of fourth field of variable size; System message");
           close (tsd);
           pthread_exit(0);
        }
        if (tot == 1000-4-9-4)
           break;
   } while (message_to_archive[tot-1] > 0);
   message_to_archive[tot] = 0;
   string_info = message_to_archive;
   printf ("Thread %d: Rest of message: %s\n", tvisits, string_info);

   sprintf(buf,"This server has now been contacted %d time%s\n",
	   tvisits, tvisits==1?".":"s.");

   printf("SERVER thread: %s", buf);

   sprintf (arch_filename, "csc573_f16_hw3_tcp.txt");
   ad_len = sizeof (cad);
   getpeername (tsd, (struct sockaddr *) &cad, &ad_len);
   inet_ntop (AF_INET, &(cad.sin_addr), client_ad, 20);
   client_port = ntohs (cad.sin_port);

   gettimeofday(&tv, NULL);
   nowtm = *(localtime (& (tv.tv_sec)));

   pthread_mutex_lock (&mut);
    sprintf (header, "Thread %d: Envelope Information:\nFrom host IP %s port %d at %s :\n", tvisits, client_ad, client_port, asctime (&nowtm));
    printf ("%s %s %s %d %s\nEnd of Envelope Information\n", header, ncsu, string_sid, sid, string_info);
    fp = fopen (arch_filename, "a");
    fprintf (fp, "%s %s %s %d %s\nEnd of Envelope Information\n", header, ncsu, string_sid, sid, string_info);
    fclose (fp);
   pthread_mutex_unlock (&mut);

   sprintf (buf, "Server received and archived %d bytes of transmitted data\n", 4+9+4+tot);
   send (tsd,buf,strlen(buf),0);

   sprintf (logfile_name, "TCP_thread_%d.buflog", tvisits);
   fp_log = fopen (logfile_name, "w");
   n = write (fileno(fp_log), (void *)rcv_buffer, 4+9+4+tot);
   fclose (fp_log);
   printf ("Thread %d: written to log, status: %d, error: %d\n", tvisits, n, errno);

   printf ("Done with socket %d; exiting thread\n", tsd);

   close (tsd);
   pthread_exit(0);
}
