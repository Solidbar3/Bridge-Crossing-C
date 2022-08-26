#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define INTERVAL 100

struct Node{
        int id;
        char type[6];
        char direction[11];
	bool inMotion;
	struct Node *next;
};
struct Node* nvHead = NULL; // Pointer to head of list of north vehicles
struct Node* svHead = NULL; // Pointer to head of list of south vehicles

void *vehicle_routine(void *vehicle); /* the thread */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* The Mutex */
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER; /* The Condition */

void push(struct Node** head_ref, struct Node* new_node)
{  	
    	/* 1. make next of new node as head*/
	new_node->next = *head_ref;

	/* 2. move the head to point to the new node*/
        *head_ref = new_node;

}

void init_and_push(int id,float carProb, float truckProb)
{
    	/* 1. allocate node */
  	struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    	/* 2. put in the data  */
	// Set vehicle id , condition, and next
	new_node->id = id;
        new_node->inMotion = false;
	new_node->next = NULL;
	while (strcmp(new_node->type, "Car") != 0 && strcmp(new_node->type, "Truck") != 0) {
        	// Randomly Generate cars and trucks
        	bool car = ((rand() % INTERVAL) + 1) <= carProb * INTERVAL;
        	bool truck = ((rand() % INTERVAL) + 1) <= truckProb * INTERVAL;
        	if(car) {
			strcpy(new_node->type, "Car");
       		}
       		else if(truck) {
			strcpy(new_node->type, "Truck");
       		}
	}
        // Set vehicle direction and fill lists
        int rand_tmp = rand() % 2;
        if(rand_tmp == 0) {
		strcpy(new_node->direction, "northbound");
		push(&nvHead, new_node);
	}
        else if(rand_tmp == 1) {
		strcpy(new_node->direction, "southbound");
		push(&svHead, new_node);
	}
	printf("\n%s", new_node->type);
        printf(" #%d", new_node->id);
        printf(" (%s", new_node->direction);
        printf(") arrived.\n");

}

void popNode(struct Node** head_ref, struct Node* node)
{
	// Store head node
        struct Node *temp = *head_ref, *prev;
	if(temp != NULL) {

    		//1. if head is the node
    		if(temp == node) {
      			*head_ref = temp->next;
   		}			 		
		else {
      			//2. Else, traverse to the find node
      			while(temp != node) {
				prev = temp;
				temp = temp->next;
			}

			prev->next = temp->next;
    		}
 	}

}

void deleteNode(struct Node** head_ref, struct Node* node)
{
	struct Node* temp = *head_ref, *prev;
	if(temp != NULL) {
		//1. If we want to delete the head
		if(temp == node) {
			*head_ref = temp->next;
			free(node);
		}
		else {
                	//2. Else, Traverse list unitl temp next is desired node
			while(temp != node) {
				prev = temp;
				temp = temp->next;
			}

			prev->next = temp->next;
			free(node);
			
		}
        }
	

}

void printVobList(struct Node *node)
{
	printf("Vehicles on the bridge: [");
  	while (node != NULL)
  	{
     		printf("%s ", node->type);
		printf("#%d,", node->id);
     		node = node->next;
  	}
	printf("]\n");
}

void printNvList(struct Node *node)
{
	printf("Waiting vehicles (northbound): [");
        while (node != NULL)
        {
                printf("%s ", node->type);
                printf("#%d,", node->id);
                node = node->next;
        }
        printf("]\n");
}

void printSvList(struct Node *node)
{
        printf("Waiting vehicles (southbound): [");
        while (node != NULL)
        {
                printf("%s ", node->type);
                printf("#%d,", node->id);
                node = node->next;
        }
        printf("]\n");
}

void crossPrint(struct Node* vobHead,struct Node *vehicle,int mCarCount,int mTruckCount) {
	printf("\n%s #%d (%s) is now crossing the bridge.\n", vehicle->type, vehicle->id, vehicle->direction);
	printVobList(vobHead);                                  // Print vehicles on bridge list
        printf("Now %d cars are moving.\n",mCarCount);          // print moving vehicle counts
        printf("Now %d trucks are moving.\n",mTruckCount);
        printNvList(nvHead);                                    // print vehicles on north
        printSvList(svHead);
}

