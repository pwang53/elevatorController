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
#include <stdlib.h>
#include<pthread.h>

//pthread_barrier_t barrier;

//Struct of elevator
static struct Elevator
{
 	enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;
 	int current_floor;
 	int direction;
 	int occupancy;
 	int destination;						//destination
 	pthread_mutex_t lock;
	pthread_barrier_t barrier;			//used to wait for the door open
	 
} Elevator_Number[ELEVATORS] ;


//Struct of Passanger
static struct Passenger
{
	int from_floor;					//each passanger has a from floor
	int to_floor;					//each passanger has a to floor
	int elevator_passenger_take;	//the number of elevator that the passenger take
	int occupy;
	pthread_mutex_t lock_for_next;  //the mutex lock for the next passager	
	pthread_cond_t Cond_t;
} Passenger_Information[PASSENGERS];

void scheduler_init() {	
	for(int i = 0; i < ELEVATORS; i++){
		for(int j = 0; j < PASSENGERS; j++){	
    	//initialize all elevators
		Elevator_Number[i].current_floor=0;		
    	Elevator_Number[i].direction=-1;
    	Elevator_Number[i].occupancy=0;
    	Elevator_Number[i].state=ELEVATOR_ARRIVED;
    	Elevator_Number[i].destination=-1;	//initialize destination as -1 means it is ready to be used
		pthread_mutex_init(&Elevator_Number[i].lock,0);
    	pthread_barrier_init(&Elevator_Number[i].barrier, 0, 1);
	
		//initialize all passangers
		//When the there has a passange in the elevator, it is used to lock next passanger
		pthread_mutex_init(&Passenger_Information[j].lock_for_next, 0);  
		pthread_cond_init(&Passenger_Information[j].Cond_t, 0);  
		}
	}
	
}

//Global Variable: get the number of total passenger 
int total_passenger = 0;

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
	//check how many passenger request the 
	if (total_passenger <= passenger)	total_passenger = passenger;
		
	//initialize the request infromation from the specific passenger
	Passenger_Information[passenger].from_floor = from_floor;
	Passenger_Information[passenger].to_floor = to_floor;

	//Wait for the signal wich passenger can board the elevator
	//after get the information, unlock the mutex
	pthread_cond_wait(&Passenger_Information[passenger].Cond_t, 
		&Passenger_Information[passenger].lock_for_next);
	
	//after get the information, get the number of elevator that this passenger take
	int elevator_passenger_take = Passenger_Information[passenger].elevator_passenger_take;

    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
    	//make the passenger wait for the door to open
    	//and lock the elevator until it is taken by the passenger
    	//pthread_barrier_wait(&Elevator_Number[elevator_passenger_take].barrier);
        pthread_mutex_lock(&Elevator_Number[elevator_passenger_take].lock);

		//once it is taken by the passenger, let the passenger enter it and bring to the destination
        if(Elevator_Number[elevator_passenger_take].current_floor == from_floor 
			&& Elevator_Number[elevator_passenger_take].state == ELEVATOR_OPEN 
			&& Elevator_Number[elevator_passenger_take].occupancy==0) {
            enter(passenger, elevator_passenger_take);
            Elevator_Number[elevator_passenger_take].occupancy++;
            Elevator_Number[elevator_passenger_take].destination = Passenger_Information[passenger].to_floor;
			waiting=0;
        }
		
		//after the passenger getting in, unlock and keep to wait for door next needed to open
        pthread_mutex_unlock(&Elevator_Number[elevator_passenger_take].lock);
        //pthread_barrier_wait(&Elevator_Number[elevator_passenger_take].barrier);
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
    while(riding) {
    	//make the passenger wait for the door to open
    	//and lock the elevator until it is taken by the passenger
    	//pthread_barrier_wait(&Elevator_Number[elevator_passenger_take].barrier);
        pthread_mutex_lock(&Elevator_Number[elevator_passenger_take].lock);
        
        //once if the current floor arrive the distination and the door is open, let the passenger out
        //after getting out, reset the destination back to -1 means elevator is waited to be used
		if(Elevator_Number[elevator_passenger_take].current_floor == to_floor 
			&& Elevator_Number[elevator_passenger_take].state == ELEVATOR_OPEN) {
            exit(passenger, elevator_passenger_take);
            Elevator_Number[elevator_passenger_take].occupancy--;
            Elevator_Number[elevator_passenger_take].destination = -1;
            riding=0;
        }

    	//after the passenger getting out, unlock and keep waiting for door next needed to open
        pthread_mutex_unlock(&Elevator_Number[elevator_passenger_take].lock);
        //pthread_barrier_wait(&Elevator_Number[elevator_passenger_take].barrier);
    }
}


