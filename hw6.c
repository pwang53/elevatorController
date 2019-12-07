.\Users\wqrzmy\Desktop\2019-12\hw6(1).c
//Peiqi Wang
//Discuss with Jason Zhang
//Citation:  https://github.com/ecurri3 (just for the brain storm, totally different)
//Reference: https://casatwy.com/pthreadde-ge-chong-tong-bu-ji-zhi.html
//			 http://pages.cs.wisc.edu/~travitch/pthreads_primer.html
//			 http://www.ibm.com/developerworks/linux/library/l-posix1.html?S_TACT=105AGX03&S_CMP=EDU
//			 https://blog.csdn.net/yzl11846/article/details/43641057
//			 http://laser.cs.umass.edu/verification-examples/elevator_con/elevator_con_1.html
//Also labsheet and help from TA in Mondays labs

#include "hw6.h"
#include <stdio.h>
#include<pthread.h>
#include <stdlib.h>
#include<unistd.h>

//Structure of Elevator
static struct E
{
	enum{ELEVATOR_ARRIVED = 1, ELEVATOR_OPEN = 2, ELEVATOR_CLOSED = 3} state;
    int ready;			//whether ready for pick up 0 is not, 1 is ready
	int current_floor;  //current floor of the elevator
	int direction;		//move direction and how many floors
	int occupancy;		//occupancy of passenger
}Elevator[ELEVATORS];

static struct P
{
    int p_id;   		//passenger id
    int from_floor;		//each passanger has a from floor
    int to_floor;		//each passanger has a to floor
    int elevator_for_passenger;		//the number of elevator that the passenger take
    pthread_barrier_t Barrier;		//barrier for the passenger
}Passenger[PASSENGERS];

int total_passenger = PASSENGERS;	//get the total passenger number
pthread_mutex_t lock;				//lock for the thread

void scheduler_init() {	
    //initialize the lock
	pthread_mutex_init(&lock,0);
  	
	//use two loop to initialize both passenger and elevator
	for(int i = 0;i<ELEVATORS;i++){
    	for(int j = 0;j<PASSENGERS;j++){
    	//initialize all passangers
        pthread_barrier_init(&Passenger[j].Barrier, NULL, 2);
        Passenger[j].elevator_for_passenger = -1;
		
		//initialize all elevators
other

		Elevator[i].current_floor=0;
        Elevator[i].direction=-1;
        Elevator[i].occupancy=0;
        Elevator[i].state=ELEVATOR_CLOSED;
        Elevator[i].ready = 1;
    	}
    }
}


other

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{
    pthread_barrier_wait(&Passenger[passenger].Barrier);
    //get the elevator number which take the passenger
	int take = Passenger[passenger].elevator_for_passenger;
    
    //initialize the request infromation from the specific passenger
    Passenger[take].from_floor = from_floor;
    Passenger[take].to_floor = to_floor;

    Elevator[take].ready = 0;
    pthread_barrier_wait(&Passenger[passenger].Barrier);
    
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
    	//make the passenger wait for the door to open
    	//and lock the elevator until it is taken by the passenger    	
		pthread_barrier_wait(&Passenger[passenger].Barrier);

		//once it is taken by the passenger, let the passenger enter it and bring to the destination		
		if(Elevator[take].occupancy == 0
			&&Elevator[take].state == ELEVATOR_OPEN){
    		enter(passenger,Passenger[passenger].elevator_for_passenger);
    		Elevator[take].occupancy++;
    		waiting = 0;
		}
		
		//after the passenger getting in, unlock and keep to wait for door next needed to open
    	pthread_barrier_wait(&Passenger[passenger].Barrier);
	}
	
    // wait for the elevator at our destination floor, then get out	
	int riding=1;
    while(riding) {
    	//make the passenger wait for the door to open
    	//and lock the elevator until it is taken by the passenger    	
		pthread_barrier_wait(&Passenger[passenger].Barrier);
	    pthread_mutex_lock(&lock);
	    
        //once if the current floor arrive the distination and the door is open, let the passenger out
        //after getting out, reset the destination back to -1 means elevator is waited to be used	   
	    if(Elevator[take].state == ELEVATOR_OPEN){
			exit(passenger, Passenger[passenger].elevator_for_passenger);
		    total_passenger--;
		    Elevator[take].occupancy--;
		    Elevator[take].ready = 1;
		    riding = 0;
		}
		
    	//after the passenger getting out, unlock and keep waiting for door next needed to open		
		pthread_mutex_unlock(&lock);
		pthread_barrier_wait(&Passenger[passenger].Barrier);
	    //pthread_mutex_unlock(&lock);
	}
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {

	//check the ARRIVED and OPEN state and decide what to do next
other

    if(Elevator[elevator].state == ELEVATOR_ARRIVED) {
        //when the state is ARRIVED and the at floor is the destination,
		//then open the elevator and change the state to open    	
        door_open(elevator);
        Elevator[elevator].state=ELEVATOR_OPEN;
        
        //double barrier wait
        pthread_barrier_wait(&Passenger[Passenger[elevator].p_id].Barrier);
        pthread_barrier_wait(&Passenger[Passenger[elevator].p_id].Barrier);
        return;
    }
    else if(Elevator[elevator].state == ELEVATOR_OPEN) {
        //when the state is OPEN, then close the door and change the state to close    	
        door_close(elevator);
        Elevator[elevator].state=ELEVATOR_CLOSED;
        return;
    }
	
	// if it is on the button or at the top of the building, change the direction	
	if(at_floor==0 || at_floor==FLOORS-1) Elevator[elevator].direction*=-1;
    
    // set current floor as the at_floor
	Elevator[elevator].current_floor = at_floor;
    
    //Situation1: when the elevator is empty and ready to pick up
	//assign the elevator the the passenger
    if(Elevator[elevator].occupancy == 0 && Elevator[elevator].ready ==1){
        //randomly select the passengers
		int p_id = rand()%PASSENGERS;
		
		//However if the passenger is not assigned with the elevator, choose again
        while(Passenger[p_id].elevator_for_passenger!=-1 ){
            p_id = rand()%PASSENGERS;
            if(total_passenger == 0){return;}		        
		}
		
		//assign this elevator to the chosen passenger
		Passenger[elevator].p_id = p_id;
        Passenger[p_id].elevator_for_passenger = elevator;
        pthread_barrier_wait(&Passenger[Passenger[elevator].p_id].Barrier);
        pthread_barrier_wait(&Passenger[Passenger[elevator].p_id].Barrier);
	}
	
	//Situation2: when the elevator is empty but not ready to pick up
	if(Elevator[elevator].occupancy == 0 && Elevator[elevator].ready == 0){
		//make the direction to the from floor and go to pick up
		Elevator[elevator].direction = Passenger[elevator].from_floor - at_floor;
		
		//move until it goes to the from floor
		if(at_floor != Passenger[elevator].from_floor){
			move_direction(elevator,Elevator[elevator].direction);
		}
		
		//mark state as ARRIVED
		Elevator[elevator].state = ELEVATOR_ARRIVED;
	}

	//Situation3: when the elevator is not empty and movING the passenger
	if(Elevator[elevator].occupancy !=0){
		Elevator[elevator].direction = Passenger[elevator].to_floor - at_floor;
		
		//move until it goes to the passenger's destination
		if(at_floor != Passenger[elevator].to_floor){
                move_direction(elevator,Elevator[elevator].direction);
        }
    	
    	//mark state as ARRIVED
        Elevator[elevator].state = ELEVATOR_ARRIVED;
	}
}
