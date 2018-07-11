/**
Koc University, Comp 304, Project 2
Purpose: Simulate metro route using POSIX threads

@author Ozgur Taylan Ozcan
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <queue>
#include <iostream>
#include <string>
#include <sstream>
#include <semaphore.h>
using namespace std;

#define TRAIN_LIMIT 10

pthread_mutex_t count_mutex;
pthread_cond_t cond_overload;
pthread_mutex_t log_queue_mutex;
pthread_mutex_t section_mutex;
sem_t log_mutex;
sem_t control_mutex;
sem_t tunnel_mutex;

int totalNumOfTrains = 0;
int next_id = 0;
float p;
int s_time;
int start_sec, start_min, start_hour;
time_t start_time, end_time;
queue <string> log_queue;

struct Train {
	int id;
	int length;
	char destinationPoint;
	char arrivalPoint;
};

struct Section {
	char startPoint;
	char endPoint;
	//queue <Train> qBackward;
	queue <Train*> qForward;
	int numOfTrains;	
};

struct Section section_AC, section_BC, section_DE, section_DF;

struct Tunnel{
	Train *train;
} tunnel;

int pthread_sleep (int seconds);
void *func_section(void *ptr);
void *func_controlCenter(void *arg);
void *func_tunnel(void *ptr);
void *func_log(void *arg);
string get_time();
string get_all_train_ids();

int main( int argc, char *argv[] )  {
	int seed;
  	if( argc == 4 ) {
		s_time = atoi(argv[1]);
		p = atof(argv[2]);
		seed = atoi(argv[3]);
  	}
   	else {
		printf("Invalid arguments.\n");
   	}
	seed = time(NULL);
    	srand(seed);

	start_time = time(NULL);
	end_time = start_time + s_time;
	
	/* Initialize semaphore, mutex and condition variable objects */
  	pthread_mutex_init(&count_mutex, NULL);
  	pthread_cond_init (&cond_overload, NULL);
  	pthread_mutex_init(&section_mutex, NULL);
  	pthread_mutex_init(&log_queue_mutex, NULL);
	sem_init(&log_mutex, 0, 0);
	sem_init(&control_mutex, 0, 0);
	sem_init(&tunnel_mutex, 0, 0);

	section_AC.startPoint = 'A';
	section_AC.endPoint = 'C';
	section_AC.numOfTrains = 0;
	section_BC.startPoint = 'B';
	section_BC.endPoint = 'C';
	section_BC.numOfTrains = 0;
	section_DE.startPoint = 'E';
	section_DE.endPoint = 'D';
	section_DE.numOfTrains = 0;
	section_DF.startPoint = 'F';
	section_DF.endPoint = 'D';
	section_DF.numOfTrains = 0;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_t thread_AC;
	pthread_t thread_BC;
	pthread_t thread_DE;
	pthread_t thread_DF;
	pthread_t thread_tunnel;
	pthread_t thread_controlCenter;
	pthread_t thread_log;
	pthread_create(&thread_AC, &attr, func_section, &section_AC);
	pthread_create(&thread_BC, &attr, func_section, &section_BC);
	pthread_create(&thread_DE, &attr, func_section, &section_DE);
	pthread_create(&thread_DF, &attr, func_section, &section_DF);
	pthread_create(&thread_tunnel, &attr, func_tunnel, &tunnel);
	pthread_create(&thread_controlCenter, &attr, func_controlCenter, NULL);
	pthread_create(&thread_log, &attr, func_log, NULL);
	
	pthread_join(thread_AC, NULL);
	cout << "ac" << endl;
	pthread_join(thread_BC, NULL);
	pthread_join(thread_DE, NULL);
	pthread_join(thread_DF, NULL);
	pthread_join(thread_controlCenter, NULL);	
	cout << "control" << endl;
	sem_post(&tunnel_mutex);
	pthread_join(thread_tunnel, NULL);
	cout << "tunnel" << endl;
	log_queue.push(string("Program Terminated"));
	sem_post(&log_mutex);
	pthread_join(thread_log, NULL);

	sem_destroy(&log_mutex);
	sem_destroy(&control_mutex);
	sem_destroy(&tunnel_mutex);
	pthread_attr_destroy(&attr);
  	pthread_mutex_destroy(&count_mutex);
   	pthread_cond_destroy(&cond_overload);
  	pthread_mutex_destroy(&section_mutex);
  	pthread_mutex_destroy(&log_queue_mutex);
   	exit(0);
}

