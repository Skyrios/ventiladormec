/*
 * Blink.c
 * 
 * Author : Álvaro Batista Lima
 *Matricula: 119210487
 */ 

#define BAUD 9600
#define myubrr F_CPU/16/BAUD-1
#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nokia5110.h"
#include <avr/eeprom.h>

#define tam_vetor 3
uint16_t tempo_ms = 0,tempo = 0;
uint8_t bpm = 0, cont=0;
uint8_t controle = 0, z=0, h=0;
uint8_t temperatura=35,saturacao=90;
uint8_t PerOxigenio = 0;
unsigned int FreqRespiracao = 30;
unsigned char var[tam_vetor];
int leitura = 0, selecao=0; 
int posicao=0,padrao=0,count=0,cont_int=0,cont_main=0;
unsigned char recebido[9] = {198,198,198,198,198,198,198,198,198};
unsigned char dados[9] = {198,198,198,198,198,198,198,198,198};
char EP[9] = {0,0,0,0,0,0,0,0,0};
char EZ[9] = {0,0,0,0,0,0,0,0,0};
char EQ[9] = {0,0,0,0,0,0,0,0,0};
char EL[9] = {0,0,0,0,0,0,0,0,0};
void respirar (uint8_t);

ISR(USART_RX_vect)
{			
	if(posicao > 8)
		posicao = 0;
	
	recebido[posicao] = UDR0;
	dados[posicao] = recebido[posicao];	
	if(recebido[posicao] >= '0' && recebido[posicao] <= '9')
			recebido[posicao] = 'n';		
	posicao++;	
	cont_int++;
	
	if(z == 1){
		eeprom_write_byte(posicao,dados[posicao]);
		EP[posicao] = eeprom_read_byte(posicao);
	} else if(z == 2){
		eeprom_write_byte(posicao,dados[posicao]);
		EZ[posicao] = eeprom_read_byte(posicao);
	}else if(z == 3){
		eeprom_write_byte(posicao,dados[posicao]);
		EQ[posicao] = eeprom_read_byte(posicao);
	}else if (z == 4){
		eeprom_write_byte(posicao,dados[posicao]);
		EL[posicao] = eeprom_read_byte(posicao);
	
	}else if(z == 5){
		z = 1;
	}
	}

ISR(TIMER0_COMPA_vect){
	tempo++;
	tempo_ms++;
	
	uint16_t delay;
	delay = (uint16_t) 3750/FreqRespiracao;
	
	if(tempo == delay){
		respirar(cont);
		tempo = 0;
		cont++;
		if(cont >= 17)
			cont = 0;
	}		
		if(FreqRespiracao > 30){
			FreqRespiracao = 30;

		}
		else if (FreqRespiracao < 5){
			FreqRespiracao = 5;
		}
}

ISR(INT0_vect){
	switch(selecao)
	{
		case 1:
			FreqRespiracao++;
		break;
		case 2:
			if(OCR1A < 4000)
				OCR1A += 200;
		break;		
	}
}

ISR(INT1_vect){
	
		switch(selecao)
		{
			case 1:
				FreqRespiracao--;
			break;
			case 2:
				if(OCR1A > 2000)
					OCR1A -= 200;
			break;			
		}
}

ISR(PCINT2_vect)
{
		uint8_t PINO_D4 =  PIND & 0b00010000;
		static uint16_t t_up=0,t_down=0,t_aux=0;		
		
		if(PINO_D4 == 16)
		{
			t_up = tempo_ms;				
		}	
		else
		{
			t_down = tempo_ms;
		}
		if(t_up>t_down)
		{
			t_aux = 2*(t_up-t_down);
			bpm = 60000/t_aux;			
		}
		else if(t_down>t_up)
		{
			t_aux = 2*(t_down-t_up);
			bpm = 60000/t_aux;
		}								
}

ISR(PCINT0_vect)
{
	int B6 = PINB & 0b01000000;
	if(B6 == 0)
	{
		selecao++;
		if(selecao > 3)
			selecao = 0;
	}
}

