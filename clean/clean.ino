#define NKOL 1
#define ON 1
#define OFF 0

const int SROUT_SHCP = 12;
const int SROUT_STCP = 13;
const int SROUT_DS = 14;
const int SRIN_PL = 15;
const int SRIN_DS = 16;

uint8_t g_panLED[NKOL];
uint8_t g_panINP[NKOL];
uint8_t g_gamestatus;

void setup()
{
	memset(g_panINP, 0x00, sizeof(g_panINP));
	pinMode(SROUT_SHCP, OUTPUT);
	digitalWrite(SROUT_SHCP, LOW);
	pinMode(SROUT_STCP, OUTPUT);
	digitalWrite(SROUT_STCP, LOW);
	pinMode(SROUT_DS, OUTPUT);
	digitalWrite(SROUT_DS, LOW);
	pinMode(SRIN_PL, OUTPUT);
	digitalWrite(SRIN_PL, LOW);
	pinMode(SRIN_DS, INPUT);
	allLEDsOff();
	g_gamestatus = OFF;
}

// set a single panle light either on or off
// func = 0 -> uit                                                                         *
// func = 1 -> aan                                                                         *
// func = 2 -> toggle
void set_panel_light(uint8_t kolom, uint8_t rij, uint8_t func)
{
	uint16_t pat;
	if (kolom >= NKOL)
	{
		return;
	}
	if (func > 2)
	{
		return;
	}
	pat = 1 << rij;
	if (func == 1)
	{
		g_panLED[kolom] |= pat;
	}
	else if (func == 0)
	{
		pat = ~pat;
		g_panLED[kolom] &= pat;
	}
	else
	{
		g_panLED[kolom] ^= pat;
	}
}

// read from g_panLED and write to the panel
void write_to_panel()
{
	int column;
	int row;
	uint8_t sd_out;
	uint8_t sd_inp;

	for (column = 0; column < NKOL; column++)
	{
		sd_out = g_panLED[column];
		sd_inp = 0;
		for (row = 0; row < 8; row++)
		{
			sd_inp = sd_inp << 1;
			if (digitalRead(SRIN_DS) == LOW)
			{
				sd_inp |= 0x01;
			}
			digitalWrite(SROUT_DS, sd_out & 1);
			digitalWrite(SROUT_SHCP, HIGH);
			digitalWrite(SROUT_SHCP, LOW);
			sd_out = sd_out >> 1;
		}
		g_panINP[column] = sd_inp;
	}
	digitalWrite(SRIN_PL, LOW);
	digitalWrite(SROUT_STCP, HIGH);
	digitalWrite(SROUT_STCP, LOW);
	digitalWrite(SRIN_PL, HIGH);
	if (g_gamestatus == OFF && g_panINP[0] == 1)
		g_gamestatus = ON;
}

void allLEDsOff()
{
	memset(g_panLED, 0, sizeof(g_panLED));
}

void allLEDsOn()
{
	memset(g_panLED, 0xFF, sizeof(g_panLED));
}

// read which buttons are pressed and write into g_panINP
void read_input()
{
	static uint32_t last_read = 0;
	uint32_t now;
	uint8_t kdata;
	bool a_change_happened = false;

	now = millis();
	if (abs(now - last_read) < 700) // only read input every 700 ms
	{
		return;
	}
	for (uint16_t column = 0; column < NKOL; column++)
	{
		kdata = panINP[column];
		for (uint16_t row = 0; row < 8; row++)
		{
			if (kdata & 1)
			{
				a_change_happened = true;
			}
			kdata >>= 1;
		}
	}
	if (a_change_happened)
	{
		last_read = now;
	}
}

void loop()
{
	write_to_panel();
	delay(2);
}
