//******************************************************************************************
// vbs-3.ino                                                                               *
//******************************************************************************************
//
// Aansturing LEDs voor voetbalspel paneel.  Versie 3.
// Het paneel bestaat uit 128 panelen, 16 rijen van 8 kolommen.
// Met behulp van schuifregisters worden de LEDs individueel aangestuurd.
// De software is ontworpen voor een STM32F103, maar in principe kan iedere
//
// De schuifregisters zijn van het type 74HC595 voor de output en van het type 74HC597 voor de input.
  
//03-04-2018, Ed Smallenburg
//
#define NKOL 1                                      // Eerste versie heeft maar 1 kolom
// Output pins:
const int SROUT_SHCP = 12 ;   // HC595 LED shift register serial clock (high is shift), pin 11 van HC595 SWItCh
const int SROUT_STCP = 13 ;   // HC595 LED storage register clock (high is shift), pin 12 van HC595 SWITCH
const int SROUT_DS   = 14 ;   // HC595 LED serial data, pin 14 on HC595
const int SRIN_PL    = 15 ;   // HC597 SWITCH parallel load (low) of seriële shift, pin 13 van HC597

// Input pins:
const int SRIN_DS    =  16;    // HC587 serial data out, pin 9 van HC597 SWITCH

// De status van de LEDs van alle 128 panelen.
uint8_t panLED[NKOL] ;                              // Kolommen van 8 bits output
uint8_t panINP[NKOL] ;                              // Kolommen van 8 bits input
char    cmd[20] ;                                   // Buffer voor commando via serial


//******************************************************************************************
//                                  S E T U P                                              *
//******************************************************************************************
void setup()
{
  int i ;                                            // Teller voor 8 rijen

 
  Serial.begin ( 115200 ) ;
  Serial.println() ;
  Serial.println ( "Gestart...." ) ;
  memset ( panINP, 0x00, sizeof(panINP) ) ;          // Clear input
  // Initialize OUTPUT pins.
  pinMode ( SROUT_SHCP, OUTPUT ) ;                   // Shift clockpuls
  digitalWrite ( SROUT_SHCP, LOW ) ;
  pinMode ( SROUT_STCP, OUTPUT ) ;                   // Storage clockpuls
  digitalWrite ( SROUT_STCP, LOW ) ;
  pinMode ( SROUT_DS, OUTPUT ) ;                     // Serial data in
  digitalWrite ( SROUT_DS, LOW ) ;
  pinMode ( SRIN_PL, OUTPUT ) ;                      // PL/serial
  digitalWrite ( SRIN_PL, LOW ) ;
  pinMode ( SRIN_DS, INPUT ) ;                       // Serial in van 597
  clearall() ;                                       // Alle rijen uit
  //memset ( panLED, 0x5555, sizeof(panLED) ) ;      // Test
}


//******************************************************************************************
//                                 P A N E E L                                             *
//******************************************************************************************
// Zet een paneel aan of uit of toggle.  Rij is 0..8, kolom is 0..15.                      *
// func = 0 -> uit                                                                         *
// func = 1 -> aan                                                                         *
// func = 2 -> toggle                                                                      *
//******************************************************************************************
void paneel ( uint8_t kolom, uint8_t rij, uint8_t func )
{
  uint16_t pat ;                                  // Bit op de gewenste positie

  if ( kolom >= NKOL )                            // Test grens voor kolom
  {
    return ;                                      // Kolom niet beschikbaar
  }
  if ( func > 2 )                                 // Test functie
  {
    return ;                                      // Illegal function
  }
  pat = 1 << rij ;                                // Zet bit op de juiste positie
  if ( func == 1 )                                // Moeten de LEDs in het paneel aan?
  {
    panLED[kolom] |= pat ;                        // Ja, OR functie voor dat bit
  }
  else if ( func == 0 )                           // Nee, moeten de LEDs uit?
  {
    pat = ~pat ;                                  // Nee, maak een patroon voor AND
    panLED[kolom] &= pat ;                        // AND functie voor dat bit
  }
  else                                            // Anders toggle (func=2)
  {
    panLED[kolom] ^= pat ;                        // Toggle
  }
}


//******************************************************************************************
//                                     S E T D A T A                                       *
//******************************************************************************************
// Schuif NKOL * 8 bits in het schuifregister.                                             *
//******************************************************************************************
void setdata()
{
  int      i ;                                       // Teller aantal kolommen
  int      j ;                                       // Teller aantal rijen
  uint8_t sd_out ;                                   // Serial data out voor 1 kolom
  uint8_t sd_inp ;                                   // Serial data in voor 1 kolom
  
  for ( i = 0 ; i < NKOL ; i++ )                     // Doe alle kolommen
  {
    sd_out = panLED [i] ;
    sd_inp = 0 ;                                     // Clear alle inputbits deze kolom
    for ( j = 0 ; j < 8 ; j++ )                      // Alle rijen
    {
      sd_inp = sd_inp << 1 ;                         // Schuif 1 positie
      if ( digitalRead ( SRIN_DS ) == LOW )          // Lees contact in deze kolom
      {
        sd_inp |= 0x01 ;                             // Contact bekrachtigd, set bit
      }
      digitalWrite ( SROUT_DS, sd_out & 1 ) ;        // Schuif LS bit er in
      digitalWrite ( SROUT_SHCP, HIGH ) ;            // Clockpuls hoog
      digitalWrite ( SROUT_SHCP, LOW ) ;             // Clockpuls laag
      sd_out = sd_out >> 1 ;                         // Schuif data 1 bit
    }
    panINP[i] = sd_inp ;                             // Vul de kolom met inputs
  }
  digitalWrite ( SRIN_PL, LOW ) ;                    // Voor 597 parallel load doen 
  digitalWrite ( SROUT_STCP, HIGH ) ;                // Storage clockpuls (latch) hoog
  digitalWrite ( SROUT_STCP, LOW ) ;                 // Storage clockpuls laag
  digitalWrite ( SRIN_PL, HIGH ) ;                   // 597 gaat weer schuiven
}