void *func_section(void *ptr){
	struct Section *section = (Section*) ptr;
	float prob_train;

	if(section->startPoint == 'B'){
		prob_train = 1 - p;
	}else{
		prob_train = p;
	}

	while(time(NULL) < end_time){
		bool create_train = false;
		int train_id;
		int rand_train;
		int rand_length;
		int rand_line;

		rand_train = ( rand() % 10 ) + 1;
		if(rand_train <= (prob_train * 10)){create_train = true;}

		//cout << section->startPoint << section->endPoint << (time(NULL) - start_time - s_time) << endl;

		pthread_mutex_lock(&count_mutex);
		if(totalNumOfTrains >= TRAIN_LIMIT){	
     			pthread_cond_wait(&cond_overload, &count_mutex);
		}
		if(create_train){
			totalNumOfTrains++;
			train_id = next_id;
			next_id++;
		}
		pthread_mutex_unlock(&count_mutex);
		
		if(create_train){	
			struct Train *train = (struct Train*)malloc(sizeof(struct Train));;

			rand_length = ( rand() % 10 ) + 1;
			if(rand_length <= 3){
				train->length = 200;
			}else{
				train->length = 100;
			}
			
			train->id = train_id;
			train->destinationPoint = section->startPoint;

			rand_line = ( rand() % 10 ) + 1;
			if(section->startPoint == 'A' || section->startPoint == 'B'){
				if(rand_line <= 5){
					train->arrivalPoint = 'E';
				}else{
					train->arrivalPoint = 'F';
				}
			}else{
				if(rand_line <= 5){
					train->arrivalPoint = 'A';
				}else{
					train->arrivalPoint = 'B';
				}
			}

			pthread_sleep(1);

			pthread_mutex_lock(&section_mutex);
			(section->qForward).push(train);
			section->numOfTrains = section->numOfTrains + 1;
			sem_post(&control_mutex);
			pthread_mutex_unlock(&section_mutex);

			//cout << "train created" << endl;
			//pthread_mutex_lock(&log_queue_mutex);
			///**********/log_queue.push("train created: " + train.arrivalPoint + train.destinationPoint);
			///**********/sem_post(&log_mutex);
			//pthread_mutex_unlock(&log_queue_mutex);

		}else{
			pthread_sleep(1);
		}

	}
	
	pthread_exit(NULL);
}


void *func_tunnel(void *ptr){
	struct Tunnel *tunnel = (Tunnel*) ptr;

	while(true){
		sem_wait(&tunnel_mutex);
		if(time(NULL) >= end_time) break;
		pthread_sleep(1);
		pthread_mutex_lock(&count_mutex);
		totalNumOfTrains--;
		pthread_mutex_unlock(&count_mutex);		
	}

	pthread_exit(NULL);
}


