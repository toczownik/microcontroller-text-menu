#include <stdint.h>
#include <stdbool.h>
#include "include/project.h"

#define BAUD_RATE 19200
#define CD (1000000 / (16 * BAUD_RATE))

#define LETTER_OFFSET ('a' - 'A')
#define ALPHABET_LENGTH (('z' - 'a') + 1)

#define BUFFERSIZE 0xF

#define BUFFER_FULL i >= (BUFFERSIZE + 1)

#define NEWLINE "\n>"

#define PIOB_PER (volatile unsigned int *) (0xfffff400) //setting IO mode
#define PIOB_OER (volatile unsigned int *) (0xfffff410) //setting the direction
#define PIOB_SODR (volatile unsigned int *) (0xfffff430) //force output
#define PIOB_CODR (volatile unsigned int *) (0xfffff434) //clear output
#define PIOB_ODSR (volatile unsigned int *) (0xfffff438)

#define PIOC_PER (volatile unsigned int *) (0xfffff600) //setting IO mode
#define PIOC_OER (volatile unsigned int *) (0xfffff610)  //setting the direction
#define PIOC_ODR (volatile unsigned int *) 0xFFFFF614 //disabling output
#define PIOC_SODR (volatile unsigned int *) (0xfffff630) //force output
#define PIOC_CODR (volatile unsigned int *) (0xfffff634) //clear output
#define PIOC_ODSR (volatile unsigned int *) (0xfffff638)
#define PIOC_PDSR (volatile unsigned int *) (0xfffff63C) //read input
#define PIOC_PUDR (volatile unsigned int *) (0xfffff660)
#define PIOC_PUER (volatile unsigned int *) (0xfffff664) //pull-up

#define PMC_PCER (volatile unsigned int*) (0xfffffC10) //peripheral clock

#define ONE_MS ((1000000/16)-1) //(frequency/period)/16-1

#define LED1 (1<<8)
#define LED2 (1<<29)

#define B1 (1<<5) //left button
#define B2 (1<<4) //right button
#define CLK_CDE (1<<4) //clock for c-e, not c-d

#define LETTER_OFFSET ('a' - 'A')

enum submenus {
	none,
	led,
	button,
	dgbu
};

void print(const char * string) {
	unsigned int i = 0;
	while(string[i] != '\0') {
		while(!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)); //check channel status & if transmitter is ready
		AT91C_BASE_DBGU->DBGU_THR = string[i];
    i++;
	}
}

void printChar(const char c) {
	while(!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)); //check channel status & if transmitter is ready
	AT91C_BASE_DBGU->DBGU_THR = c;
}

const char *put_error = "Buffer overflow\n";

static void Open_DBGU(void) {
	AT91C_BASE_PMC->PMC_IDR = AT91C_US_RXRDY | AT91C_US_TXRDY | AT91C_US_ENDRX | AT91C_US_ENDTX
														| AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE | AT91C_US_TXEMPTY
														| AT91C_US_TXBUFE | AT91C_US_RXBUFF | AT91C_US_COMM_TX | AT91C_US_COMM_RX; //disable all interrupts

	AT91C_BASE_DBGU->DBGU_CR = AT91C_US_RSTRX | AT91C_US_RXDIS | AT91C_US_RSTTX | AT91C_US_TXDIS; //reset & disable reciever & transmitter

	AT91C_BASE_PIOC->PIO_PER = AT91C_PC30_DRXD | AT91C_PC31_DTXD;
	AT91C_BASE_PIOC->PIO_PDR = AT91C_PC30_DRXD | AT91C_PC31_DTXD;
	AT91C_BASE_PIOC->PIO_ASR = AT91C_PC30_DRXD | AT91C_PC31_DTXD; //configure RxD and TxD

	AT91C_BASE_DBGU->DBGU_BRGR = CD; //configure throughput
	AT91C_BASE_DBGU->DBGU_MR = AT91C_US_PAR_NONE | AT91C_US_CHMODE_NORMAL; //configure operation mode

	AT91C_BASE_DBGU->DBGU_CR &= ~(AT91C_US_RSTRX | AT91C_US_RXDIS);
	AT91C_BASE_DBGU->DBGU_CR |= AT91C_US_RXEN; //enable reciever

	AT91C_BASE_DBGU->DBGU_CR &= ~(AT91C_US_RSTTX | AT91C_US_TXDIS);
	AT91C_BASE_DBGU->DBGU_CR |= AT91C_US_TXEN; //enable transmitter
}

bool PIT_Flag;

void Flag_Reset(void) {
	volatile unsigned int reset = AT91C_BASE_PITC->PITC_PIVR;
	reset;
}

