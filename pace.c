/*
  Multiple Change Detector
  RTES 2015

  Nikos P. Pitsianis 
  AUTh 2015
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>


void exitfunc(int sig)
{
  _exit(0);
}

// Checks if all signals that changed, have been detected
void compfunc(int a,int b,int c){
  printf("There are %d signals that have been changed.\n",a);
  printf("There are %d signals that have been changed from 0 to 1.\n",b);
  printf("There are %d signals that have been detected.\n",c);
  if(b==c){
	printf("All signals that changed from 0 to 1, have been detected so far!\n\n");
	}
};

// Declare array for Signals
volatile int *signalArray;

// Declare array for previous states of signals
volatile int *prevArray;	
struct timeval *timeStamp;

// N: Number of Signals,  T: Number of Threads for Detection
int N,T,split;
int Asignals=0,Bsignals=0,Csignals=0;

FILE *f;

pthread_mutex_t SignalMutex = PTHREAD_MUTEX_INITIALIZER;

void *SensorSignalReader (void *args);
void *ChangeDetector (void *args);

int main(int argc, char **argv)
{
  int i,temp,Tstart;
  
  // Open the log files
  f = fopen("log.txt","w+");
  
  // Add column as header
  fprintf(f,"Detection Time (in seconds)\r\n\r\n");fflush(f);
 
  
  // Adds a second input during execution in order to declare the number of signals
  N = atoi(argv[1]);
 
  // In case of wrong input
  if (argc != 2) {
    printf("\nUsage: %s N \n"
           " where\n"
           " N    : number of signals to monitor\n"
	   , argv[0]);
    
    return (1);
  }
  
  // Choose the number of threads that will be used for detection
  printf ("\n\nHow many threads to use for detection ");
  printf ("\n\nWe recommend : \n"
		  "                  1 for 1-600    signals\n"
          "                  2 for 601-1200  signals\n"      
		  "                  4 for 1200+    signals\n" );
  printf ("\n\nMake your choice :         ");
  scanf ("%d",&T);
  
  if (T==0 || T>4){
	printf("\nWrong choice of Threads \n");
	return(1);
	}
  
  // Split the number of signals in T threads 
  split=N/T;
  
  printf("\n\nNumber of Signals : %d \n\n"
		"Number of Threads : %d \n\n"
		"Number of Signal per Thread : %d \n\n\n",N,T,split);
	
  // Set a timed signal to terminate the program
  signal(SIGALRM, exitfunc);
  alarm(20); // after 20 sec

  // Allocate signal,previous signal, time-stamp arrays and thread handles
  signalArray = (int *) malloc(N*sizeof(int));
  timeStamp = (struct timeval *) malloc(N*sizeof(struct timeval));
  prevArray = (int *) malloc(N*sizeof(int)); 
  
  // Declare signal generator and dignal detectors
  pthread_t sigGen;
  pthread_t sigDet[T];
  
  // Initialize Arrays
  for (i=0; i<N; i++) {
    signalArray[i] = 0;
	prevArray[i]=0;
  }
  
  // Initialize mutex
  pthread_mutex_init(&SignalMutex, NULL);

  // Create detector threads
  for (i=0; i<T; i++){
	pthread_create (&sigDet[i], NULL, ChangeDetector, (void *)(intptr_t) i);
  }
  
  // Create reader thread
  pthread_create (&sigGen, NULL, SensorSignalReader, NULL);
  
  // Wait here until the signal 
  for (i=0; i<T; i++){
	pthread_join (sigDet[i], NULL);
  }
  
  fclose (f);
 
  return 0;
}


void *SensorSignalReader (void *arg)
{

  char buffer[30];
  struct timeval tv; 
  time_t curtime;
  
  srand(time(NULL));

  while (1) {
    int t = rand() % 10 + 1; // wait up to 1 sec in 10ths
    usleep(t*100000);
    
    int r = rand() % N;
	
	// Lock
	pthread_mutex_lock(&SignalMutex);
    signalArray[r] ^= 1;
	Asignals+=1;
	
    if (signalArray[r]) {
	  Bsignals+=1;
      gettimeofday(&tv, NULL);
      timeStamp[r] = tv; 
      curtime = tv.tv_sec;
      strftime(buffer,30,"%d-%m-%Y  %T.",localtime(&curtime));
      printf("Changed    %5d   at   Time   %s%ld\n",r,buffer,tv.tv_usec);
	 
    }
	
	// Unlock
	pthread_mutex_unlock(&SignalMutex); 
  }
}

void *ChangeDetector (void *arg)
{
  char buffer[30];
  struct timeval tv; 
  time_t curtime;
  int i,Tnum,Tstart,Tend,x,y;
   
  // The number of threads used for detection
  Tnum=(intptr_t)arg;
  
  Tstart=Tnum*split;
  Tend=(Tnum+1)*split;
  
  while (1) {
    
	for(i=Tstart; i<Tend; i++)
	{
		x=prevArray[i];
		y=signalArray[i];
		if(x != y)
		{
			if(y == 1)
			{
				Csignals+=1;
				gettimeofday(&tv, NULL); 
				curtime = tv.tv_sec;
				strftime(buffer,30,"%d-%m-%Y  %T.",localtime(&curtime));
				
				// Lock
				pthread_mutex_lock(&SignalMutex); 
				printf("Detected   %5d   at   Time   %s%ld   after   %ld.%06ld   sec\n\n", i, buffer, tv.tv_usec,
                       tv.tv_sec - timeStamp[i].tv_sec,
                       tv.tv_usec - timeStamp[i].tv_usec); 
				// Unlock
				pthread_mutex_unlock(&SignalMutex); 
				
				// Lock 
				pthread_mutex_lock(&SignalMutex);
				fprintf(f,"\n%ld.%06ld\r\n",tv.tv_sec - timeStamp[i].tv_sec,tv.tv_usec - timeStamp[i].tv_usec); fflush(f);
				// Unlock
				pthread_mutex_unlock(&SignalMutex); 
				
				// Call Compare Function
				compfunc(Asignals,Bsignals,Csignals);
				
				prevArray[i]=1;
			}
			else
			{
				prevArray[i]=0;
			}
		}
	}
  }

}

