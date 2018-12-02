
/* Sample UDP server */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char**argv)
{
   int sockfd, i, n, m, client_num;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char rcv_buffer[1001];         /* buffer for data to receive */
   char *message_to_archive;      /* pointer to each PDU segment */
   char ncsu[5];                  /* to store NCSU string */
   char string_sid[10];           /* to store string version of student ID */
   int sid;                       /* to store student ID */
   char *string_info;             /* to point to beginning of string information */
   char     buf[100];             /* buffer for string the server sends */
   FILE *fp_log, *fp;             /* to log the exact received buffer per client */
   char logfile_name[20];
   char arch_filename[50];
   char header[5000];
   int ad_len;
   struct timeval tv;
   struct tm nowtm;
   char client_ad[20];
   int client_port;



   client_num = 0;
   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(8222);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   for (;;)
   {
      len = sizeof(cliaddr);
      client_num++;
      for (i=0;i<1000;i++) {rcv_buffer[i]='%';}
      rcv_buffer[1000] = 0;   /* Protect against clients failing to put NULL at end of PDU */
      n = recvfrom (sockfd, rcv_buffer, 1000, 0, (struct sockaddr *)&cliaddr, &len);
      message_to_archive = rcv_buffer;

      memcpy ((void *)&ncsu, (void *)message_to_archive, 4);
      message_to_archive += 4;
      ncsu[4] = 0;
      printf ("Client %d: preamble: %s\n", client_num, ncsu);

      memcpy ((void *)&string_sid, (void *)message_to_archive, 9);
      message_to_archive += 9;
      string_sid[9] = 0;
      printf ("Client %d: SID as string: %s\n", client_num, string_sid);

      sid = ntohl (*(int *)message_to_archive);
      message_to_archive += 4;
      printf ("Cilent %d: SID as number: %d\n", client_num, sid);

      string_info = message_to_archive;
      printf ("Cilent %d: Rest of message: %s\n", client_num, string_info);

      if (rcv_buffer[n-1] != 0) {
         /* Client has failed to put NULL at end of PDU - flag */
         printf ("Client %d: ERROR in PDU - no terminating NULL\n", client_num);
      }

      sprintf (arch_filename, "570_f18_hw9_udp.txt");
      inet_ntop (AF_INET, &(cliaddr.sin_addr), client_ad, 20);
      client_port = ntohs (cliaddr.sin_port);

      gettimeofday(&tv, NULL);
      nowtm = *(localtime (& (tv.tv_sec)));

      sprintf (header, "Client  %d: Envelope Information:\nFrom host IP %s port %d at %s :\n", client_num, client_ad, client_port, asctime (&nowtm));
      printf ("%s %s %s %d %s\nEnd of Envelope Information\n", header, ncsu, string_sid, sid, string_info);
      fp = fopen (arch_filename, "a");
      fprintf (fp, "%s %s %s %d %s\nEnd of Envelope Information\n", header, ncsu, string_sid, sid, string_info);
      if (rcv_buffer[n-1] != 0) {
         /* Client has failed to put NULL at end of PDU - flag */
         fprintf (fp, "ERROR in PDU - no terminating NULL\n");
      }
      fclose (fp);

      sprintf (buf, "Server received and archived %d bytes of transmitted data\n", n);
      sendto (sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));

      sprintf (logfile_name, "UDP_client_%d.buflog", client_num);
      fp_log = fopen (logfile_name, "w");
      m = write (fileno(fp_log), (void *)rcv_buffer, n);
      fclose (fp_log);
      printf ("Client %d: written to log, status: %d, error: %d\n", client_num, m, errno);

   }
}
