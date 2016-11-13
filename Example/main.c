/*
 * main.c
 *
 *  Created on: 28 июля 2016 г.
 *      Author: gavrilov.iv
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "bits_macros.h"
#include "MAX72xx.h"
#include "i2c-soft.h"
#include "rtc.h"
#include "ring_buffer.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

volatile struct DateTime
{
	uint8_t Year;
	uint8_t Mons;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
} DateTime;

volatile uint8_t ReadRTC = 0;

enum RxCommand
{
	SetTime,
	SetDate,
	SetRS,
	ShowRS,
	SetIntensity
};

// Структура для обмена данными
struct RXPacket
{
	unsigned char  	StartByte;
	unsigned char	Cmd;
	unsigned char	Data[16];
	unsigned char  	StopByte;
} RXPacket;

volatile uint8_t ReadRingBuffer = 0;			// Флаг для проверки кольцевого буфера

void USART_Init( unsigned int ubrr )
{
	/* Set baud rate */
	UBRR0H = (unsigned char)( ubrr >> 8 );
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (0 << TXCIE0) | (1 << RXCIE0)| (0 << TXEN0) | (1 << RXEN0);
	UCSR0C = (3 << UCSZ00) | (1 << USBS0) | (0 << UMSEL00);
}

ISR(USART_RX_vect)
{
	uint8_t Data = UDR0;
	RB_push_char( Data );	// Кладем принятый байт в кольцевой буфер
}

ISR(TIMER0_OVF_vect)
{
	ReadRingBuffer = 1;
}

ISR(INT0_vect)
{
	ReadRTC = 1;
}