void *func_controlCenter(void *arg){
	int clearance_start_time;
	bool clearance = false;
	struct Train *train;

	while(time(NULL) < end_time){

		pthread_mutex_lock(&count_mutex);
		if(totalNumOfTrains >= TRAIN_LIMIT){
			pthread_mutex_lock(&log_queue_mutex);
			ostringstream ss;
			ss << "System Overload\t\t" << get_time() << "\t#\t\t" << get_all_train_ids();
			log_queue.push(ss.str());
			///**********/log_queue.push(string("System Overload\t\t") + string("Time\t\t#\t\tTrain ids"));
			/**********/sem_post(&log_mutex);
			pthread_mutex_unlock(&log_queue_mutex);
			clearance = true;
			clearance_start_time = time(NULL);
		}
		
		if(clearance){			
			if(totalNumOfTrains <= 1){
				int clearance_time = time(NULL) - clearance_start_time;
				pthread_mutex_lock(&log_queue_mutex);
				ostringstream ss;
				ss << "Tunnel Cleared\t\t" << get_time() << "\t#\t\tTime to Clear: " << clearance_time;
				log_queue.push(ss.str());
				///**********/log_queue.push(string("Tunnel Cleared\t\t") + string("Time\t\t#\t\tTime to Clear: "));
				/**********/sem_post(&log_mutex);
				pthread_mutex_unlock(&log_queue_mutex);
				clearance = false;
				pthread_cond_broadcast(&cond_overload);
			}
		}
		pthread_mutex_unlock(&count_mutex);


		sem_wait(&control_mutex);
		pthread_mutex_lock(&section_mutex);
		if(section_AC.numOfTrains == section_BC.numOfTrains 
			&& section_BC.numOfTrains == section_DE.numOfTrains 
				&& section_DE.numOfTrains == section_DF.numOfTrains){

			train = section_AC.qForward.front();
			section_AC.qForward.pop();
			section_AC.numOfTrains--;

		}else if(section_AC.numOfTrains < section_BC.numOfTrains 
			&& section_BC.numOfTrains == section_DE.numOfTrains 
				&& section_DE.numOfTrains == section_DF.numOfTrains){

			train = section_BC.qForward.front();
			section_BC.qForward.pop();
			section_BC.numOfTrains--;

		}else if(section_AC.numOfTrains < section_BC.numOfTrains 
			&& section_BC.numOfTrains < section_DE.numOfTrains 
				&& section_DE.numOfTrains == section_DF.numOfTrains){

			train = section_DE.qForward.front();
			section_DE.qForward.pop();
			section_DE.numOfTrains--;
		}else{
			Section* array[4];
			array[0] = &section_AC;
			array[1] = &section_BC;
			array[2] = &section_DE;
			array[3] = &section_DF;		
		
			int index = 0;

			for(int i = 1; i < 4; i++){
				if (array[i]->numOfTrains > array[index]->numOfTrains){
       					index = i;
    				}		
			}		

			//cout << array[index]->numOfTrains << endl;
			//cout << array[index]->startPoint << array[index]->endPoint << endl;	
			train = array[index]->qForward.front();
			array[index]->qForward.pop();
			array[index]->numOfTrains--;
		}
		pthread_mutex_unlock(&section_mutex);


		pthread_mutex_lock(&log_queue_mutex);
		ostringstream ss;
		ss << "Tunnel Passing\t\t" << get_time() << "\t\t" << train->id << "\t" << get_all_train_ids();
		log_queue.push(ss.str());
		///**********/log_queue.push(string("Tunnel Passing\t") + string("Time\t") + train.id + string("\tTrain ids"));
		/**********/sem_post(&log_mutex);
		pthread_mutex_unlock(&log_queue_mutex);


		int tunnel_time = 1 + train->length / 100;
		pthread_sleep(tunnel_time);

		int rand_breakdown = ( rand() % 10 ) + 1;
		if(rand_breakdown == 1){
			pthread_mutex_lock(&log_queue_mutex);
			ostringstream ss;
			ss << "Breakdown\t\t" << get_time() << "\t\t" << train->id << "\t" << get_all_train_ids();
			log_queue.push(ss.str());
			///**********/log_queue.push(string("Breakdown\t") + string("Time\t") + train.id + string("\tTrain ids")));
			/**********/sem_post(&log_mutex);
			pthread_mutex_unlock(&log_queue_mutex);	
			pthread_sleep(4);
		}
		
		tunnel.train = train;
		sem_post(&tunnel_mutex);
	}

	pthread_exit(NULL);
}

void *func_log(void *arg){
	cout << "Event\t\t\tEvent Time\tTrain ID\tTrains Waiting Passage" << endl;
	while(time(NULL) < end_time){
		sem_wait(&log_mutex);
		cout << log_queue.front() << endl;
		log_queue.pop();
	}

	pthread_exit(NULL);
}

string get_time(){
	time_t t = time(NULL);
	tm* timePtr = localtime(&t);
	int sec = (int) (timePtr->tm_sec);
	int min = (int) (timePtr->tm_min);
	int hour = (int) (timePtr->tm_hour);

	ostringstream ss;
	ss << " " << hour << ":" << min << ":"<< sec;
	return ss.str();
}

string get_all_train_ids(){
	queue <Train*> q1 = section_AC.qForward;
	queue <Train*> q2 = section_BC.qForward;
	queue <Train*> q3 = section_DE.qForward;
	queue <Train*> q4 = section_DF.qForward;
	ostringstream ss;

	while (!q1.empty()){
		ss << q1.front()->id << ",";
		q1.pop();
	}
	while (!q2.empty()){
		ss << q2.front()->id << ",";
		q2.pop();
	}
	while (!q3.empty()){
		ss << q3.front()->id << ",";
		q3.pop();
	}
	while (!q4.empty()){
		ss << q4.front()->id << ",";
		q4.pop();
	}
	
	return ss.str();
}


 /****************************************************************************** 
  pthread_sleep takes an integer number of seconds to pause the current thread 
  original by Yingwu Zhu
  updated by Muhammed Nufail Farooqi
  *****************************************************************************/
int pthread_sleep (int seconds)
{
   pthread_mutex_t mutex;
   pthread_cond_t conditionvar;
   struct timespec timetoexpire;
   if(pthread_mutex_init(&mutex,NULL))
    {
      return -1;
    }
   if(pthread_cond_init(&conditionvar,NULL))
    {
      return -1;
    }
   struct timeval tp;
   //When to expire is an absolute time, so get the current time and add //it to our delay time
   gettimeofday(&tp, NULL);
   timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

   pthread_mutex_lock (&mutex);
   int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
   pthread_mutex_unlock (&mutex);
   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&conditionvar);

   //Upon successful completion, a value of zero shall be returned
   return res;

}

