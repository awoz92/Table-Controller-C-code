//Solar Simulator

//to compile use: gcc solar_simulator.c -o solar_simulator -lm -lwiringPi
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

//Notes:
/*
*For the 28BYJ-48 unipolar stepper motor 512 steps ---- 360 angle
*The SCA100T inclinometer only measures between +- 30 degrees
*exit program with Ctrl + C	
*Actual nominal offset after calibration 983.00, 1024.00 is the ideal value which corresponds to the analog value at 0 degrees
*Size of buff is 3 bytes,send command to SCA100T in one byte
then in the next two array elements will be the 11 bits of data read from the SCA100T
*/


#define IN1 0    // pin11  
#define IN2 1    // pin12 
#define IN3 2    // pin13
#define IN4 3    // pin15

#define CSB 0 // chip select channel 0
#define GETX 0x10 //sca100t command to read from x register
#define GETY 0x11 //sca100t command to read from y register

//functions
int axis_sample_average(unsigned char axis);
double get_angle(char *xy);
void setStep(int a1, int b1, int a2, int b2);
void forward(int delay_time, int steps);
void backward(int delay_time, int steps);
void reset_position(void);
void set_angle(int angle);
int get_input(void);

//global variables
float device_sens = 1638.00;	
float nominal_offset = 1024.00;
uint16_t datax,datay;
unsigned char buff[2];


int main()
{	
	//setup wiringPi
	if (wiringPiSetup() < 0) {  
	printf("Setup wiringPi failed!\n");  
	return -1;  
	} 
	
	//setup SPI bus pins
	if (wiringPiSPISetup(CSB,500000) < 0)
		return -1;
		
	//set motor control pins as outputs
	pinMode(IN1, OUTPUT);  
	pinMode(IN2, OUTPUT);  
	pinMode(IN3, OUTPUT);  
	pinMode(IN4, OUTPUT); 
	
	int angle_des; // desired angle
	
	buff[0] = 0x00; //set measure mode
	wiringPiSPIDataRW(CSB,buff,2);	
	usleep(100000);

	reset_position(); //reset platform to 0 degrees
	
	while(1){
		
		angle_des = get_input(); // get desired angle from user
		
		set_angle(angle_des); //set desired angle
		
		delay(1000); // 1s
	}

	return 0;
}


int get_input(void)
{
	char angles[13][3] = {"0","5","10","15","20","25","30","-5","-10","-15","-20","-25","-30"};
	int angle_length = sizeof(angles)/sizeof(angles[0]);
	char input[3];
	int result,angle_des;	

	printf("Choose an angle at intervals of 5 degrees, between (-30,+30) degrees\n\n");
	printf("Type desired angle: ");
	scanf("%s",input);
	printf("desired angle: %s degrees\n",input);
	
	for(int i = 0; i <= angle_length - 1; i++){
		
		result = strncmp(input,angles[i],3);
		
		if(result == 0){
			angle_des = atoi(angles[i]);
			break;
		}
		else{
			continue;
		}
	}
	return(angle_des);
}


void set_angle(int angle)
{
	char buffxy[2];
	double xangle;
	
	buffxy[0] = 'x';
	xangle = get_angle(buffxy);
	
	while(!((float)angle -0.50 < xangle && xangle < (float)angle + 0.50))
	{
		datax = axis_sample_average(GETX);
		delay(50);
		buffxy[0] = 'x';
		xangle = get_angle(buffxy);
		if(xangle < (float)angle) {
			forward(3,1);
			delay(100);
		}
		else if(xangle > (float)angle){
			backward(3,1);
			delay(100);
		}
	}
	printf("**************************\n    X angle %.3f degrees\n**************************\n",xangle);
	
}


void reset_position(void)
{
	char buffxy[2];
	double xangle;
	
	buffxy[0] = 'x';
	xangle = get_angle(buffxy);
	
	while(!(-1.00 < xangle && xangle < 1.00)){
		datax = axis_sample_average(GETX);
		delay(50);
		buffxy[0] = 'x';
		xangle = get_angle(buffxy);
		if(xangle < -1.00){
			forward(3,1);
			delay(100);
		}
		else if(xangle > 1.00){
			backward(3,1);
			delay(100);
		}
  
	}
}

void setStep(int a1, int b1, int a2, int b2)  
{  
	digitalWrite(IN1, a1);     
	digitalWrite(IN2, b1);     
	digitalWrite(IN3, a2);     
	digitalWrite(IN4, b2);     
}  

void stop(void)  
{  
	setStep(0, 0, 0, 0);      
}  


//uses wave drive stepping scheme
void forward(int delay_time, int steps)  
{  
	int i;  

	for(i = 0; i < steps; i++){  
		setStep(1, 0, 0, 0);  
		delay(delay_time);  
		setStep(0, 1, 0, 0);      
		delay(delay_time);  
		setStep(0, 0, 1, 0);      
		delay(delay_time);  
		setStep(0, 0, 0, 1);      
		delay(delay_time);  
	}  
}  


// uses wave drive stepping scheme
void backward(int delay_time, int steps)  
{  
	int i;  

	for(i = 0; i < steps; i++){  
		setStep(0, 0, 0, 1);  
		delay(delay_time);  
		setStep(0, 0, 1, 0);      
		delay(delay_time);  
		setStep(0, 1, 0, 0);      
		delay(delay_time);  
		setStep(1, 0, 0, 0);      
		delay(delay_time);  
	}  
} 


double get_angle(char *xy)
{
	double angle;
	double xval,yval;
	uint16_t datax,datay;
	char temp[2];
	
	strcpy(temp,xy);
	// Get X data
	datax = axis_sample_average(GETX);
	delay(50);
	// Get Y data
	datay = axis_sample_average(GETY);
	delay(50);
	//normalize data using forula from SCA100T datasheet
	xval = (datax - nominal_offset)/device_sens;
	yval = (datay - nominal_offset)/device_sens;
	
	// Convert analog data to angles with atan()
	if(temp[0] == 'x')
		angle = atan(xval)* (180.0 / M_PI);
	else
		angle = atan(yval)* (180.0 / M_PI);
	
	if(temp[0] == 'x'){
		//to account for error at 0 degrees, x measurement error around 1.3 degrees off
		if(angle > 0){
			angle = angle - 1.3;
		}
		else{
			angle = angle + 1.3;
		}	
		return(angle);
	}
	else{
		//y measurement error off by around 1 degree
		if(angle > 0){
			angle = angle - 1.0;
		}
		else{
			angle = angle + 1.0;
		}
		return(angle);
	}	
}


int axis_sample_average(unsigned char axis)
{
	int c = 10;
	int value = 0;
	uint16_t data = 0;
	unsigned char buff[2];

	while(c--){
		buff[0] = axis; //0x10 for x, 0x11 for y;
		wiringPiSPIDataRW(CSB,buff,2); 
		usleep(200);
		buff[2] = (buff[2]>>5);
		data = (buff[1]<<3)+buff[2];
		
		value += data;
	}
	return ( value/10 );
}

