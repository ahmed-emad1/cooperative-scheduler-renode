# cooperative-scheduler-renode
This project is developed to be a cooperative scheduler for embedded systems and the code must be developed using Keil uVison IDE and run on the STM32F4_discovery kit simulated using Renode. 

## Steps for building and running the project:							<br/>	(given that you already have Keil uVison IDE and Renode installed on your computer)<br/>

1- Download the “main.c” file <br/>
2-  Follow the instructions in the “Renode Example Guide.pdf” file in the repo to create a new project, add packs for the STM32F4 microcontroller, and run the code on renode.  <br/>
3- The output should be as :<br/>
![OutputImage](https://github.com/ahmed-emad1/cooperative-scheduler-renode/blob/main/app1.png)

4- To Run the other applications: You Should follow the same steps in order to run the other application but using the other main.c files that exist in each application's folder. 

## Project Description & Implementation:			
1. All ready to run tasks (functions) are stored in the ready queue. 
 * We created a task struct that has a function pointer of type void (that points to the function name), a priority number and a sleeping time. 
 * We implemented a ready queue with its enqueue and dequeue function using arrays since C doesn’t have a queue library . 
 
2. The ready queue is sorted based on the tasks priorities 
* For sorting we used the bubble sort depending on the priority 
* Having it in descending order having the highest priority at the head of the queue

3. A task is a just a function that returns no data and accepts no arguments. 
* We implemented a function for each task where it just outputs a string on the UART to be able to identify which task is running and we call any ReRunMe instances if it exists.
4. To insert a task to the ready queue call QueTask() and pass a pointer to the function (function name) that implements the task and the task priority. 
* We passed the function pointer and set the priority in the function depending on the task
* After setting the priority we enqueue in the ready queue
5. Supports 8 priority levels 
6. Dispatch() is used to remove the highest priority task from the queue and run it. 
In this function we sort the ready queue based on the task priority number 
We then call the dequeue function where it pops the top of the queue and executes its task. 
7. The system is initialized by a single call to the Init()function. This function creates and initializes all needed data structures. 
We implemented an init function that initializes the system clock, GPIO, and the UART
Enabled external interrupts 
8. A task may enqueue itself by calling ReRunMe function. If ReRunMe  is called with a non-zero argument as a parameter and the task name, the task will be inserted to the ready queue after a delay specified by the argument. For example, ReRunMe(5, taskA) inserts the taskA into the ready queue after 5 ticks. To implement this functionality, we added another queue called delay queue to store the delayed tasks. This queue is sorted ascendingly based on the delay/sleeping time. Once the sleeping time of the head of this queue expires, the task is dequeued from the delay queue and enqueued into the ready queue(QueTask function is called). The sleeping time of the tasks in the delayed queue is decremented by 1every tick this is done by creating a function called decrement where it loops on the delay queue and decrements all the sleeping times by 1. 
9. The tick is 100msec.



 