void Interrupt_Handler(void) {
	if((AT91C_BASE_PITC->PITC_PIMR & AT91C_PITC_PITEN) && (AT91C_BASE_PITC->PITC_PISR)) { //interrupts enabled and PIT requested interrupt
		Flag_Reset();
		PIT_Flag = true;
	}
}

void Interrupt_Init(void) {
	AT91C_BASE_AIC->AIC_IDCR = (1 << AT91C_ID_SYS); //interrupt disable
	AT91C_BASE_AIC->AIC_SVR[AT91C_ID_SYS] = (unsigned int) Interrupt_Handler;
	AT91C_BASE_AIC->AIC_SMR[AT91C_ID_SYS] = AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE | AT91C_AIC_PRIOR_LOWEST;
	AT91C_BASE_AIC->AIC_ICCR = (1 << AT91C_ID_SYS); //interrupt clear
	AT91C_BASE_AIC->AIC_IECR = (1 << AT91C_ID_SYS); //interrupt enable
}

void PIT_Init(void) {
	AT91C_BASE_PITC->PITC_PIMR = ONE_MS; //also disables interrupts and stops the counter
	Flag_Reset();
	Interrupt_Init();
	AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITIEN; //start timer, enable interrupts
}

void delay_ms(unsigned int ms_number) {
	AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN;
	for(unsigned int i = 0; i < ms_number; ++i) {
		while(!PIT_Flag) {}
		PIT_Flag = false;
	}
	AT91C_BASE_PITC->PITC_PIMR &= (~AT91C_PITC_PITEN);
}

char read(void) {
	while(!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_RXRDY));
	return AT91C_BASE_DBGU->DBGU_RHR;
}

char swap(char commandBuffer[BUFFERSIZE + 1]) {
	for(unsigned int i = 0; i < BUFFERSIZE; ++i) {
		if(commandBuffer[i] >= 'A' && commandBuffer[i] <= 'Z') {
			commandBuffer[i] += LETTER_OFFSET;
		}
	}
}

bool isEqual(const char *string, const char commandBuffer[BUFFERSIZE + 1]) {
	unsigned int i = 0;
	while(string[i] != '\0' && commandBuffer[i] != '\0') {
		if(string[i] == commandBuffer[i]) i++;
		else return false;
	}
	if(i > 0) return true;
	else return false;
}

void printHelp(void) {
	printChar('\n');
	print("Choose category or function by typing its name and following it with ENTER.\nBelow is the list of avaliable categories and functions:\n");
	print("LED: setLED, clearLED, blinkLED, LEDstatus, changeLED\n");
	print("Button: readButton, pullupEn, pullupDis\n");
	print("DGBU: deviceStatus\n");
	print("Choose 'Up' to go back to main menu");
}

void mainMenu(const char commandBuffer[BUFFERSIZE + 1], enum submenus *currentSubmenu) {
	if(isEqual("help", commandBuffer)) printHelp();
	else if(isEqual("led", commandBuffer)) *currentSubmenu = led;
	else if(isEqual("button", commandBuffer)) *currentSubmenu =  button;
	else if(isEqual("dgbu", commandBuffer)) *currentSubmenu = dgbu;
	else print("\nThis command does not exist or is unavaliable from this menu!");
}

void configureGPIOButtons(void)
{
	*PIOC_PER = B1| B2;
  *PIOC_ODR = B1 | B2 ;
  *PIOC_PUER = B1| B2 ;
  *PMC_PCER = CLK_CDE;
}

void configureGPIOLEDs(void)
{
  *PIOB_PER = LED1;
  *PIOB_OER = LED1;
  *PIOB_SODR = LED1;
  *PIOC_PER = LED2;
  *PIOC_OER = LED2;
  *PIOB_SODR = LED2;
}

void blinkA(void) {
	unsigned int i = 0;
	while(i < 5) {
		delay_ms(100);
		*PIOB_CODR = LED1;
		delay_ms(100);
		*PIOB_SODR = LED1;
		i++;
  }
}

void blinkB(void) {
	unsigned int i = 0;
	while(i < 5) {
		delay_ms(100);
		*PIOC_CODR = LED2;
		delay_ms(100);
		*PIOC_SODR = LED2;
		i++;
  }
}

void changeA(void) {
	if((*PIOB_ODSR & LED1) == 0) {
		*PIOB_SODR = LED1;
	} else {
		*PIOB_CODR = LED1;
	}
}

void changeB(void) {
	if((*PIOC_ODSR & LED2) == 0) {
		*PIOC_SODR = LED2;
	} else {
		*PIOC_CODR = LED2;
	}
}

