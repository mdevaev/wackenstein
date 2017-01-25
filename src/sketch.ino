#include <Arduino.h>
#include <Rotary.h>

// http://techdocs.altium.com/display/FPGA/PS2+Keyboard+Scan+Codes

const int KBD_CLOCK_PIN = A1;
const int KBD_DATA_PIN = A2;

const unsigned char K_PFX = 0xE0;
const unsigned char K_SEP = 0xFF;
const unsigned char K_REL = 0xF0;
const unsigned char K_END = 0;

const unsigned char SEQ_ESCAPE[] = {0x76,  K_END};
const unsigned char SEQ_ENTER[]  = {0x5A,  K_END};
const unsigned char SEQ_DELETE[] = {K_PFX, 0x71,  K_END};
const unsigned char SEQ_CTRL_Z[] = {0x14,  K_SEP, 0x1A, K_END};
const unsigned char SEQ_CTRL_S[] = {0x14,  K_SEP, 0x1B, K_END};
const unsigned char SEQ_UP[]     = {K_PFX, 0x75,  K_END};
const unsigned char SEQ_DOWN[]   = {0x72,  K_END};
const unsigned char SEQ_LEFT[]   = {K_PFX, 0x6b,  K_END};
const unsigned char SEQ_RIGHT[]  = {0x74,  K_END};
const unsigned char SEQ_CTRL[]   = {K_PFX, 0x14,  K_END};
const unsigned char SEQ_SHIFT[]  = {0x59,  K_END};
const unsigned char SEQ_ALT[]    = {K_PFX, 0x11};

const unsigned char CLICK_PG_UP[]   = {K_PFX, 0x7D, K_PFX, K_REL, 0x7D, K_END};
const unsigned char CLICK_PG_DOWN[] = {K_PFX, 0x7A, K_PFX, K_REL, 0x7A, K_END};

const int ENC_A_PIN = A5;
const int ENC_B_PIN = A4;

const int BT_1_PIN = A7;
const int BT_2_PIN = A6;
const int BT_3_PIN = A3;
const int BT_4_PIN = 2;

const int BT_UP_PIN = 3;
const int BT_DOWN_PIN = 5;
const int BT_LEFT_PIN = 6;
const int BT_RIGHT_PIN = 4;

const int BT_5_PIN = 10;
const int BT_6_PIN = 9;
const int BT_7_PIN = 8;
const int BT_8_PIN = 7;
const int BT_9_PIN = 11;
const int BT_10_PIN = 12;


void kbd_send_bit(bool b) {
	digitalWrite(KBD_CLOCK_PIN, HIGH);
	digitalWrite(KBD_DATA_PIN, b);
	digitalWrite(KBD_CLOCK_PIN, LOW);
	digitalWrite(KBD_CLOCK_PIN, HIGH);
}

void kbd_send_byte(unsigned char ch) {
	int b_sum = 0;
	kbd_send_bit(0);
	for (int count = 0; count < 8; ++count) {
		unsigned char b = (ch >> count) & 0b00000001;
		b_sum += b;
		kbd_send_bit(b);
	}
	kbd_send_bit(!(b_sum % 2));
	kbd_send_bit(1);
}

void kbd_send_bytes(const unsigned char *seq) {
	for (; *seq; ++seq) {
		kbd_send_byte(*seq);
	}
}

void kbd_click(int pin, const unsigned char *seq, int *state) {
	int new_state = read_button(pin);

	if (new_state && !*state) { // Press
		for (; *seq; ++seq) {
			kbd_send_byte(*seq);
		}
		*state = new_state;

	} else if (!new_state && *state) { // Release
		int start = 0;
		for (; *(seq + start); ++start);
		while (start >= 0) {
			if (start == 0 || *(seq + start) == K_SEP) {
				int from = start + (start != 0 ? 1 : 0);
				for (; *(seq + from) != K_SEP && *(seq + from) != K_END; ++from) {
					if (*(seq + from) != K_PFX) {
						kbd_send_byte(K_REL);
					}
					kbd_send_byte(*(seq + from));
				}
			}
			--start;
		}
		*state = new_state;
	}
}

int read_button(int pin) {
	return (pin == A6 || pin == A7 ? analogRead(pin) > 100 : digitalRead(pin));
}

Rotary encoder = Rotary(ENC_A_PIN, ENC_B_PIN);

ISR(PCINT1_vect) {
	unsigned char result = encoder.process();
	if (result == DIR_CW) {
		kbd_send_bytes(CLICK_PG_UP);
	} else if (result == DIR_CCW) {
		kbd_send_bytes(CLICK_PG_DOWN);
	}
}

void setup() {
	pinMode(KBD_CLOCK_PIN, OUTPUT);
	pinMode(KBD_DATA_PIN, OUTPUT);
	digitalWrite(KBD_CLOCK_PIN, HIGH);
	digitalWrite(KBD_DATA_PIN, HIGH);

	digitalWrite(13, HIGH); // Ready

	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT12) | (1 << PCINT13);
}

#define _MAP_BUTTON(_PIN, _SEQ)  static int _PIN ## _state = read_button(_PIN); kbd_click(_PIN, _SEQ, &_PIN ## _state)

void loop() {
	_MAP_BUTTON(BT_1_PIN, SEQ_ESCAPE);
	_MAP_BUTTON(BT_2_PIN, SEQ_ENTER);
	_MAP_BUTTON(BT_3_PIN, SEQ_DELETE);
	_MAP_BUTTON(BT_4_PIN, SEQ_CTRL_Z);

	_MAP_BUTTON(BT_UP_PIN, SEQ_UP);
	_MAP_BUTTON(BT_DOWN_PIN, SEQ_DOWN);
	_MAP_BUTTON(BT_LEFT_PIN, SEQ_LEFT);
	_MAP_BUTTON(BT_RIGHT_PIN, SEQ_RIGHT);

	_MAP_BUTTON(BT_5_PIN, SEQ_CTRL);
	_MAP_BUTTON(BT_6_PIN, SEQ_SHIFT);
	_MAP_BUTTON(BT_7_PIN, SEQ_ALT);
	_MAP_BUTTON(BT_10_PIN, SEQ_CTRL_S);
}
