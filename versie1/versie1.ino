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
uint8_t g_activity;

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
	g_activity = 0;
}

// set a single panle light either on or off
// func = 0 -> uit                                                                         *
// func = 1 -> aan                                                                         *
// func = 2 -> toggle
void set_panel_light(uint8_t kolom, uint8_t rij, uint8_t func)
{
	uint16_t pat;
	if (kolom >= NKOL)
		return;
	if (func > 2)
		return;
	pat = 1 << rij;
	if (func == 1)
		g_panLED[kolom] |= pat;
	else if (func == 0)
	{
		pat = ~pat;
		g_panLED[kolom] &= pat;
	}
	else
		g_panLED[kolom] ^= pat;
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
				g_activity = millis();
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
	if (g_gamestatus == OFF && (g_panINP[0] & (1 << 0)))
	{
		g_gamestatus = ON;
		for (int i = 0; i < 8; i++)
			set_panel_light(0, i, 0);
	}
}

void allLEDsOff()
{
	memset(g_panLED, 0, sizeof(g_panLED));
}

void allLEDsOn()
{
	memset(g_panLED, 0xFF, sizeof(g_panLED));
}

bool snooze_patern()
{
	static uint32_t t0 = 0;
	static uint16_t head = -1;
	uint32_t t1;
	uint16_t rij, kol;

	rij = random(8); // Kies random positie
	t1 = millis();			// Haal huidige tijd
	if (abs(t1 - t0) < 200) // In wachttijd?
		return 1;
	head++;
	if (head > 7)
		head = 0;
	rij = head;
	for (kol = 0; kol < NKOL; kol++)
	{
		set_panel_light(kol, rij, 1); // Set set_panel_lightl
		if (head > 3)
			set_panel_light(kol, rij - 4, 0);
		if (head < 4)
			set_panel_light(kol, 7 - 3 + rij, 0);
		rij++;
	}
	write_to_panel(); // Toon patroon
	t0 = t1;
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
		return;
	for (uint16_t column = 0; column < NKOL; column++)
	{
		kdata = g_panINP[column];
		for (uint16_t row = 0; row < 8; row++)
		{
			if (kdata & 1)
				a_change_happened = true;
			kdata >>= 1;
		}
	}
	if (a_change_happened)
		last_read = now;
}

void check_activity()
{
	int	t;

	t = millis();
	if (abs(t - g_activity) > 60000)
	{
		g_gamestatus = OFF;
		g_activity = t;
	}
}

void loop()
{
	if (g_gamestatus == OFF)
		snooze_patern();
	read_input();
	write_to_panel();
	check_activity();
	delay(2);
}
