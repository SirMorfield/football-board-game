#define NKOL 1
#define ON 1
#define OFF 0

const int SHIFT_REGISTER_CLOCK_INPUT = 12;	// HC595 LED shift register serial clock (high is shift), pin 11 van HC595 SWItCh
const int SERIAL_REGISTER_CLOCK_INPUT = 13; // HC595 LED storage register clock (high is shift), pin 12 van HC595 SWITCH
const int SERIAL_DATA_INPUT = 14;			// HC595 LED serial data, pin 14 on HC595
const int PARALLEL_LOAD_INPUT = 15;			// HC597 SWITCH parallel load (low) of seriële shift, pin 13 van HC597
const int SERIAL_DATA_INPUT = 16;			// HC587 serial data out, pin 9 van HC597 SWITCH

uint8_t g_panLED[NKOL];
uint8_t g_panINP[NKOL];
uint8_t g_gamestatus;
uint8_t g_activity;

void setup()
{
	memset(g_panINP, 0x00, sizeof(g_panINP));
	pinMode(SHIFT_REGISTER_CLOCK_INPUT, OUTPUT);
	digitalWrite(SHIFT_REGISTER_CLOCK_INPUT, LOW);
	pinMode(SERIAL_REGISTER_CLOCK_INPUT, OUTPUT);
	digitalWrite(SERIAL_REGISTER_CLOCK_INPUT, LOW);
	pinMode(SERIAL_DATA_INPUT, OUTPUT);
	digitalWrite(SERIAL_DATA_INPUT, LOW);
	pinMode(PARALLEL_LOAD_INPUT, OUTPUT);
	digitalWrite(PARALLEL_LOAD_INPUT, LOW);
	pinMode(SERIAL_DATA_INPUT, INPUT);
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

// sync shift registers with local buffers
void sync_board()
{
	uint8_t sd_out;
	uint8_t sd_inp;

	for (uint32_t column = 0; column < NKOL; column++)
	{
		sd_out = g_panLED[column];
		sd_inp = 0;
		for (uint32_t row = 0; row < 8; row++)
		{
			sd_inp = sd_inp << 1;
			if (digitalRead(SERIAL_DATA_INPUT) == LOW)
			{
				g_activity = millis();
				sd_inp |= 0x01;
			}
			digitalWrite(SERIAL_DATA_INPUT, sd_out & 1);
			digitalWrite(SHIFT_REGISTER_CLOCK_INPUT, HIGH);
			digitalWrite(SHIFT_REGISTER_CLOCK_INPUT, LOW);
			sd_out = sd_out >> 1;
		}
		g_panINP[column] = sd_inp;
	}
	digitalWrite(PARALLEL_LOAD_INPUT, LOW);
	digitalWrite(SERIAL_REGISTER_CLOCK_INPUT, HIGH);
	digitalWrite(SERIAL_REGISTER_CLOCK_INPUT, LOW);
	digitalWrite(PARALLEL_LOAD_INPUT, HIGH);
	if (g_gamestatus == OFF && (g_panINP[0] & (1 << 0)))
	{
		g_gamestatus = ON;
		for (uint32_t i = 0; i < 8; i++)
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

	rij = random(8);		// Kies random positie
	t1 = millis();			// Haal huidige tijd
	if (abs(t1 - t0) < 200) // In wachttijd?
		return 1;
	head++;
	if (head > 7)
		head = 0;
	rij = head;
	for (kol = 0; kol < NKOL; kol++)
	{
		set_panel_light(kol, rij, 1); // Set set_panel_lightl --> fills data inside shifters
		if (head > 3)
			set_panel_light(kol, rij - 4, 0);
		if (head < 4)
			set_panel_light(kol, 7 - 3 + rij, 0);
		rij++;
	}
	sync_board(); // Toon patroon --> writes to the leds
	t0 = t1;
}

void check_activity()
{
	uint32_t t = millis();
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
	sync_board();
	check_activity();
	delay(2);
}
