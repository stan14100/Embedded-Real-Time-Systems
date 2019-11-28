#include<stdio.h>
#include<sys/time.h>
#include<stdlib.h>
#include<signal.h>
#include<pthread.h>
#include<string.h>
#include<time.h>
#include<unistd.h>

#define QUEUESIZE 10
struct timeval tpro,tcons;
FILE *Wifi, *Diff;
int firsttime;
typedef struct {
  char buf[QUEUESIZE][200];
  long head, tail;
  int flag;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q);
void queueDel (queue *q);

void handler(int signum){
   printf("here\n" ); // i want to check if i loose a scan
}
void *producer(void *q){
  FILE *fp;
  queue *fifo;
  fifo = (queue *)q;
  printf("*****SCANNING*****\n");
  /* We take the time when we find the Wifis */
  time_t rawtime;
  char buffer[15],path[200];
  struct tm *info;
  time( &rawtime );
  info = localtime(&rawtime);
  strftime(buffer,20,"%H:%M:%S",info);

  gettimeofday(&tpro,NULL);
  /* Open the command for reading. */
  fp = popen("/bin/sh searchWifi.sh", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    strtok(path,"\n"); // we take every wifi name by checking when we have break line
    //strcat(path,"Timestamps:"); // after every wifi name we write the times we scan them
    strcat(path,buffer);
    strcat(path, "\n");
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      //printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    strcpy(fifo->buf[fifo->tail],path);
    //printf("producer=%s\n",fifo->buf[fifo->tail]);
    if(firsttime==1){
     fifo->flag=1;
    }
    else{
      fifo->flag=0;
    }
    queueAdd (fifo);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

/* close */
pclose(fp);
}
void Writef(char* context){
  int found=0,i=0;
  char timestp[15],name[200],ch,nametemp[200];
  char *temp;
  FILE *tempFile;
  gettimeofday(&tcons,NULL);
  double timediffer = (tcons.tv_sec - tpro.tv_sec) + (tcons.tv_usec - tpro.tv_usec) / 1000000.0;
  Diff = fopen("Diff.txt","a");
  if(Diff == NULL){
     printf("Failed to open File.\n");
     exit(1);
     }
     fprintf(Diff,"elapsedtime:%lf \n",timediffer);
     fclose(Diff);
  strcpy(name,context);
  temp=strtok(name,"\"");
  temp=strtok(NULL,"\"");
  strcpy(name,temp);
  //printf("0)%s\n",name );
  //temp =strtok(NULL,":");
  temp=strtok(NULL,"\n");
  strcpy(timestp,temp);
  //printf("1)%s\n",timestp );
  Wifi=fopen("Wifi.txt", "r+");
  if (Wifi ==NULL)
  {
       printf("Failed to open file . \n" );
       exit(1);
  }
  tempFile = fopen("tempFile.txt","w");  //initiate
  if (tempFile ==NULL)
  {
       printf("Failed to open file . \n" );
       exit(1);
  }
  fclose(tempFile);
  tempFile = fopen("tempFile.txt","r+");
  if (tempFile ==NULL)
  {
       printf("Failed to open file . \n" );
       exit(1);
  }
  while (1)
  {
       ch = fgetc(Wifi);
       if (ch=='\"')// i'm searching for a start of a name
       {
            i=0;
            memset(nametemp, 0, 200); // clear the previous name
            while (1)
            {
                 ch = fgetc(Wifi);
                 if (ch == '\"') // if i find the end of the name then break
                      break;
                 nametemp[i] = ch; // write the name into temporary variable
                 i++;
            }
            putc('\"',tempFile);
            fprintf(tempFile, "%s", nametemp);
            putc('\"',tempFile);

            if (strcmp(name,nametemp)==0)
            {
                 found=1;
                 fprintf(tempFile, "%s", timestp);
            }
       }else if (ch == EOF)
       {
            break;
       }
       else
       {
            //grapse se ena allo arxeio
            putc(ch,tempFile);
       }
  }
  if (found == 0)
  {
       putc('\n',tempFile);
       putc('\"',tempFile);
       fprintf(tempFile, "%s", name);
       putc('\"',tempFile);
       fprintf(tempFile, "%s", timestp);
  }

  //write tempF to SSIDts
  rewind(Wifi);
  rewind(tempFile);
  //to tempF einai eksorismoy megalytero apo to SSIDts opote tha to epikalypsei
  while (1)
  {
       ch = fgetc(tempFile);
       if (ch == EOF)
            break;
       else
       {
            putc(ch, Wifi);
       }
  }


  fclose(Wifi);
  fclose(tempFile);
}

void *consumer(void *q){
  queue *fifo;
  char d[500];
  int step;
  fifo = (queue *)q;
  for (;;) {
    step=1;
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      // printf ("consumer: queue EMPTY.\n");
       pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    strcpy(d,fifo->buf[fifo->head]);
    //printf ("consumer: recieved %s\n", d);
    //memset(fifo->buf[fifo->head],0,500);
    if(fifo->flag==1){
      queueDel (fifo);
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notFull);
      step=0;
      Wifi=fopen("Wifi.txt", "a");
      if (Wifi ==NULL)
      {
           printf("Failed to open file . \n" );
           exit(1);
      }
      gettimeofday(&tcons,NULL);
      double timediffer = (tcons.tv_sec - tpro.tv_sec) + (tcons.tv_usec - tpro.tv_usec) / 1000000.0;
      Diff = fopen("Diff.txt","a");
      if(Diff == NULL){
         printf("Failed to open File.\n");
         exit(1);
         }
         fprintf(Diff,"elapsedtime:%lf \n",timediffer);
         fclose(Diff);

      fprintf(Wifi,"%s",d);
      fclose(Wifi);
    }
    if(step==1){
      queueDel (fifo);
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notFull);
      Writef(d);
    }
  }
  //return (NULL);
}

