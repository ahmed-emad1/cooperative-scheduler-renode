#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include <stdio.h>
#define SIZE 20
static uint8_t msg[] = "Ready Queue is full !!\n";
static uint8_t empty_msg[] = "Ready Queue is empty !!\n";
static uint8_t taskA_msg[] = "TaskA is running\n";
static uint8_t taskB_msg[] = "TaskB is running\n";
static uint8_t taskC_msg[] = "TaskC is running\n";
static uint8_t pressedMsg[] = "Button is pressed !!\n";
static uint8_t releasedMsg[] = "Button is released !!\n";
static uint8_t counter []="tick ";
static uint8_t count1 []="000:  ";
static char buttonPressed = 1;
static char timerFlag = 0;
static volatile uint8_t stopFlag = 0;
struct Task {
	void (*name)(void) ; 
	int priority ;
	int sleeping_time; 
};



static struct Task items_readyQ[SIZE];
static int front_readyQ = -1, rear_readyQ = -1;
static struct Task items_delayQ[SIZE];
static int front_delayQ = -1, rear_delayQ = -1;
void SysTick_Handler(void);
void USART2_IRQHandler(void);
void EXTI0_IRQHandler(void);
static void sendUART(uint8_t * data, uint32_t length);
static uint8_t receiveUART(void);
void enQueue_readyQ(struct Task);
void deQueue_readyQ(void);
void enQueue_delayQ(struct Task);
void deQueue_delayQ(void);
void taskA(void);
void taskB(void);
void taskC(void);
void QueTask (void (var)(void));
void sort_ReadyQueue(void);
void sort_DelayQueue(void);
void Dispatch(void);
void delayMs(int n);
void ReRunMe(int n,void (var)(void));
void decrement(void);
int intToAscii(int number);
void init(void);
void SysTick_Handler(void)  {
	timerFlag = 1;
}


void USART2_IRQHandler(void) {
	/* pause/resume UART messages */
	stopFlag = !stopFlag;
	
	/* dummy read */
	(void)receiveUART();
}

void EXTI0_IRQHandler(void) {
		/* Clear interrupt request */
		EXTI->PR |= 0x01;
		/* send msg indicating button state */
		if(buttonPressed)
		{
				sendUART(pressedMsg, sizeof(pressedMsg));
				buttonPressed = 0;
		}
		else
		{
				sendUART(releasedMsg, sizeof(releasedMsg));
				buttonPressed = 1;
		}
}

static void sendUART(uint8_t * data, uint32_t length)
{
	 for (uint32_t i=0; i<length; ++i){
      // add new data without messing up DR register
      uint32_t value = (USART2->DR & 0x00) | data[i];
		  // send data
			USART2->DR = value;
      // busy wait for transmit complete
      while(!(USART2->SR & (1 << 6)));
		  // delay
      for(uint32_t j=0; j<1000; ++j);
      }
}

static uint8_t receiveUART()
{
	  // extract data
	  uint8_t data = USART2->DR & 0xFF;
	
	  return data;
}

static void gpioInit()
{	
    // enable GPIOA clock, bit 0 on AHB1ENR
    RCC->AHB1ENR |= (1 << 0);

    // set pin modes as alternate mode 7 (pins 2 and 3)
    // USART2 TX and RX pins are PA2 and PA3 respectively
    GPIOA->MODER &= ~(0xFU << 4); // Reset bits 4:5 for PA2 and 6:7 for PA3
    GPIOA->MODER |=  (0xAU << 4); // Set   bits 4:5 for PA2 and 6:7 for PA3 to alternate mode (10)

    // set pin modes as high speed
    GPIOA->OSPEEDR |= 0x000000A0; // Set pin 2/3 to high speed mode (0b10)

    // choose AF7 for USART2 in Alternate Function registers
    GPIOA->AFR[0] |= (0x7 << 8); // for pin A2
    GPIOA->AFR[0] |= (0x7 << 12); // for pin A3
}

static void uartInit()
{
	
    // enable USART2 clock, bit 17 on APB1ENR
    RCC->APB1ENR |= (1 << 17);
	
	  // USART2 TX enable, TE bit 3
    USART2->CR1 |= (1 << 3);

    // USART2 rx enable, RE bit 2
    USART2->CR1 |= (1 << 2);
	
	  // USART2 rx interrupt, RXNEIE bit 5
    USART2->CR1 |= (1 << 5);

    // baud rate = fCK / (8 * (2 - OVER8) * USARTDIV)
    //   for fCK = 16 Mhz, baud = 115200, OVER8 = 0
    //   USARTDIV = 16Mhz / 115200 / 16 = 8.6805
    // Fraction : 16*0.6805 = 11 (multiply fraction with 16)
    // Mantissa : 8
    // 12-bit mantissa and 4-bit fraction
    USART2->BRR |= (8 << 4);
    USART2->BRR |= 11;

    // enable usart2 - UE, bit 13
    USART2->CR1 |= (1 << 13);
}