//******************************************************************************************
//                                C L E A R A L L                                          *
//******************************************************************************************
// Clear alle LEDS.                                                                        *
//******************************************************************************************
void clearall()
{
  memset ( panLED, 0, sizeof(panLED) ) ;                  // Clear data, 8 * NKOL bits
}


//******************************************************************************************
//                                S E T A L L                                              *
//******************************************************************************************
// Zet alle LEDS aan.                                                                      *
//******************************************************************************************
void setall()
{
  memset ( panLED, 0xFF, sizeof(panLED) ) ;               // Set data, 8 * NKOL bits
}


//******************************************************************************************
//                            P A R S E _ C O M A N D O                                    *
//******************************************************************************************
// Behandel het commando in cmd                                                            *
// Voorbeelden:                                                                            *
// commando   Betekenenis                                                                  *
// --------   ------------------------                                                     *
// "uit"      Alle LEDs uit                                                                *
// "aan"      Alle LEDs aan                                                                *
// "0,12,0"   Kolom 0, rij 12 uit                                                          *
// "3,11,1"   Kolom 3, rij 11 aan                                                          *
// "test1"    Testpatroon 1                                                                *
// "test2"    Testpatroon, complementair aan testpatroon 1                                *
// "rand"     Random pattroon                                                              *
// "pong"     Pong patroon                                                                 *
// "io"       Toon de input en output voor debug                                                     *
//                                                                                         *
//******************************************************************************************
bool parse_commando()
{
  int8_t  rij, kol ;                              // Rij en kolom ion de matrix
  bool    rrij, rkol ;                            // Richting voor pongpatroon
  int     i ;                                     // Teller voor random patroon
  char*   p = cmd ;                               // Pointer in commando string
  int     par1, par2, par3 ;                      // Parameters gevonden
  uint8_t onoff = 1 ;                             // Voor testpatroon

  if ( strstr ( cmd, "uit" ) )                    // Commando "uit"?
  {
    clearall() ;                                  // Ja, alles uit
  }
  else if ( strstr ( cmd, "aan" ) )               // Commando "aan"?
  {
    setall() ;                                    // Alles aan
  }

  else if ( strstr ( cmd, "test" ) )              // Commando "test"?
  {
    onoff = cmd[4] & 1 ;                          // Bepaal testpatroon
    for ( rij = 0 ; rij < 8 ; rij++ )
    {
      for ( kol = 0 ; kol < NKOL ; kol++ )
      {
        paneel ( kol, rij, onoff ) ;              // Set of clear paneel
        onoff = 2 - onoff ;                       // Wissel patroon
      }
    }
  }
  else if ( strstr ( cmd, "rand" ) )              // Commando "rand"?
  {
    for ( i = 0 ; i < 200 ; i++ )                 // Aantal handelingen
    {
      rij = random ( 8 ) ;                        // Kies random positie
      kol = random ( NKOL ) ;
      kol = 0 ;                                   // We hebben maar 1 kolom
      onoff = ( random ( 2 ) & 1 ) ;              // Random aan of uit
      paneel ( kol, rij, onoff ) ;                // Set of clear paneel
      setdata() ;                                 // Toon patroon
      delay ( 100 ) ;
    }
    clearall() ;                                  // Display weer leeg
  }
  else if ( strstr ( cmd, "pong" ) )              // Commando "pong"?
  {
    rrij = true ;                                 // Richting naar rechtsonder
    rkol = true ;
    rij = random ( 8 ) ;                          // Kies random positie
    kol = random ( NKOL ) ;
    for ( i = 0 ; i < 200 ; i++ )                 // Aantal handelingen
    {
      paneel ( kol, rij, 0 ) ;                    // Clear paneel
      if ( rrij )                                 // Naar beneden?
      {
        rij++ ;                                   // Ja, volgende rij
        if ( rij == 8 )                           // Over de rand?
        {
          rrij = false ;                          // Ja, terug
          rij-- ;
        }
      }
      else
      {
        rij-- ;                                   // Naar boven
        if ( rij < 0 )                            // Over de rand?
        {
          rrij = true ;                           // Ja, terug
          rij++ ;
        }
      }
      if ( random ( 5 ) != 0 )                    // Af en toe wat variatie
      {
        if ( rkol )                               // Naar rechts?
        {
          kol++ ;                                 // Ja, volgende kolom
          if ( kol == NKOL )                      // Over de rand?
          {
            rkol = false ;                        // Ja, terug
            kol-- ;
          }
        }
        else
        {
          kol-- ;                                 // Naar links
          if ( kol < 0 )                          // Over de rand?
          {
            rkol = true ;                         // Ja, terug
            kol++ ;
          }
        }
      }
      paneel ( kol, rij, 1 ) ;                    // Set paneel
      setdata() ;                                 // Toon patroon
      delay ( 100 ) ;
    }
    clearall() ;
  }
  else if ( strstr ( cmd, "io" ) )                // Commando "io"?
  {
    for ( i = 0 ; i < NKOL ; i++ )
    {
      Serial.print ( "Input kolom  " ) ;
      Serial.print ( i ) ;
      Serial.print ( " is " ) ;
      Serial.println ( panINP[i], HEX ) ;         // Toon huidige stand
      Serial.print ( "Output kolom " ) ;
      Serial.print ( i ) ;
      Serial.print ( " is " ) ;
      Serial.println ( panLED[i], HEX ) ;
    }
  }
  else
  {
    par1 = atoi ( cmd ) ;                         // Pak parameter 1
    while ( *p )                                  // zoek naar komma
    {
      if ( *p++ == ',' )                          // Komma gevonden?
      {
        break ; 
      }
    }
    if ( !*p != '\0' )                            // Commando goed?
    {
      return false ;                              // Nee, stop ermee
    }
    par2 = atoi ( p ) ;                           // Pak 2e parameter
    while ( *p )                                  // zoek naar komma
    {
      if ( *p++ == ',' )                          // Komma gevonden?
      {
        break ; 
      }
    }
    if ( !*p != '\0' )                            // Commando goed?
    {
      return false ;                              // Nee, stop ermee
    }
    par3 = atoi ( p ) ;                           // Pak 3e parameter
    paneel ( par1, par2, par3 ) ;                 // Schakel LED
  }
  return true ;
}