int main()
{
	char Text[16];				// Массив для формирования строки
	char RXText[17];				// Массив для формирования строки
	uint8_t BlinkPoint = 0;		// Флаг для мигания точки в отображении времени
	int Temperature = 0;		// Переменная для хранения температуры

	// External interrupt INT0
	EIMSK = (1 << INT0);
	EICRA = ( 0 << ISC01) | (1 << ISC00);
	// ISC11 ISC10 Description
	// 0 0 The low level of INT1 generates an interrupt request.
	// 0 1 Any logical change on INT1 generates an interrupt request.
	// 1 0 The falling edge of INT1 generates an interrupt request.
	// 1 1 The rising edge of INT1 generates an interrupt request.

	// Timer 0 init
	TCCR0A = 0x00;
	TCCR0B = (1 << CS02) | (0 << CS01) | (1 << CS00);
	TCNT0 = 0x00;
	TIMSK0 = (1 << TOIE0);

	MAX72xx_Init(0);			// Инициализация MAX72xx

	i2c_Init();					// Инициализация программного интерфейса I2C

	/* Инициализация RTC */
#ifdef DS1307
	rtc_init( 1 << CTRL_SQWE );	// DS1307 init data
#endif
#ifdef DS323x
	rtc_init( ( 1 << CTRL_CONV ) | ( 0 << CTRL_EOSC ) | ( 1 << CTRL_BBSQW ) | ( 0 << CTRL_INTCN ) );	// DS323x init data
#endif

	USART_Init(MYUBRR);			// Инициализация USART

	sei();

	/* Демонстрация возможностей вывода библиотеки MAX72xx*/
	_delay_ms(1000);
	MAX72xx_OutSym("  HELLO   7219  ", 16);		// Вывод строки
	_delay_ms(4000);
	for(int c = 0; c < 4; c++) {
		for(int i = 0; i < 16; i++) {
			MAX72xx_SetIntensity(0, i);		// Установка яркости индикатора 0
			MAX72xx_SetIntensity(1, i);		// Установка яркости индикатора 1
			_delay_ms(50);
		}
		_delay_ms(100);
		for(int i = 15; i > -1; i--) {
			MAX72xx_SetIntensity(0, i);
			MAX72xx_SetIntensity(1, i);
			_delay_ms(50);
		}
		_delay_ms(100);
	}
	_delay_ms(4000);

	MAX72xx_SetIntensity(0, 5);
	MAX72xx_SetIntensity(1, 5);
	_delay_ms(4000);

	MAX72xx_Clear(0);				// Очистка индикатора 0
	MAX72xx_Clear(1);				// Очистка индикатора 1
	_delay_ms(1000);

	MAX72xx_OutInt(0, 78456, 0);	// Простой вывод числа на индикатор 0
	MAX72xx_OutInt(1, -12345, 0);	// Простой вывод отрицательного числа на индикатор 1
	_delay_ms(4000);

	MAX72xx_Clear(0);
	MAX72xx_Clear(1);
	_delay_ms(1000);

	MAX72xx_OutInt(0, 123456, 3);	// Простой вывод числа на индикатор 0 с установкой точки в позицию 3
	MAX72xx_OutInt(1, 654321, 3);	// Простой вывод отрицательного числа на индикатор 1 с установкой точки в позицию 3
	_delay_ms(4000);

	MAX72xx_Clear(0);
	MAX72xx_Clear(1);
	_delay_ms(1000);

	MAX72xx_OutIntFormat(123, 1, 3, 0);	// Форматированный вывод числа с позиции 1 по позицию 3
	_delay_ms(4000);

	MAX72xx_OutIntFormat(222, 5, 7, 6);	// Форматированный вывод числа с позиции 5 по позицию 7 с установкой точки
	_delay_ms(4000);

	MAX72xx_OutIntFormat(-111, 9, 12, 0);	// Форматированный вывод отрицательного числа с позиции 9 по позицию 12
	_delay_ms(4000);

	MAX72xx_OutIntFormat(-333, 13, 16, 14);	// Форматированный вывод отрицательного числа с позиции 13 по позицию 16 с установкой точки
	_delay_ms(4000);

	MAX72xx_Clear(0);
	MAX72xx_Clear(1);
	_delay_ms(1000);

	MAX72xx_OutIntFormat(1234, 3, 6, 0);
	_delay_ms(4000);

	MAX72xx_SetCommaMask(0, 40);
	_delay_ms(4000);

	MAX72xx_OutIntFormat(8746, 11, 14, 0);
	_delay_ms(4000);

	MAX72xx_SetComma(12, 1);
	_delay_ms(4000);

	MAX72xx_SetComma(12, 0);
	_delay_ms(4000);

	MAX72xx_Clear(0);
	MAX72xx_Clear(1);
	_delay_ms(1000);
	/******************************************************/

	while(1) {
		if (ReadRTC == 1) {		// Проверяем флаг чтения данных с RTC

			ReadRTC = 0;
			/* Чтение времени, даты, температуры */
			rtc_get_time( (uint8_t*)&DateTime.Hour, (uint8_t*)&DateTime.Minute, (uint8_t*)&DateTime.Second);
			rtc_get_date( (uint8_t*)&DateTime.Day, (uint8_t*)&DateTime.Mons, (uint8_t*)&DateTime.Year);
			#ifdef DS323x
				Temperature = rtc_get_temperature();
			#endif
#if SYMBOL_INCLUDE
			/* Вывод на индикатор */
			sprintf(Text, "%02u%02u %2u*", DateTime.Hour, DateTime.Minute, Temperature);
			MAX72xx_OutSym(Text, 16);
			MAX72xx_SetComma(15, (BlinkPoint)?(15):(0));

			sprintf(Text, "%02u%02u", DateTime.Day, DateTime.Mons);
			MAX72xx_OutSym(Text, 8);
			MAX72xx_SetCommaMask(0, ((1 << 6) | (1 << 4)));
			MAX72xx_OutIntFormat(DateTime.Year + 2000, 1, 4, 0);
			/******************************************************/
#else
			/* Вывод на индикатор */
			MAX72xx_OutIntFormat(DateTime.Hour, 15, 16, (BlinkPoint)?(15):(0));
			MAX72xx_OutIntFormat(DateTime.Minute, 13, 14, 0);

			MAX72xx_OutIntFormat(Temperature, 9, 11, 0);

			MAX72xx_OutIntFormat(DateTime.Year + 2000, 1, 4, 0);
			MAX72xx_OutIntFormat(DateTime.Mons, 5, 6, 5);
			MAX72xx_OutIntFormat(DateTime.Day, 7, 8, 7);
#endif
			if(++BlinkPoint > 1) BlinkPoint = 0;
		}
		/******************************************************/

		if (ReadRingBuffer == 1) {		// Проверяем флаг чтения кольцевого буфера

			ReadRingBuffer = 0;

			if ( RB_read_buffer((uint8_t *)&RXPacket) == 1 ) {		// Проверяем кольцевой буфер
				/* Разбор команд */
				switch(RXPacket.Cmd) {
					case SetTime: {
						rtc_set_time(RXPacket.Data[0], RXPacket.Data[1], 0 );
					}; break;
					case SetDate: {
						rtc_set_date(RXPacket.Data[0], RXPacket.Data[1], RXPacket.Data[2] );
					}; break;
					case SetRS: {
						memcpy(RXText, (uint8_t*)&(RXPacket.Data), 16);
						RXText[16] = 0x00;
					}; break;
					case ShowRS: {
						MAX72xx_OutSym(RXText, 16);
						for (int c = 0; c < RXPacket.Data[0]; c++) {
							_delay_ms(1000);
						}
					}; break;
					case SetIntensity: {
						MAX72xx_Send(RXPacket.Data[0], INTENSITY, RXPacket.Data[1]);
					}; break;
					default: break;
				}
				/**************************************************************************/
			}
		}
	}
	return 0;
}