void enQueue_readyQ(struct Task value) {
  if (rear_readyQ == SIZE - 1)
    sendUART(msg, sizeof(msg));
  else {
    if (front_readyQ == -1)
      front_readyQ = 0;
    rear_readyQ++;
    items_readyQ[rear_readyQ] = value;

  }
}

void deQueue_readyQ() {
  if (front_readyQ == -1)
    sendUART(empty_msg, sizeof(empty_msg));
  else {
    items_readyQ[front_readyQ].name();
    front_readyQ++;
    if (front_readyQ > rear_readyQ)
      front_readyQ = rear_readyQ = -1;
  }
}

void enQueue_delayQ(struct Task value) {
    if (rear_delayQ == SIZE - 1)
		{}//queue is empty
  else {
      
    if (front_delayQ == -1)
      front_delayQ = 0;
    rear_delayQ++;
    items_delayQ[rear_delayQ] = value;
  }
}

void deQueue_delayQ() {
  if (front_delayQ == -1)
	{}//queue is empty
  else {
		front_delayQ++;
    if (front_delayQ > rear_delayQ)
      front_delayQ = rear_delayQ = -1;
  }
}

void sort_ReadyQueue(){
	struct Task a; 
       for (int i = front_readyQ; i <= rear_readyQ; ++i) 
        {
            for (int j = i + 1; j <= rear_readyQ ; ++j) 
            {
                if (items_readyQ[i].priority < items_readyQ[j].priority) 
                {
                    a = items_readyQ[i];
                    items_readyQ[i] = items_readyQ[j];
                    items_readyQ[j] = a;
                }
            }
        }
}
void sort_DelayQueue(){
	struct Task a; 

       for (int i = front_delayQ; i <= rear_delayQ; ++i) 
        {
            for (int j = i + 1; j <= rear_delayQ; ++j) 
            {                                                       
                if (items_delayQ[i].sleeping_time > items_delayQ[j].sleeping_time) 
                {
                    a = items_delayQ[i];
                    items_delayQ[i] = items_delayQ[j];
                    items_delayQ[j] = a;
                }
            }
  
	}
}


void taskA ()
{
				
						sendUART(taskA_msg, sizeof(taskA_msg));
						ReRunMe(3,taskA); 
				
}

void taskB ()
{
			
						sendUART(taskB_msg, sizeof(taskB_msg));
						ReRunMe(1,taskB);
					 
				
}
void taskC ()
{
			
						sendUART(taskC_msg, sizeof(taskC_msg));
					  
					 
				
}

void QueTask (void (var)(void)){
	struct Task task1;
	task1.name=var;
	if (task1.name==taskA)
	{
		task1.priority=6;
	}
	else if (task1.name==taskB)
	{
		task1.priority=5;
	}
		else if (task1.name==taskC)
	{
		task1.priority=3;
	}
	enQueue_readyQ(task1);
	
	
	
	
}
void Dispatch(){
 sort_ReadyQueue();
 deQueue_readyQ();
}

void ReRunMe(int n,void (var)(void)){
	struct Task w;
	w.name = var;
	w.sleeping_time = n+1;
	enQueue_delayQ(w);
	sort_DelayQueue();
}
void decrement(){
	 if (front_delayQ == -1)
	 {}
	 else{
		
		for(int i = 0;i<SIZE;++i)
			{
				if(items_delayQ[i].sleeping_time==1)
				{	QueTask(items_delayQ[i].name);
					items_delayQ[i].sleeping_time=0;
					deQueue_delayQ();
				}else items_delayQ[i].sleeping_time--;
			}
	 }
}
int intToAscii(int number) {
   return '0' + number;
}
void init(){
	/* startup code initialization */
	  SystemInit();
    SystemCoreClockUpdate();
	  /* intialize GPIO */
	  gpioInit();
		/* intialize UART */
	  uartInit();
	  /* enable SysTick timer to interrupt system every second */
		SysTick_Config(SystemCoreClock/10);
	  /* enable interrupt controller for USART2 external interrupt */
		NVIC_EnableIRQ(USART2_IRQn);
		/* Unmask External interrupt 0 */
		EXTI->IMR |= 0x0001;
	  /* Enable rising and falling edge triggering for External interrupt 0 */
		EXTI->RTSR |= 0x0001;
		EXTI->FTSR |= 0x0001;
	  /* enable interrupt controller for External interrupt 0 */
		NVIC_EnableIRQ(EXTI0_IRQn);
}
int main()
{	 
	  
		init(); 
	
		QueTask(taskA);
		QueTask(taskB);
		QueTask(taskC);
		
		int count =0;
	  while(count<SIZE)
		{

			if(timerFlag && !stopFlag)
				{
					

					count1[2] = intToAscii(count%10);
					count1[1] = intToAscii(count/10);
					count1[0] = intToAscii(count/100);
					
					sendUART(counter, sizeof(counter));
					sendUART(count1, sizeof(count1));
					
					Dispatch();
					decrement();
					count++;
					timerFlag =0;
				
					
				}
		}
}