//******************************************************************************************
//                              S C A N S E R I A L                                        *
//******************************************************************************************
// Leest een commando van de serial input.                                                 *
//******************************************************************************************

bool scanserial()
{
  static int inx = 0 ;                           // Command from Serial input
  char       c ;                                 // Input character
  
  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( inx )
      {
        cmd[inx] = '\0' ;                        // Delimeter
        inx = 0 ;                                // For next command
        return true ;                            // Complete command seen
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      cmd[inx++] = c ;                           // Add to the command
    }
    if ( inx > ( sizeof(cmd) - 2 ) )             // Check for excessive length
    {
      inx = 0 ;                                  // Too long, reset
    }
  }
  return false ;                                 // Command not completed yet
}


//******************************************************************************************
//                                    H A N D L E _ I N P U T                              *
//******************************************************************************************
// Reageer op de input.  Als er wat gebeurt is doen we daarna 1 seconde niets.             *
//******************************************************************************************
void handle_input()
{
  static uint32_t t0 = 0 ;                        // Tijdstip laatste gebeurtenis
  uint32_t        t1 ;                            // Huidig tijdstip
  uint16_t        kol, rij ;                      // Rijen en kolommen teller
  uint8_t         kdata ;                         // 8 bits in een kolom
  bool            active = false ;                // Test voor iets gedaan
  
  t1 = millis() ;                                 // Haal huidige tijd
  if ( abs ( t1 - t0 ) < 1000 )                   // In wachttijd?
  {
    return ;                                      // Ja, gauw klaar
  }
  for ( kol = 0 ; kol < NKOL ; kol++ )            // Bekijk alle kolommen
  {
    kdata = panINP[kol] ;                         // Pak input, 8 bits in een kolom
    for ( rij = 0 ; rij < 8 ; rij++ )             // Bekijk alle rijen
    {
      if ( kdata & 1 )                            // Input swich ON?
      {
        Serial.print ( "Input kolom " ) ;
        Serial.print ( kol ) ;
        Serial.print ( ", rij " ) ; 
        Serial.println ( rij ) ; 
        paneel ( kol, rij, 2 ) ;                  // Ja, toggle LED
        active = true ;                           // Actie gezien 
      }
      kdata >>= 1 ;                               // Schuif naar volgend bit 
    }
  }
  if ( active )                                   // Enige activitei gezien?
  {
    t0 = t1 ;                                     // Ja, set timer
  }
} 


//******************************************************************************************
//                                      L O O P                                            *
//******************************************************************************************
void loop()
{
  setdata() ;                                    // Refresh en scan alle panelen
  handle_input() ;                               // Reageer op inputs 
  if ( scanserial() )                            // Commando via Serial input?
  {
    Serial.print ( "Command is " ) ;
    Serial.println ( cmd ) ;
    if ( parse_commando() )                      // Ja, ontrafel
    {
       Serial.println ( "OK" ) ;
    }
    else
    {
      Serial.println ( "Fout in commando!" ) ;
    }
  }
  delay ( 2 ) ;
}