int main(void)
{
    nokia_lcd_init();
    nokia_lcd_clear();
	uint8_t  cont = 0; //FreqRespiracao deve ser entre 5~30	
	
	UBRR0H = (unsigned char)(myubrr>>8);	
	UBRR0L = (unsigned char)myubrr;
	UCSR0B = ((1<<RXEN0)|(1<<RXCIE0));
	UCSR0C = (3<<UCSZ00);
		
	DDRB = 0b10111111; // habilita os pinos B como saida
	PORTB = 0b01000000;
	DDRD = 0b10000000; //habilita os pinos D como entrada
	PORTD = 0b00101100; 
	
	ICR1 = 39999;
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011010;
	
	OCR1A = 2000;//1ms - oxigenio 
	OCR1B = 2000;//2ms - bomba
	
	TCCR0A = 0b00000010;
	TCCR0B = 0b00000011;
	OCR0A = 249;
	TIMSK0 = 0b00000010;
	
	ADMUX = 0b01000000;
	ADCSRA = 0b11100111;
	ADCSRB = 0x00;
	DIDR0 = 0b00111100;
	
	EICRA = 0b00001010;
	EIMSK = 0b00000011;
	PCICR = 0b00000101;
	PCMSK2 |= (1<<PCINT20);
	PCMSK0 |= (1<<PCINT6);
	sei();   
					
	while (1) {			
		PerOxigenio = (OCR1A-2000)/20;
		if(cont_int != 0)
			cont_main++;
		if(cont_main > 9)
			cont_int++;
			if(cont_int > 9){
				analisa_vetor();
				cont_main = 0;
				cont_int = 0;
				posicao = 0;
			}
		DIDR0 = 0b00111111;
		double temp=0,sat=0;	
		if(tempo_ms%150 == 0)
		{	
			if(leitura == 0)
			{
				DIDR0 = 0b01111101;
				ADMUX = 0b01000000;
				temp = 0.04886*ADC+10.0163;
				temperatura = (uint8_t)temp;
			    DIDR0 = 0b01111111;
				leitura = 1;
			} 
			else
			{
				DIDR0 = 0b01111110;
				ADMUX = 0b01000001;
				sat = 0.1223*ADC;
				saturacao = (uint8_t)sat;
				DIDR0 = 0b01111111;
				leitura = 0;
			}
			if(temperatura < 35 || temperatura > 41 || saturacao < 60)
				PORTD |= 0b10000000;
			else
				PORTD &= 0b01111111;		
		}
				
		nokia_lcd_clear();		
		switch(selecao){
			case 0:
			nokia_lcd_write_string("Sinais Vitais:", 1);
			int2string(bpm, var);					
			nokia_lcd_set_cursor(0, 10);
			nokia_lcd_write_string(var,1);
			nokia_lcd_set_cursor(35, 10);
			nokia_lcd_write_string("BPM", 1);		
			nokia_lcd_set_cursor(45, 40);
			nokia_lcd_write_string("mmHg",1);
			if(padrao != 0 && posicao == 0)
			{
				nokia_lcd_set_cursor(0, 40);	
				for(int i=1;dados[i] != ':';i++)	
				nokia_lcd_write_char(dados[i],1);
				
			}
			else
			{
				nokia_lcd_set_cursor(0, 40);
				nokia_lcd_write_string("error",1);
				if(count>8)
				{
					posicao = 0;
					for(int i=0;i<9;i++)
					recebido[i] = 198;
					count = 0;
				}			
			}
			nokia_lcd_set_cursor(0, 20);				
			if(tempo_ms <= 200){
				nokia_lcd_write_string("---", 1);		
				nokia_lcd_set_cursor(35, 20);
				nokia_lcd_write_string("Celsius", 1);
				nokia_lcd_set_cursor(0, 30);
				nokia_lcd_write_string("---", 1);
				nokia_lcd_set_cursor(35, 30);
				nokia_lcd_write_string("%SpO2", 1);
				nokia_lcd_render();
			}		
		
			if(tempo_ms%200 == 0){
				int2string(temperatura, var);
				nokia_lcd_set_cursor(0, 20);
				nokia_lcd_write_string(var,1);
				nokia_lcd_set_cursor(35, 20);
				nokia_lcd_write_string("Celsius", 1);
				int2string(saturacao, var);
				nokia_lcd_set_cursor(0, 30);
				nokia_lcd_write_string(var,1);
				nokia_lcd_set_cursor(35, 30);
				nokia_lcd_write_string("%SpO2", 1);
				nokia_lcd_render();	
			}
			break;
			case 1:
				nokia_lcd_write_string("Parametros:", 1);	
				int2string(FreqRespiracao, var);
				nokia_lcd_set_cursor(0,10);
				nokia_lcd_write_string(var,1);
				nokia_lcd_set_cursor(20,10);
				nokia_lcd_write_string("->", 1);
				nokia_lcd_set_cursor(35, 10);
				nokia_lcd_write_string("Resp/Min", 1);
				int2string(PerOxigenio, var);
				nokia_lcd_set_cursor(0,20);
				nokia_lcd_write_string(var,1);				
				nokia_lcd_set_cursor(35, 20);
				nokia_lcd_write_string("%O2", 1);
				nokia_lcd_render();
			break;
			case 2:
				nokia_lcd_write_string("Parametros:", 1);
				int2string(FreqRespiracao, var);
				nokia_lcd_set_cursor(0,10);
				nokia_lcd_write_string(var,1);				
				nokia_lcd_set_cursor(35, 10);
				nokia_lcd_write_string("Resp/Min", 1);
				int2string(PerOxigenio, var);
				nokia_lcd_set_cursor(0,20);
				nokia_lcd_write_string(var,1);
				nokia_lcd_set_cursor(20,20);
				nokia_lcd_write_string("->", 1);
				nokia_lcd_set_cursor(35, 20);
				nokia_lcd_write_string("%O2", 1);
				nokia_lcd_render();	
			break;
			case 3:
				nokia_lcd_write_string("Dados Salvos:", 1);
				nokia_lcd_set_cursor(45, 10);
				nokia_lcd_write_string("mmHg",1);
				nokia_lcd_set_cursor(0,10);
				for(h=1 && z==1;h<=6;h++){
				nokia_lcd_write_char(EP[h],1);
				}
				nokia_lcd_set_cursor(45, 20);
				nokia_lcd_write_string("mmHg",1);
				nokia_lcd_set_cursor(0,20);
				for(h=1 && z==2;h<=6;h++){
				nokia_lcd_write_char(EZ[h],1);
				}
				nokia_lcd_set_cursor(45, 30);
				nokia_lcd_write_string("mmHg",1);
				nokia_lcd_set_cursor(0,30);
				for(h=1 && z==3;h<=6;h++){
				nokia_lcd_write_char(EQ[h],1);
				}
				nokia_lcd_set_cursor(45, 40);
				nokia_lcd_write_string("mmHg",1);
				nokia_lcd_set_cursor(0,40);
				for(h=1 && z==4;h<=6;h++){
				nokia_lcd_write_char(EL[h],1);
				}
				nokia_lcd_render();
				
			
			break;
		}
		}			
}