void ledMenu(const char commandBuffer[BUFFERSIZE + 1], enum submenus *currentSubmenu) {
	if(isEqual("help", commandBuffer)) printHelp();
	else if(isEqual("setled a", commandBuffer)) *PIOB_CODR = LED1;
	else if(isEqual("setled b", commandBuffer)) *PIOC_CODR = LED2;
	else if(isEqual("clearled a", commandBuffer)) *PIOB_SODR = LED1;
	else if(isEqual("clearled b", commandBuffer)) *PIOC_SODR = LED2;
	else if(isEqual("blinkled a", commandBuffer)) blinkA();
	else if(isEqual("blinkled b", commandBuffer)) blinkB();
	else if(isEqual("changeled a", commandBuffer)) changeA();
	else if(isEqual("changeled b", commandBuffer)) changeB();
	else if(isEqual("up", commandBuffer)) *currentSubmenu = none;
	else print("\nThis command does not exist or is unavaliable from this menu!");
}

void readButton(int button) {
	if((*PIOC_PDSR & button) == 0) print("\nButton is pressed");
	else print("\nButton is not pressed");
}

void buttonMenu(const char commandBuffer[BUFFERSIZE + 1], enum submenus *currentSubmenu) {
	if(isEqual("help", commandBuffer)) printHelp();
	else if(isEqual("readbutton a", commandBuffer)) readButton(B1);
	else if(isEqual("readbutton b", commandBuffer)) readButton(B2);
	else if(isEqual("pullupen a", commandBuffer)) *PIOC_PUER = LED1;
	else if(isEqual("pullupen b", commandBuffer)) *PIOC_PUER = LED2;
	else if(isEqual("pullupdis a", commandBuffer)) *PIOC_PUDR = LED1;
	else if(isEqual("pullupdis b", commandBuffer)) *PIOC_PUDR = LED2;
	else if(isEqual("up", commandBuffer)) *currentSubmenu = none;
	else print("\nThis command does not exist or is unavaliable from this menu!");
}

int power(int a, int b) {
	int result = 1;
	for(unsigned int i = 0; i < b; ++i) {
		result *= a;
	}
	return result;
}

void printNumber(int number) {
	unsigned int length = 0;
	int temp = number;
	if(temp == 0) length++;
	else {
		while(temp != 0){
			temp /= 10;
			length++;
		}
	}
	for(int i = length; i > 0; --i) {
	 printChar((number/power(10, i - 1) % 10) + '0');
	}
}

void checkStatus(void) {
	printChar('\n');
	if(*AT91C_DBGU_MR & AT91C_US_PAR_EVEN) print("Even parity\n");
	else if(*AT91C_DBGU_MR & AT91C_US_PAR_ODD) print("Odd parity	");
	else if(*AT91C_DBGU_MR & AT91C_US_PAR_SPACE) print("Space (forced 1) parity\n");
	else if(*AT91C_DBGU_MR & AT91C_US_PAR_MARK) print("Mark (forced 0) parity\n");
	else if(*AT91C_DBGU_MR & AT91C_US_PAR_NONE) print("No parity\n");
	print("Databits: 8\n");
	print("Baudrate: ");
	printNumber(BAUD_RATE);
}

void dgbuMenu(const char commandBuffer[BUFFERSIZE + 1], enum submenus *currentSubmenu) {
	if(isEqual("help", commandBuffer)) printHelp();
	else if(isEqual("device status", commandBuffer)) checkStatus();
	else if(isEqual("up", commandBuffer)) *currentSubmenu = none;
	else print("\nThis command does not exist or is unavaliable from this menu!");
}


int main(void) {
	Open_DBGU();
	configureGPIOLEDs();
  configureGPIOButtons();
  PIT_Init();
	print("Simple menu by Jan Szataniak\n");
	print("Write 'help' to obtain more information");
	print(NEWLINE);
	char commandBuffer[BUFFERSIZE + 1] = "\0";
	enum submenus currentSubmenu = none;
	unsigned int i = 0;
	while(1) {
		char letter = read();
		if(letter == '\n') {
			commandBuffer[i] = '\0';
			for(unsigned int j = 0; j < i; ++j) {
				printChar(commandBuffer[j]);
			}
			swap(commandBuffer);
			switch(currentSubmenu) {
				case none:
					mainMenu(commandBuffer, &currentSubmenu);
					break;
				case led:
					ledMenu(commandBuffer, &currentSubmenu);
					break;
				case button:
					buttonMenu(commandBuffer, &currentSubmenu);
					break;
				case dgbu:
					dgbuMenu(commandBuffer, &currentSubmenu);
					break;
			}
			print(NEWLINE);
			switch(currentSubmenu) {
				case led:
					print("LED>");
					break;
				case button:
					print("BUTTON>");
					break;
				case dgbu:
					print("DGBU>");
					break;
			}
			i = 0;
		} else if(BUFFER_FULL) {
			print(put_error);
			i = 0;
		} else {
			commandBuffer[i] = letter;
			i++;
		}
	}
	return 1;
}

