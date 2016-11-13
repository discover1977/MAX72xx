/*
 * ring_buffer.h
 *
 *  Created on: 19 ��� 2015 �.
 *      Author: gavrilov.iv
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#define RX_PACKET_SIZE		19	// �������� ������ ���������� ������
#define BUFFER_SIZE			40	// ������ ���� � ��� ���� ������ RX_PACKET_SIZE

#define START_BYTE			's'
#define STOP_BYTE			'e'

void RB_push_char( unsigned char Data  );
unsigned char RB_pop_char( unsigned char *Data );
unsigned char RB_peek_char( unsigned char Index, unsigned char *Data );
unsigned char RB_get_size();
unsigned char RB_UnderflowCount();
unsigned char RB_OverflowCount();
unsigned char RB_read_buffer( unsigned char *RxData );

#endif /* RING_BUFFER_H_ */
