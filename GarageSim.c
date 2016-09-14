//Garage Simulator
 
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>
 
//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "keypad.h"
#include "lcd.h"
#include "usart_ATmega1284.h"

#define STEPS 8				//Stepper Motor
#define A (1 << PA2)
#define B (1 << PA3)
#define C (1 << PA4)
#define D (1 << PA5)
#define ALL_PHASES (A|B|C|D)
unsigned char step = 0;
const unsigned char drive[] = {A,	//8 steps 
                               A|B,
                               B,
                               B|C,
                               C,
                               C|D,
                               D,
                               D|A};
 
signed long goal = 0;
signed long tach = 0;
 
unsigned char keyFlag = 0;
unsigned char passwordFlag = 16;
unsigned char checkflag = 0;
char Denied[]="Access Denied";
char Granted[]="Access Granted";
char InputCheck[10];
char* PasswordRotate[]= {"19BCD","12345", "54321", "DCB91"};		//password rotation(4 codes)
unsigned char GrantKey= 0;
unsigned long RotateCount=0;
unsigned char RotateKey=0;
unsigned long RotateAmount=0;
int openclose = 0;
int resetdelay = 0;
int inputcounter = 0;
long resetcount = 0;
 
void Stepper_Init()					//turns PORT A for stepper
{
    DDRA |= ALL_PHASES;
    PORTA &= ~ALL_PHASES;
}
 
void Stepper_Write(unsigned char val)			//Moves motor
{
    PORTA = (PORTA & (~ALL_PHASES)) | drive[val];
}
 
void Stepper_Move(signed char dir)
{
    if (dir > 0)
    {
        step++;
        if (step == STEPS) step = 0;   
    }
    else if (dir < 0)
    {
        if (step == 0) step = STEPS;
        step--;
    }
     
    Stepper_Write(step);
}
 
 
void StepperTask()				//RTOS Stepper Task 
{
    Stepper_Init();
    while(1)
    {      
        if(GrantKey==1){
            if (openclose == 0)
            {
                if (RotateAmount>15000)
                {
                    GrantKey=0;
                    openclose = 1;
                }
				else{
                RotateAmount++;
                Stepper_Move(1);
				}
            }
            else if (openclose == 1)
            {
                if (RotateAmount<=0)
                {
                    GrantKey = 0;
                    openclose = 0;
                }
				else {
					if ( (PINA & (1 << 7)))
					{
						openclose = 0;
					}
				  RotateAmount--;
				  Stepper_Move(-1);
				}
            }
        }
 
        vTaskDelay(2);
    }
     
}
 
 
char str[] ="Enter Password:                 ";
char holder[]="Enter Password:                 ";
char storeValue;
 
void LCD_Show()
{
        str[15+inputcounter] = keyFlag ;
        LCD_DisplayString(1, str);
}
 
void Passwordcheck()
{
    if (strcmp(InputCheck,PasswordRotate[RotateKey])==0)
    {
        LCD_init();
        LCD_DisplayString(1, Granted);
        storeValue= str[15];
        checkflag=0;
        for (int i = 0; i < 32; i++) {
            str[i] = holder[i];
        }
        str[15]= storeValue;
        memset(InputCheck,'\0',10);
        passwordFlag=15;
        GrantKey = 1;
        resetdelay=1;
        inputcounter=0;
    }
    else
    {
        LCD_init();
        LCD_DisplayString(1, Denied);
        checkflag=0;
        storeValue= str[15];
        for (int i = 0; i < 32; i++) {
            str[i] = holder[i];
        }
        str[15]=storeValue;
        memset(InputCheck,'\0',10);
        passwordFlag=15;
        resetdelay=1;
        inputcounter=0;
    }
}
 
void KeyTask()
{
    unsigned char pressed = 0;
    Keypad_Init();
     
    while(1)
    {
		
        unsigned char btn = GetKeypadKey();
         
        if (pressed)
        {
            if (btn == 0)
            {
                pressed = 0;   
            }
             
        }
        else
        {
            if (btn)
            {
                pressed = 1;
                keyFlag = btn;
                if(keyFlag != '#'){
                    InputCheck[inputcounter]= keyFlag;
                }
				inputcounter++;
            }
        }
         
        vTaskDelay(1);
    }
}
 
 
void LCDTask()				// LCD Screen Task to output KeyCodes
{
    LCD_init();
    LCD_Show();
     
    while(1)
    {
        if(resetdelay == 0)
        {
            if (keyFlag) LCD_Show();
            if(keyFlag =='#')
            {
                Passwordcheck();
            }
 
            if (RotateCount==0)
            {
                RotateKey=0;
                str[15]='A';
                LCD_DisplayString(1,str);
            }
            else if (RotateCount==10000)
            {
                RotateKey=1;
                str[15]='B';
                LCD_DisplayString(1,str);
            }
            else if (RotateCount==20000)
            {
                RotateKey=2;
                str[15]='C';
                LCD_DisplayString(1,str);
            }
            else if (RotateCount==30000)
            {
                RotateKey=3;
                str[15]='D';
                LCD_DisplayString(1,str);
            }
            else if (RotateCount>=40000)
            {
                RotateCount=-1;
            }
            keyFlag = 0;
            RotateCount++;
        }
       else if (resetdelay == 1)
        {
            resetcount++;
            if (resetcount>5000)
            {
                resetcount=0;
                resetdelay=0;
				LCD_DisplayString(1,str);
            }
        }
        vTaskDelay(1);
    }  
}
#define FOLLOWER
#ifdef FOLLOWER
#define USART_NUM 0
#define UDR   UDR0

enum FollowerStateT { INIT, LOOP} followerState;
void FollowerTask()
{
	followerState = INIT;
	
	while(1)
	{
		switch (followerState)
		{
			case INIT:
			initUSART(USART_NUM);
			followerState = LOOP;
			
			case LOOP:
			if (USART_HasReceived(USART_NUM))
			{
				if (UDR){ 
					if (GrantKey==1){
						GrantKey=0;
						if (openclose==0)
							openclose=1;
						else
							openclose=0;
					}
					else
						GrantKey=1;
				}

			}
			
			break;
			
			default:
			followerState = INIT;
		}
		vTaskDelay(1);
		
	}
}
#endif

 
int main(void)
{
    PORTB |= 0x3F; // button pullups
     
    //Start Tasks
    xTaskCreate(StepperTask, (signed portCHAR *)"StepperTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	    xTaskCreate(KeyTask, (signed portCHAR *)"KeyTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate(LCDTask, (signed portCHAR *)"LCDTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	xTaskCreate(FollowerTask, (signed portCHAR *)"FollowerTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    //RunSchedular
 
    vTaskStartScheduler();
     
    return 0;
}