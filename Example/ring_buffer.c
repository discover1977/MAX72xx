/*
 * ring_buffer.c
 *
 *  Created on: 19 мая 2015 г.
 *      Author: gavrilov.iv
 */

#include "ring_buffer.h"

static unsigned char Buffer[BUFFER_SIZE];
static unsigned char Head = 0;
static unsigned char Tail = 0;
static unsigned char Overflow = 0;
static unsigned char Underflow = 0;

unsigned char is_full()
{
	unsigned char IncTail = Tail + 1;

	if (IncTail == BUFFER_SIZE)
	{
		IncTail=0;
	}
	return (IncTail == Head);
}

unsigned char is_empty()
{
	return ( Head == Tail );
}

void RB_push_char( unsigned char Data  )
{
	if ( is_full() ) Overflow++;
	else
	{
		Buffer[Tail] = Data;
		Tail++;
		if ( Tail == BUFFER_SIZE ) Tail = 0;
	}
}

unsigned char RB_pop_char( unsigned char *Data )
{
	if ( is_empty() )
	{
		Underflow++;
		return 0;
	}
	else
	{
		*Data = Buffer[Head];
		Head++;
		if ( Head == BUFFER_SIZE ) Head = 0;
		return 1;
	}
}

unsigned char get_size()
{
	signed char Size = Tail - Head;
	if ( Size < 0 ) Size += BUFFER_SIZE;
	return (unsigned char)Size;
}

unsigned char RB_peek_char( unsigned char Index, unsigned char *Data )
{
	unsigned char IncHead;
	if ( Index < get_size() )
	{
		IncHead = Index + Head;
		if ( IncHead >= BUFFER_SIZE ) IncHead -= BUFFER_SIZE;
		*Data = Buffer[IncHead];
		return 1;
	}
	else
	{
		Underflow++;
		return 0;
	}
}

unsigned char RB_UnderflowCount()
{
	return Underflow;
}

unsigned char RB_OverflowCount()
{
	return Overflow;
}
unsigned char RB_read_buffer( unsigned char *RxData )
{
	unsigned char b;
	unsigned char Data;
	unsigned char *Pointer;

	b = 1;
	while(b)
	{
		b = RB_peek_char(0, &Data);
		if (!b) return 0;
		if (Data != START_BYTE)
			RB_pop_char(&Data);
		else
			b = 0;
	}
	b = RB_peek_char(RX_PACKET_SIZE - 1, &Data);
	if (!b) return 0;
	if (Data == STOP_BYTE)
	{
		Pointer = RxData;
		for ( b = 0; b < RX_PACKET_SIZE; b++ )
		{
			RB_pop_char( Pointer );
			Pointer++;
			asm("nop");
		}
		asm("nop");
		return 1;
	}
	else
	{
		RB_pop_char(&Data);
		return 0;
	}
}

