#define NKOL 1
#define SLEEP_DELAY 60000

// io pins
#define SROUT_SHCP 12
#define SROUT_STCP 13
#define SROUT_DS 14
#define SRIN_PL 15
#define SRIN_DS 16

uint8_t g_panLED[NKOL];
uint8_t g_panINP[NKOL];
uint8_t g_game_is_running;
uint8_t g_last_input_time;

void setup()
{
	Serial.begin(115200);
	Serial.println("\nGestart....");
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
	g_game_is_running = false;
	g_last_input_time = 0;
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
void sync_board()
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
				Serial.print(row);
				g_last_input_time = millis();
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
	uint16_t row, column;

	t1 = millis();			// Haal huidige tijd
	if (abs(t1 - t0) < 200) // In wachttijd?
		return 1;
	// Serial.print(t0);
	head++;
	if (head > 7)
		head = 0;
	row = head;
	for (column = 0; column < NKOL; column++)
	{
		set_panel_light(column, row, 1); // Set set_panel_lightl
		if (head > 3)
			set_panel_light(column, row - 4, 0);
		if (head < 4)
			set_panel_light(column, 7 - 3 + row, 0);
		row++;
	}
	t0 = t1;
}

void check_activity()
{
	uint32_t now = millis();

	if (now - g_last_input_time > SLEEP_DELAY)
	{
		g_game_is_running = true;
		for (int i = 0; i < 8; i++)
			set_panel_light(0, i, 0);
		Serial.print("turn game on");
	}
	else
		g_game_is_running = false;
}

void loop()
{
	if (!g_game_is_running)
		snooze_patern();
	sync_board();
	check_activity();
	delay(2);
}