int main (int argc, char *argv[])
{
  /* Warning about input */
  if(argc!=2){
    printf("Give me the time between 2 searches in sec \n");
    exit(1);
  }
  FILE *CLOCK;
  clock_t start,end,total;
  struct timeval go,stop;
  gettimeofday(&go,NULL);
  start=clock(); // starting the clock

  /* Initialize Wifi File */
  Wifi=fopen("Wifi.txt", "w");
  if (Wifi ==NULL)
  {
       printf("Failed to open file . \n" );
       exit(1);
  }
  fclose(Wifi);
  CLOCK=fopen("Clock.txt", "w"); // Initialize clock file
  if (CLOCK ==NULL)
  {
       printf("Failed to open file . \n" );
       exit(1);
  }
  fclose(CLOCK);


  pthread_t pro, con;
  int check;
  float n;
  firsttime=1;
  n=atof(argv[1]); // the period between every sample
  int intpart=(int)n;
  int decimalpart= (n*1000000)-(intpart*1000000);
  /* Initialize Queue */
  queue *fifo;
  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }


  printf("Sample every %.2f sec! \n",n);
  sigset_t mask;
  struct sigaction new_action;
  struct itimerval inter;


  inter.it_interval.tv_sec= n; //we set the period between two successive timer interruptimestp
  inter.it_interval.tv_usec=decimalpart;
  inter.it_value.tv_sec= n; // we set the time for the first interrupt
  inter.it_value.tv_usec= decimalpart;

  /* Set up the structure to specify the new action */
  new_action.sa_handler= handler; // setting the handler function
  sigemptyset(&new_action.sa_mask);  // we empty the mask
  new_action.sa_flags=0; // we setting flags with zero value

  sigaction(SIGALRM, &new_action, NULL);
  check=setitimer(ITIMER_REAL, &inter, NULL);
  if(check==-1) exit(1);

  /* Create a thread for consumer */
  pthread_create (&con, NULL, consumer, fifo);

  /* Set up the mask of signals to temporarily block */
  sigfillset(&mask); // we put all standard signals in mask
  sigdelset(&mask, SIGALRM); // we delete SIGALRM from blocked signals
  for(;;){
      if(sigsuspend(&mask)==-1){
        pthread_create (&pro, NULL, producer, fifo);
        pthread_join (pro, NULL);
        gettimeofday(&stop,NULL);
        end=clock();
        total=(double)(end - start) / CLOCKS_PER_SEC;
        double elapsed=(stop.tv_sec- go.tv_sec + (stop.tv_usec - go.tv_usec)/1000000.0); // i estimate the elapsed time
        double percentage=((elapsed-total)/elapsed) *100;
        CLOCK=fopen("Clock.txt","a");
        fprintf(CLOCK, "Idle_percentage:%.4lf\n",percentage);
        fclose(CLOCK);
        firsttime=0;
      }
    }
  pthread_join (con, NULL);
  /* Clear Queue */
  queueDelete (fifo);

  return 0;
}


queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q)
{
  //q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q)
{
  //*out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
