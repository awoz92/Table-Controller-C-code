#include <stdio.h>
#include <wiringPi.h>


#define IN1 0    // wiringPi GPIO0(pin11)  
#define IN2 1    // pin12 
#define IN3 2    // pin13
#define IN4 3    // pin15

void cleanUp()
{
	pinMode(IN1, OUTPUT);  
	pinMode(IN2, OUTPUT);  
	pinMode(IN3, OUTPUT);  
	pinMode(IN4, OUTPUT);  
	
	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);
}

int main(void)
{
	if (wiringPiSetup() < 0) {  
		printf("Setup wiringPi failed!\n");  
		return -1;  
	}  
	cleanUp();
	return 0;
}
