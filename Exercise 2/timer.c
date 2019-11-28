#include<stdio.h>
#include<sys/time.h>
#include<stdlib.h>
#include<signal.h>

int counter;
struct timeval start,stop;
FILE *timestamp;
double *timestamps;
void handler(int signum){
   // i make a file to store times between every interrupt
  counter++; // i increase counter every time i take sample
  gettimeofday(&stop, NULL);
  double elapsed=(stop.tv_sec-start.tv_sec)*1000000 + stop.tv_usec-start.tv_usec; // i estimate the elapsed time
  timestamp=fopen("deigmata.txt","a");
  if(timestamp!=NULL){
    timestamps[counter-1]=elapsed;
    fprintf(timestamp,"%d sample: %.0lf \n",counter, elapsed); // i write to the file the  time after every sample
    fclose(timestamp);

  }
}

int main(int argc, char *argv[])
{
  if(argc!=3){
    printf("Put 2 arguments: First for period and second for number of interrupts \n");
    exit(1);
  }
  timestamp=fopen("deigmata.txt","w"); // for recreate the file if already exist to clean the previous
  fclose(timestamp);

  int deigm,check;
  float n;
  counter=0; // initialize counter

  n=atof(argv[1]); // the period between every sample
  deigm=atoi(argv[2]);// the number of
  int intpart=(int)n;
  int decimalpart= (n*1000000)-(intpart*1000000);

  printf("Sample every %.2f sec and number of samples: %d! \n",n,deigm);
  sigset_t mask;
  struct sigaction new_action;
  struct itimerval inter;


  timestamps=(double*)malloc(deigm*sizeof(double));
  inter.it_interval.tv_sec= n; //we set the period between two successive timer interrupts
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
  gettimeofday(&start, NULL); // getting the starting time so as to count what time every sample taken

  /* Set up the mask of signals to temporarily block */
  sigfillset(&mask); // we put all standard signals in mask
  sigdelset(&mask, SIGALRM); // we delete SIGALRM from blocked signals
  while(counter != deigm){
    sigsuspend(&mask); // we loop sigsuspend for the given number of samples we want, sigsuspend stops every time sigaction makes a SIGALRM signal
  }
  printf("Interrupt time in microseconds: \n");
  int j;
  for(j=0; j<counter; j++) printf("%.0lf\n",timestamps[j]); 
 printf("Timer finally ends (see deigmata.txt for more infos) \n");


}