void *vehicle_routine(void *args) {

        struct Node* vehicle = (struct Node*)args;
       
        // Arrive Stage

        // Safely examine condition
        pthread_mutex_lock(&mutex);
        while(!vehicle->inMotion) {
                pthread_cond_wait(&cv, &mutex);
	}
        pthread_mutex_unlock(&mutex);
	
        // Cross Stage
        sleep(2);
	
	// Leave Stage
	vehicle->inMotion = false;  // Signifies that vehicle is no longer on the bridge
	
        pthread_exit(0);
}

void traffic_control(int runCount,int schedule,int numVehicles,int delay, int delay2,float carProb,float truckProb) {

	// Inits
	struct timeval tv;
	double begin, end, end2;
	double whileLoop;
	double oneLoop;
	int vehicleDump = 0;
	int mCarCount = 0;
        int mTruckCount = 0;
	pthread_t tid[30];
        struct Node* vobHead = NULL;  // Pointer to head of list of vehicles on bridge
        struct Node* prevVehicle = (struct Node*) malloc(sizeof(struct Node));

	// Get time at beginning
        gettimeofday(&tv, NULL);
        begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
	
	// Push onto lists
	if(schedule == 6 && runCount == 1) {
		for(int i = numVehicles; i < numVehicles+10; i++) {
                        init_and_push(i, carProb, truckProb);
                }
	}
	else {
		for(int i = runCount*numVehicles; i < (runCount+1)*numVehicles; i++) {
			init_and_push(i, carProb, truckProb);
		}
	}

	while(nvHead != NULL || svHead != NULL) {
		// Inits
                int deleteCount = 0;
                struct Node* vob = vobHead;
                struct Node* frontN = nvHead;
                struct Node* frontS = svHead;
	
		// Loop over vehicle on bridge list
		while(vob != NULL) {
			if(!vob->inMotion && deleteCount == 0) {
				// Making sure thread has exited
				pthread_join(tid[vob->id], NULL);

				// Decrement moving vehicle counts
                                if(strcmp(vob->type, "Car") == 0) {
                                        mCarCount--;
				}
				else if(strcmp(vob->type, "Truck") == 0) {
                                        mTruckCount--;
				}
				printf("\n%s #%d exited the bridge\n", vob->type, vob->id);

				// Delete from vehicle on bridge list
		                deleteNode(&vobHead, vob);
				deleteCount++;
			}
			vob = vob->next;
		}

	        // Getting vehicle in front on north side
	        while (frontN != NULL && frontN->next != NULL) 
		{
			
			// Prioritize trucks in list
			if(strcmp(frontN->type, "Truck") == 0)
			      break;
       		 	frontN = frontN->next;
       		}
        	// Getting vehicle in front on south side
	        while (frontS != NULL && frontS->next != NULL) {

			// Prioritize trucks in list
                        if(strcmp(frontS->type, "Truck") == 0)
                               break;
       	        	frontS = frontS->next;
       		}
		/* CASES */		
	        if(mTruckCount < 1) {
                        // Trucks on both sides
                        if(frontN != NULL && frontS != NULL && strcmp(frontN->type, "Truck") == 0 && strcmp(frontS->type, "Truck") == 0) {
                                // First vehicle to move
                                if(strcmp(prevVehicle->type, "Car") != 0 && strcmp(prevVehicle->type, "Truck") != 0) {
                                        // We default north as first direction
                                        if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                printf("Error creating thread");
                                                return;
                                        }
                                        strcpy(prevVehicle->type, frontN->type);
                                        strcpy(prevVehicle->direction, frontN->direction);
                                        popNode(&nvHead, frontN);
                                        push(&vobHead, frontN);
                                        mTruckCount++;
                                        crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                        // Alter Condition
                                        pthread_mutex_lock(&mutex);
                                        frontN->inMotion = true;
                                }
                                else if(strcmp(prevVehicle->direction, "northbound") == 0) {
                                        // Make sure bridge is empty
                                        if(vobHead == NULL) {
		 				// Trucks alternate traffic
                                                if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontS->type);
                                                strcpy(prevVehicle->direction, frontS->direction);
                                                popNode(&svHead, frontS);
                                                push(&vobHead, frontS);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontS->inMotion = true;
                                        }
                                }
                                else if(strcmp(prevVehicle->direction, "southbound") == 0) {
                                        // Make sure bridge is empty
                                        if(vobHead == NULL) {
                                                // Trucks alternate traffic
                                                if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontN->type);
                                                strcpy(prevVehicle->direction, frontN->direction);
                                                popNode(&nvHead,frontN);
                                                push(&vobHead, frontN);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontN->inMotion = true;
                                        }
                                }
                        }
                        // Car on north, Truck on south
                        else if(frontN != NULL && frontS != NULL && strcmp(frontN->type, "Car") == 0 && strcmp(frontS->type, "Truck") == 0) {
                                // First vehicle to move
                                        if(vobHead == NULL) {
                                                // Trucks takes priority and alternates traffic
                                                if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontS->type);
                                                strcpy(prevVehicle->direction, frontS->direction);
                                                popNode(&svHead, frontS);
                                                push(&vobHead, frontS);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontS->inMotion = true;
                                        }

                        }
			// Car on south, Truck on north
                        else if(frontN != NULL && frontS != NULL && strcmp(frontN->type, "Truck") == 0 && strcmp(frontS->type, "Car") == 0) {
                                // First vehicle to move
                                        // Make sure bridge is empty
                                        if(vobHead == NULL) {
                                                // Track takes priority and alternates traffic
                                                if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontN->type);
                                                strcpy(prevVehicle->direction, frontN->direction);
                                                popNode(&nvHead, frontN);
                                                push(&vobHead, frontN);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontN->inMotion = true;
                                        }
                        }
                        // North side empty
                        else if(frontN == NULL) {
                                if(strcmp(frontS->type, "Truck") == 0) {
                                        // Make sure bridge is empty
                                        if(vobHead == NULL) {
                                                // Trucks alternate traffic
                                                if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontS->type);
                                                strcpy(prevVehicle->direction, frontS->direction);
                                                popNode(&svHead,frontS);
                                                push(&vobHead, frontS);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontS->inMotion = true;
                                        }

                                }
                        }
                        // South side empty
                        else if(frontS == NULL) {
                                if(strcmp(frontN->type, "Truck") == 0) {
                                        // Make sure bridge is empty
                                        if(vobHead == NULL) {
                                                // Trucks alternate traffic
                                                if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontN->type);
                                                strcpy(prevVehicle->direction, frontN->direction);
                                                popNode(&nvHead,frontN);
                                                push(&vobHead, frontN);
                                                mTruckCount++;
                                                crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                                // Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontN->inMotion = true;
                                        }

                                }
                        }
                }
		if(mCarCount < 3) {
			// Cars on both sides
			if(frontN != NULL && frontS != NULL && strcmp(frontN->type, "Car") == 0 && strcmp(frontS->type, "Car") == 0) {
				// First vehicle to move
				if(strcmp(prevVehicle->type, "Car") != 0 && strcmp(prevVehicle->type, "Truck") != 0) {
					// We default north as first direction
					if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                               			printf("Error creating thread");
                                		return;
                        		}
					strcpy(prevVehicle->type, frontN->type);
					strcpy(prevVehicle->direction, frontN->direction);
					popNode(&nvHead, frontN);
					push(&vobHead, frontN);
					mCarCount++;
					crossPrint(vobHead, frontN, mCarCount, mTruckCount);
					// Alter Condition
                                        pthread_mutex_lock(&mutex);
                                        frontN->inMotion = true;

				}
				// Every other vehicle
				else if(strcmp(prevVehicle->direction, "northbound") == 0) {		
					if(strcmp(prevVehicle->type, "Truck") == 0) {
						if(vobHead == NULL) {
							// Go same direction as prevVehicle
                                        		if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                		printf("Error creating thread");
                                                		return;
                                        		}
							strcpy(prevVehicle->type, frontN->type);
							strcpy(prevVehicle->direction, frontN->direction);
                                        		popNode(&nvHead, frontN);
                                        		push(&vobHead, frontN);
							mCarCount++;
                                        		crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                        		// Alter Condition
                                        		pthread_mutex_lock(&mutex);
                                        		frontN->inMotion = true;
						}
					}
					else {	
						// Go same direction as prevVehicle
                                        	if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                        	        printf("Error creating thread");
                                        	        return;
                                        	}
                                        	strcpy(prevVehicle->type, frontN->type);
                                        	strcpy(prevVehicle->direction, frontN->direction);
                                        	popNode(&nvHead, frontN);
                                        	push(&vobHead, frontN);
                                        	mCarCount++;
                                        	crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                        	// Alter Condition
                                        	pthread_mutex_lock(&mutex);
                                        	frontN->inMotion = true;
					}
				}
				else if(strcmp(prevVehicle->direction, "southbound") == 0) {	
					if(strcmp(prevVehicle->type, "Truck") == 0) {
                                               	if(vobHead == NULL) {
							// Go same direction as prevVehicle
                                        		if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                		printf("Error creating thread");
                                                		return;
                                        		}
							strcpy(prevVehicle->type, frontS->type);
							strcpy(prevVehicle->direction, frontS->direction);
                                        		popNode(&svHead, frontS);
                                        		push(&vobHead, frontS);
							mCarCount++;
                                        		crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                        		// Alter Condition
                                        		pthread_mutex_lock(&mutex);
                                        		frontS->inMotion = true;
						}
					}
					else {	
						// Go same direction as prevVehicle
                                                if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                        printf("Error creating thread");
                                                        return;
                                                }
                                                strcpy(prevVehicle->type, frontS->type);
                                                strcpy(prevVehicle->direction, frontS->direction);
                                                popNode(&svHead, frontS);
                                                push(&vobHead, frontS);
                                                mCarCount++;
                                              	crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                               	// Alter Condition
                                                pthread_mutex_lock(&mutex);
                                                frontS->inMotion = true;
					}
				}
						
			}
			// North side empty which means traffic is switching
			else if(frontN == NULL) {
				if(strcmp(frontS->type, "Car") == 0) {
					// Go same direction as previous vehicle
					if(strcmp(prevVehicle->direction, "southbound") == 0) {
						if(strcmp(prevVehicle->type, "Truck") == 0) {
							if(vobHead == NULL) {
								// Start south vehicles
                                                		if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                        		printf("Error creating thread");
                                                        		return;
                                                		}
								strcpy(prevVehicle->type, frontS->type);
								strcpy(prevVehicle->direction, frontS->direction);
                                                		popNode(&svHead, frontS);
                                                		push(&vobHead, frontS);
                                                		mCarCount++;
                                                		crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                                		// Alter Condition
                                                		pthread_mutex_lock(&mutex);
                                                		frontS->inMotion = true;
							}
						}
						else {
						
							// Start south vehicles
                                                         if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                                                                printf("Error creating thread");
                                                                return;
                                                         }
                                                         strcpy(prevVehicle->type, frontS->type);
                                                         strcpy(prevVehicle->direction, frontS->direction);
                                                         popNode(&svHead, frontS);
                                                         push(&vobHead, frontS);
                                                         mCarCount++;
                                                         crossPrint(vobHead, frontS, mCarCount, mTruckCount);
                                                         // Alter Condition
                                                         pthread_mutex_lock(&mutex);
                                                     	 frontS->inMotion = true;
						}
					}
					// Make sure bridge is empty
					else if(vobHead == NULL) {
       		                        	// Start south vehicles
						if(pthread_create(&tid[frontS->id],NULL,vehicle_routine,(void *)frontS) != 0 ) {
                        	                	printf("Error creating thread");
                                        	        return;
               	                        	}
						strcpy(prevVehicle->type, frontS->type);
						strcpy(prevVehicle->direction, frontS->direction);
                                        	popNode(&svHead, frontS);
                                        	push(&vobHead, frontS);
						mCarCount++;
						crossPrint(vobHead, frontS, mCarCount, mTruckCount);
						// Alter Condition
		                        	pthread_mutex_lock(&mutex);
                                        	frontS->inMotion = true;
					}
				}
			}
			// South side empty
			else if(frontS == NULL) {
				if(strcmp(frontN->type, "Car") == 0) {
					// Go same direction as previous vehicle
                                        if(strcmp(prevVehicle->direction, "northbound") == 0) {
						if(strcmp(prevVehicle->type, "Truck") == 0) {
							// Make sure bridge is empty
                                                        if(vobHead == NULL) {
                                                		// follow north vehicles
                                                		if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                        		printf("Error creating thread");
                                                        		return;
                                                		}
								strcpy(prevVehicle->type, frontN->type);
								strcpy(prevVehicle->direction, frontN->direction);
                                                		popNode(&nvHead, frontN);
                                                		push(&vobHead, frontN);
                                                		mCarCount++;
                                                		crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                                		// Alter Condition
                                                		pthread_mutex_lock(&mutex);
                                                		frontN->inMotion = true;
							}
						}
						else {
							// follow north vehicles
                                                	if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                                        	printf("Error creating thread");
                                                        	return;
                                                	}
                                                	strcpy(prevVehicle->type, frontN->type);
                                                	strcpy(prevVehicle->direction, frontN->direction);
                                                	popNode(&nvHead, frontN);
                                                	push(&vobHead, frontN);
                                                	mCarCount++;
                                                	crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                                	// Alter Condition
                                                	pthread_mutex_lock(&mutex);
                                                	frontN->inMotion = true;
						}

                                        }
					// Make sure bridge is empty
					else if(vobHead == NULL) {
                                        	// Start north vehicles
                                        	if(pthread_create(&tid[frontN->id],NULL,vehicle_routine,(void *)frontN) != 0 ) {
                                        	        printf("Error creating thread");
                                        	        return;
                                        	}
						strcpy(prevVehicle->type, frontN->type);
						strcpy(prevVehicle->direction, frontN->direction);
                                        	popNode(&nvHead, frontN);
                                        	push(&vobHead, frontN);
						mCarCount++;
                                        	crossPrint(vobHead, frontN, mCarCount, mTruckCount);
                                        	// Alter Condition
                                        	pthread_mutex_lock(&mutex);
                                        	frontN->inMotion = true;
					}
	                        }
			}
		}
       		// Wakeup blocked threads
       	 	pthread_cond_signal(&cv);
       		// Allow others to proceed
       		pthread_mutex_unlock(&mutex);

		// Empty vob list when done
		if(nvHead == NULL && svHead == NULL) {
			vob = vobHead;
			// Loop over vehicle on bridge list
        		while(vob != NULL) {

                		// Making sure thread has exited
                		pthread_join(tid[vob->id], NULL);
				
				// Decrement moving vehicle counts
                                if(strcmp(vob->type, "Car") == 0) {
                                        mCarCount--;
                                }
                                else if(strcmp(vob->type, "Truck") == 0) {
                                        mTruckCount--;
                                }

                		printf("\n%s #%d exited the bridge\n", vob->type, vob->id);

                		// Delete from vehicle on bridge list
                		deleteNode(&vobHead, vob);
                		vob = vobHead;
        		}
		}
		// Calculate time passed
                gettimeofday(&tv, NULL);
                end2 = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

		oneLoop = (end2-begin)/1000;

                // Handling delays triggered inside loop
                if(oneLoop != 0) {
                        if(oneLoop >= delay) {
                                if(schedule == 2 && vehicleDump == 0) { // Add 10 more vehicles
                                        vehicleDump++;
                                        if(vehicleDump == 1) {
                                                for(int i = numVehicles; i < numVehicles*2; i++) {
                                                        init_and_push(i, carProb, truckProb);
                                                }
                                        }
                                }
                                if(schedule == 5 && vehicleDump == 0) { // Add 10 more vehicles
                                        vehicleDump++;
                                        if(vehicleDump == 1) {
                                                for(int i = numVehicles; i < numVehicles*2; i++) {
                                                        init_and_push(i, carProb, truckProb);
                                                }
                                        }
                                }
                                if(schedule == 6 && vehicleDump == 0) { // Add 10 more vehicles
                                        vehicleDump++;
                                        if(vehicleDump == 1) {
                                                for(int i = numVehicles; i < numVehicles+10; i++) {
                                                        init_and_push(i, carProb, truckProb);
                                                }
                                        }
                                }
                        }
                        if(oneLoop >= delay2+delay) {
                                if(schedule == 5 && vehicleDump == 1) { // Add 10 more vehicles
                                        vehicleDump++;
                                        if(vehicleDump == 2) {
                                                for(int i = numVehicles*2; i < numVehicles*3; i++) {
                                                        init_and_push(i, carProb, truckProb);
                                                }
                                        }
                                }
                        }
                }
	}
	runCount++;
	// Calculate time passed
        gettimeofday(&tv, NULL);
        end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

	whileLoop = (end-begin)/1000;
	
	// Handle delays triggered outside the loop
	if(whileLoop < delay) {
		if(schedule == 1 && runCount == 1) {
			sleep(delay - (int)whileLoop);
			traffic_control(runCount,schedule,numVehicles,delay,delay2,carProb,truckProb);
		}
		else if(schedule == 4 && (runCount == 1 || runCount == 2)) {
			sleep(delay - (int)whileLoop);
			traffic_control(runCount,schedule,numVehicles,delay,delay2,carProb,truckProb);
		}
		else if(schedule == 6 && runCount == 1) {
			sleep(delay - (int)whileLoop);
                        traffic_control(runCount,schedule,numVehicles,delay,delay2,carProb,truckProb);
		}
	}
	else if(whileLoop < delay2) {
		if(schedule == 5 && runCount == 1) {
			sleep(delay - (int)whileLoop);
                        traffic_control(runCount+1,schedule,numVehicles,delay,delay2,carProb,truckProb);
		}
	}

}
	
