#define F_CPU 3686400UL
#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <stdint.h>

#define lcd_port PORTB          // we have connected the lcd on port D
#define lcd_data_dir  DDRB      // we're defining the direction of the pins, weather it is input or output
#define rs PB0                  // we need total six pin to show output on the LCD. Datapin 4, 5, 6, and 7 for sending the data to the lcd and Enable and RS pin for controlling 
                                // the behavior of the LCD we have connected RS pin of LCD to port D pin 0
#define en PB1                  // we have connected Enable pin of LCD to port D pin 1

void initialize (void);
void lcd_command( unsigned char );
void lcd_clear();
void lcd_print (char *);
/*
Using LCD with avr microcontrollers is a bit tricky. It is hard to explain in code comments. I highly recommend to watch the following video and read the article 
to have a clear understanding of how LCD works with microcontrollers 
Video: https://www.youtube.com/watch?v=7-DK8kNHvlA&list=PLE72E4CFE73BD1DE1&index=19
Article: https://newbiehack.com/MicrocontrollersABeginnersGuideIntroductionandInterfacinganLCD.aspx
The article clearly explains the working of the LCD. Please read it throughly 
*/
void initialize (void)
{
	lcd_data_dir = 0xFF;     // this will set the LCD pins connected on the microcontroller as output
	_delay_ms(15);           // to show data on the LCD we need to send commands first then the data
	lcd_command(0x02);       // this command returns the cursor to the first row and first column position
	lcd_command(0x28);       // please refer to this link to understand meaning of all the commands https://www.electronicsforu.com/technology-trends/learn-electronics/16x2-lcd-pinout-diagram
	lcd_command(0x0c);
	lcd_command(0x06);
	lcd_command(0x01);
	_delay_ms(2);
}

void lcd_command( unsigned char cmnd )
{
	// in order to send command to the lcd first we need to write the command on the data pins. then set the RS pin to zero and enable pin to high
	// then wait for one microseconds and set the enable pin to low, this process repeats again. We're using 4 bit data communication but the data is 8-bit
	// so we will send the data divinding it into two section. Higher 4 bit and lower 4 bit
	// the following lines of codes are used to send higher 4 bits of data
	lcd_port = (lcd_port & 0x0F) | (cmnd & 0xF0);  // this line writes the command on the data pins of the lcd connected to th microcontroller portD pin 4 to 7
	lcd_port &= ~ (1<<rs);
	lcd_port |= (1<<en);
	_delay_us(1);
	lcd_port &= ~ (1<<en);
	
	// wait 200 microseconds
	_delay_us(200);
	
	// send the lower 4 bit of the data
	lcd_port = (lcd_port & 0x0F) | (cmnd << 4);
	lcd_port |= (1<<en);
	_delay_us(1);
	lcd_port &= ~ (1<<en);
	_delay_ms(2);
}

void lcd_clear()
{
	lcd_command (0x01);   // this line clears the LCD screen
	_delay_ms(2);         // waits for two milliseconds 
	lcd_command (0x80);   // this line sets the cursor to the row 1 column 1
}


void lcd_print (char *str)
{
	// this function will be used to display the string on the LCD screen
	int i;
	for(i=0; str[i]!=0; i++)
	{
		// we can not send the whole string to the LCD we need to send character by character
		// data sending is same as sending a command. there is one difference, in this case the RS pin will be set to HIGH while the RS pin was set to zero in case of the command sending
		lcd_port = (lcd_port & 0x0F) | (str[i] & 0xF0);
		lcd_port |= (1<<rs);
		lcd_port|= (1<<en);
		_delay_us(1);
		lcd_port &= ~ (1<<en);
		_delay_us(200);
		lcd_port = (lcd_port & 0x0F) | (str[i] << 4);
		lcd_port |= (1<<en);
		_delay_us(1);
		lcd_port &= ~ (1<<en);
		_delay_ms(2);
	}
}



void lcd_setCursor(unsigned char x, unsigned char y){    // this function will be used to set cursor. the place where we want to display the data
	unsigned char adr[] = {0x80, 0xC0};    // the 16x2 LCD has two rows first row has a value of 0x80. So let's say we want to go to the seconds column of first row
										   // we just need to send the command with adding 2 with the initial value. So, it will be (0x80 + 2) this is how the code works
	lcd_command(adr[y-1] + x-1);
	_delay_us(100);
}


void initUART()
{ 
	DDRD |= 1 << PIND1;
	 int baud = 25;          //   We're defining the speed of the communication with LCD   

     UBRRH = (unsigned char) (baud >> 8);    //Put the upper part of the baud number here (bits 8 to 11)

     UBRRL = (unsigned char) baud;         //Put the remaining part of the baud number here

     UCSRB =  (1 << TXEN);       //Enable the receiver and transmitter

     UCSRC = (1 << USBS) | (3 << UCSZ0);     //Set 2 stop bits and data bit length is 8-bit
}
//---------------------------------------------------
void sendByte(char x)
{
	/* Student: sendByte routine as done before */
	while ((UCSRA& (1<<UDRE))==0);
	UDR=x;
}
//---------------------------------------------------
void print_string(char *s)
{
	int i;
	for (i = 0; s[i] != '\0'; i++)
	sendByte(s[i]);
}
//---------------------------------------------------
void print_decimal(unsigned char x)
{
	char h, t;
	h = x / 100;
	x = x - h * 100;
	t = x / 10;
	x = x - t * 10;
	if (h > 0) {
		sendByte(h + '0');
		sendByte(t + '0');
		sendByte(x + '0');
		} else if (t > 0) {
		sendByte(t + '0');
		sendByte(x + '0');
	} else
	sendByte(x + '0');
}
//---------------------------------------------------
void print_newline()
{
	sendByte(10);
	sendByte(13);
}
int main(void)
{

	initialize();
	lcd_clear();
	_delay_ms(250);

	unsigned int result;
	char str[16];
	int previous_temp = 0;
	// Student: initialize UART
	initUART();
	// Student: configure ADC registers
	ADMUX=0xE1;
	ADCSRA=0x86;
	

	while (1)
	{

		ADCSRA |= (1 << ADSC);
		// Student: wait until conversion is completed
		while ((ADCSRA & (1 << ADIF))==0);
		result= ADCH;
		
		if (previous_temp != result)
		{
			lcd_clear();
			//Sending digital readings through UART
			print_string("Tempreture= ");
			print_decimal(result/* Student: send the result here */);
			print_newline();
		}

		lcd_setCursor(1, 1);
		lcd_print("Temperature");

		itoa(result, str, 10); // Convert integer to string DisplayText (str, 2);

		lcd_setCursor(1, 2);
		lcd_print(str);

		lcd_setCursor(3, 2);
		lcd_print("\337C");

		
		if (result> 35)
		{
			DDRD &= ~(1<<5);
			PORTD =0x80;
		}
		else
		{
			PORTD &=~((1<<7) | (1<<4));
			DDRD &= ~(1<<5);
			if (result<18)
			{
				PORTD|=(1<<4);
				DDRD |= (1<<5);
			}
			_delay_ms(10);
		}
		previous_temp = result;
		
	}

	return 0;
}