void respirar(uint8_t num){
		
	if(num <= 8)
		OCR1B = (num*250)+2000;	
	else
		OCR1B = ((17-num)*250)+2000;
	if(OCR1B == 2000)
		PORTD |= 0b10000000;
	else
		PORTD &= 0b01111111;
}

void int2string(unsigned int valor, unsigned char *disp)
{
	for(uint8_t n=0; n<tam_vetor; n++)
	disp[n] = 0 + 48;
	disp += tam_vetor-1;

	do
	{
		*disp = (valor%10) + 48;
		valor /= 10;
		disp--;
	}while(valor!=0);
}

void analisa_vetor()
{	
	padrao = 0;
	switch(recebido[0])
	{
		case ';':
		switch(recebido[1])
		{
			case 'n':
			switch(recebido[2])
			{
				case 'n':
				switch(recebido[3])
				{
					case 'n':
					switch(recebido[4])
					{
						case 'x':
						switch(recebido[5])
						{
							case'n':
							switch(recebido[6])
							{
								case 'n':
								switch(recebido[7])
								{
									case 'n':
									switch(recebido[8])
									{
										case':':
										padrao = 1;									
										posicao = 0;
										z++;
										for(int i=0;i<9;i++)
										recebido[i] = 198;
										break;
									}
									break;
									case ':':
									padrao = 2;
									z++;									
									posicao = 0;
									for(int i=0;i<9;i++)
									recebido[i] = 198;
									break;
								}
								break;
								case ':':
								z++;
								padrao = 9;								
								posicao = 0;
								for(int i=0;i<9;i++)
								recebido[i] = 198;
								break;
							}
							break;
						}
						break;
					}
					break;
					case 'x':
					switch(recebido[4])
					{
						case 'n':
						switch(recebido[5])
						{
							case 'n':
							switch(recebido[6])
							{
								case 'n':
								switch(recebido[7])
								{
									case ':':
									padrao = 3;
									z++;									
									posicao = 0;
									for(int i=0;i<9;i++)
									recebido[i] = 198;
									break;
								}
								break;
								case ':':
								padrao = 4;	
								z++;							
								posicao = 0;
								for(int i=0;i<9;i++)
								recebido[i] = 198;
								break;
							}
							break;
							case ':':
							padrao = 5;				
							z++;			
							posicao = 0;
							for(int i=0;i<9;i++)
							recebido[i] = 198;
							break;
						}
						break;
					}
					break;
				}
				break;
				case 'x':
				switch(recebido[3])
				{
					case 'n':
					switch(recebido[4])
					{
						case 'n':
						switch(recebido[5])
						{
							case 'n':
							switch(recebido[6])
							{
								case ':':
								z++;
								padrao = 6;								
								posicao = 0;
								for(int i=0;i<9;i++)
								recebido[i] = 198;
								break;
							}
							break;
							case ':':
							z++;
							padrao = 7;							
							posicao = 0;
							for(int i=0;i<9;i++)
							recebido[i] = 198;
							break;
						}
						break;
						case ':':
						z++;
						padrao = 8;						
						posicao = 0;
						for(int i=0;i<9;i++)
						recebido[i] = 198;
						break;
					}
					break;
				}
				break;
			}
			break;
		}
		break;
	}			
}