int main(int argc, char *argv[])
{

        // Inits
        srand(time(NULL));
	struct timeval tv;

        // Schedule 1
        if(atoi(argv[1]) == 1) {

                // Specifics
                int numVehicles = 10;
                int delay = 10;
                float carProb = 1.0;
                float truckProb = 0.0;
		// Get time at beginning
		gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

		traffic_control(0,1, numVehicles, delay, 0, carProb, truckProb);

		// Calculate time passed		
		gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

		printf("Finished Execution in %f seconds\n", (end - begin)/1000);

        }// Schedule 2
	
        else if(atoi(argv[1]) == 2) {
		
                // Specifics
                int numVehicles = 10;
                int delay = 10;
                float carProb = 0.0;
                float truckProb = 1.0;

		// Get time at beginning
		gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                traffic_control(0,2,numVehicles,delay,0,carProb,truckProb);

                // Calculate time passed
                gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                printf("Finished Execution in %f seconds\n", (end - begin)/1000);

        }// Schedule 3
        else if(atoi(argv[1]) == 3) {

                // Specifics
                int numVehicles = 20;
                int delay = 0;
                float carProb = 0.65;
                float truckProb = 0.35;
		
		// Get time at beginning
		gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                traffic_control(0, 3,numVehicles,delay,0,carProb,truckProb);

                // Calculate time passed
                gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                printf("Finished Execution in %f seconds\n", (end - begin)/1000);


        }// Schedule 4
        else if(atoi(argv[1]) == 4) {

                // Specifics
                int numVehicles = 10;
                int delay = 25;
                float carProb = 0.5;
                float truckProb = 0.5;
		
		// Get time at beginning
		gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                traffic_control(0,4,numVehicles,delay,0,carProb,truckProb);

                // Calculate time passed for first 10 vehicles
                gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		
                printf("Finished Execution in %f seconds\n", (end - begin)/1000);


        }// Schedule 5
        else if(atoi(argv[1]) == 5) {

                // Specifics
                int numVehicles = 10;
                int delay = 3;
                float carProb = 0.65;
                float truckProb = 0.35;
                int delay2 = 10;

		 // Get time at beginning
                gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                // Start traffic control
                traffic_control(0,5,numVehicles,delay,delay2,carProb,truckProb);

                // Calculate time passed for first 10 vehicles
                gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                printf("Finished Execution in %f seconds\n", (end - begin)/1000);


        }// Schedule 6
        else if(atoi(argv[1]) == 6) {

                // Specifics
                int numVehicles = 20;
                int delay = 15;
                float carProb = 0.75;
                float truckProb = 0.25;

		// Get time at beginning
		gettimeofday(&tv, NULL);
                double begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                traffic_control(0,6,numVehicles,delay,0,carProb,truckProb);

                // Calculate time passed for program
                gettimeofday(&tv, NULL);
                double end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

                printf("Finished Execution in %f seconds\n", (end - begin)/1000);

        }// Error Checking
        else {
                printf("Invalid input");
                return 0;
        }
}