void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    //lock the ready elevator first to do the next operation
	pthread_mutex_lock(&Elevator_Number[elevator].lock);

	//check the ARRIVED and OPEN state and decide what to do next
    if(Elevator_Number[elevator].state == ELEVATOR_ARRIVED 
		&& Elevator_Number[elevator].destination == at_floor) {
        //when the state is ARRIVED and the at floor is the destination,
		//then open the elevator and change the state to open
		door_open(elevator);
        Elevator_Number[elevator].state=ELEVATOR_OPEN;
        
        //keep the door open and give the user time to step in
        //and then unlock the elevator return
        pthread_mutex_unlock(&Elevator_Number[elevator].lock);
        //pthread_barrier_wait(&Elevator_Number[elevator].barrier);  
        
		return;
    }
    else if(Elevator_Number[elevator].state == ELEVATOR_OPEN) {
    	//before close, double check to allow the user have enough time to step in
        //pthread_barrier_wait(&Elevator_Number[elevator].barrier);  
        
        //when the state is OPEN, then close the door and change the state to close
	    door_close(elevator);
        Elevator_Number[elevator].state=ELEVATOR_CLOSED;
    	
    	//unlock the elevator and return
		pthread_mutex_unlock(&Elevator_Number[elevator].lock);
    	
		return;
	}
	
	int distance = 0;
	int position = -1;
	int temp = FLOORS;
		
	for(int i = 0; i <= total_passenger; i++){
		//get the absolutly value and assign to distance
		distance = Passenger_Information[i].from_floor - at_floor;
		
		if(distance < 0) distance *= -1;
		//find the minimum distance in the array and get its position
		if(distance < temp && Passenger_Information[i].occupy == 0) {
			temp = distance;
			position = i;
		}
	}
		
	//After checking ARRIVED and OPEN state, all left are in the CLOSED condition    
	//1. when it is CLOSED and the elevator is waited to be used
    if(Elevator_Number[elevator].destination == -1 && position > -1){
    	//get the floor that the passenger locate
    	int from = Passenger_Information[position].from_floor;
		
		//give the signal that the nearest passenger can get on the elevator
    	//and then set the destination of this elevator to the from florr of the closet passenger
		Passenger_Information[position].occupy = 1;
		Passenger_Information[position].elevator_passenger_take = elevator;
		pthread_cond_signal(&Passenger_Information[position].Cond_t);					
    	Elevator_Number[elevator].destination = from;	
	}	
	
	if(Elevator_Number[elevator].destination >= 0 ){
		//2. when it is CLOSED and the elevator is in the process
		// if it is on the button or at the top of the building, change the direction
		if(at_floor==0 || at_floor==FLOORS-1)  Elevator_Number[elevator].direction*=-1;
    	
    	// going up
    	if(Elevator_Number[elevator].destination > at_floor) 
			Elevator_Number[elevator].direction = 1;
		// going down	
		else if(Elevator_Number[elevator].destination < at_floor)
		    Elevator_Number[elevator].direction = -1;
    		
		Elevator_Number[elevator].state = ELEVATOR_ARRIVED;
		move_direction(elevator,Elevator_Number[elevator].direction);
    	Elevator_Number[elevator].current_floor=at_floor+ Elevator_Number[elevator].direction;
    
	}
    //unlock the elevator to 
    pthread_mutex_unlock(&Elevator_Number[elevator].lock);
}
