/////////////////////////////////////////////////////
//
// ARMEBA - Arduino Mega Basic 
//
/////////////////////////////////////////////////////

//
// Inits serial communication on MEGA serial port
//
void init_Console(){
  Serial.begin(CONSOLE_BAUD);
  //while( !Serial ); // only for Leonardo
  CONSOLE_PrintPROGMEM(ARMEBA_INITIAL_MSG);
  CONSOLE_PrintPROGMEM(ARMEBA_VERSION_MSG);
}

//
// Prints to console only
//
static void CONSOLE_PrintPROGMEM( const unsigned char *msg){
  append_Message_PROGMEM( LCD_Message, msg, true, false);
  Serial.println( LCD_Message);
  *LCD_Message = NULLCHAR;
  LCD_Message_Keep = false;
}

//
// Cyrillic fonts available:
//
// u8g2_font_4x6_t_cyrillic
// u8g2_font_5x7_t_cyrillic
// u8g2_font_5x8_t_cyrillic
// u8g2_font_6x12_t_cyrillic
// u8g2_font_6x13_t_cyrillic
// u8g2_font_6x13B_t_cyrillic
// u8g2_font_7x13_t_cyrillic
// u8g2_font_8x13_t_cyrillic
// u8g2_font_9x15_t_cyrillic
// u8g2_font_10x20_t_cyrillic
//

//
// Inits the LCD display
// Note that a reset must be supplied, or the display is garbled
//
static void init_LCD(){
  for( int i=0; i<LCD_SCREEN_ROWS; i++){
    LCD_Line_Pointers[i] = LCD_Text_Buffer + LCD_TEXT_BUFFER_LINE_LENGTH * i;
    *LCD_Line_Pointers[i] = 0;
  }
  for( int i=0; i<LCD_STACK_ROWS; i++){
    LCD_Stack_Pointers[i] = LCD_Stack_Buffer + LCD_STACK_BUFFER_LINE_LENGTH * i;
    *LCD_Stack_Pointers[i] = 0;
  }
  STACK_Reset();
  u8g2.begin();
  u8g2.enableUTF8Print();
  LCD_initialized = true;
  display_SplashScreen();
}

//
// Redraws the entire screen
//
static void LCD_DrawScreen(){
  if( !LCD_initialized) return;
  u8g2.firstPage();  
  do {
    for( int i=LCD_SCREEN_ROWS-1, j=63; i>=0; i--, j-=8)
      u8g2.drawUTF8(0,j,LCD_Line_Pointers[i]);
  }while ( u8g2.nextPage() );
}

//
// Scrolls screen buffer up one line
//
static void LCD_ScrollUp(){
  for( int i=0; i<LCD_SCREEN_ROWS-1; i++)
    strcpy( LCD_Line_Pointers[i], LCD_Line_Pointers[i+1]);
}

//
// Scrolls screen buffer up one line and prints message
//
static void LCD_PrintString( char *msg){
  LCD_ScrollUp();
  strncpy( LCD_Line_Pointers[LCD_SCREEN_ROWS-1], msg, LCD_TEXT_BUFFER_LINE_LENGTH);
  LCD_DrawScreen();
  Serial.println( LCD_Line_Pointers[LCD_SCREEN_ROWS-1]);
}

//
// Scrolls screen buffer up one line and prints message from progmem
//
static void LCD_PrintPROGMEM( const unsigned char *msg){
  LCD_ScrollUp();
  append_Message_PROGMEM( LCD_Line_Pointers[LCD_SCREEN_ROWS-1], msg, true, false);
  LCD_DrawScreen();
  Serial.println( LCD_Line_Pointers[LCD_SCREEN_ROWS-1]);
}

//
// Returns number of printable positions in the string
// Currently Cyrillic Unicode makers 208 and 209 are supported
//
static size_t count_Unicode( char *dest){
  size_t position_count = 0;
  byte *tmp = (byte *)dest;
  while( *tmp != NULLCHAR){
    byte b = *tmp++;
    if( b == 208 || b == 209) continue;
    position_count++;
  }
  return position_count;
}

//
// Appends a string massage into a memory location;
// Trims the string to prevent screen overflow
// Returns the total number of bytes in the string
//
static size_t append_Message_String( char *dest, char *msg, bool reset, bool unicode_count){
  size_t i = 0;
  size_t position_count = 0;
  if( !reset){
    i=strlen( dest);
    position_count = count_Unicode( dest);
  }
  while( i<LCD_TEXT_BUFFER_LINE_LENGTH-1){
    byte c = (byte)(*msg++);
    dest[i++] = c;
    if( c == NULLCHAR) break;
    if( c == NL){
      dest[i-1] = NULLCHAR;
      break;
    }
    if( c != 208 && c != 209) position_count++;
    if( unicode_count && position_count > LCD_SCREEN_COLUMNS-1){
       dest[i++] = '>';
       dest[i++] = NULLCHAR;
       break;
    }
  }
  dest[LCD_TEXT_BUFFER_LINE_LENGTH-1] = NULLCHAR; // prevents buffer overflow
  return i-1;
}

//
// Appends a PROGMEM massage into a memory location;
// Trims the string to prevent screen overflow
// Returns the total number of bytes in the string
//
static size_t append_Message_PROGMEM( char *dest, const unsigned char *msg, bool reset, bool unicode_count){
  size_t i = 0;
  size_t position_count = 0;
  if( !reset){
    i=strlen( dest);
    position_count = count_Unicode( dest);    
  }
  while( i<LCD_TEXT_BUFFER_LINE_LENGTH-1){
    char c = pgm_read_byte( msg++ );
    dest[i++] = c;
    if( c == NULLCHAR) break;
    if( c != 208 && c != 209) position_count++;
    if( unicode_count && position_count > LCD_SCREEN_COLUMNS-1){
       dest[i++] = '>';
       dest[i++] = NULLCHAR;
       break;
    }
  }
  dest[LCD_TEXT_BUFFER_LINE_LENGTH-1] = NULLCHAR; // prevents buffer overflow
  return i-1;
}

//
// Prints a message from PROGMEM in any screen location
//
static void display_Message_PROGMEM(uint8_t x, uint8_t y, const unsigned char *msg){
  if( !LCD_initialized) return;
  append_Message_PROGMEM( LCD_Message, msg, true, false);
  u8g2.drawUTF8(x,y,LCD_Message);
  *LCD_Message = NULLCHAR;
  LCD_Message_Keep = false;
}

//
// Prints one program line to LCD and console
// Line number is converted to characters
//
static void LCD_PrintProgLine( unsigned char *ptr){
  snprintf( LCD_Message, LCD_TEXT_BUFFER_LINE_LENGTH, "%03u ", Program_Line_Number( ptr));
  append_Message_String( LCD_Message, Program_Line_Body( ptr), false, true);
  LCD_PrintString( LCD_Message);
  *LCD_Message = NULLCHAR;
  LCD_Message_Keep = false;
}

//
// Shows ARMEBA splash screen
//
static void display_SplashScreen(){
  if( !LCD_initialized) return;
  u8g2.firstPage();  
  do {
    u8g2.setFont( u8g2_font_6x12_t_cyrillic);
    display_Message_PROGMEM(  7, 24, ARMEBA_INITIAL_MSG);
    u8g2.setFont( u8g2_font_5x8_t_cyrillic);
    display_Message_PROGMEM( 25, 33, ARMEBA_VERSION_MSG);
  }while ( u8g2.nextPage() );
}

//
// Prints an error message, followed by the offending code line
//
static void LCD_PrintError(const unsigned char *msg){
  LCD_PrintPROGMEM(msg);
  byte tmp = *txtpos;
  *txtpos = NULLCHAR;
  if( current_line == NULL || txtpos >= program_end){
    append_Message_String( LCD_Message, program_end + sizeof(LINE_NUMBER_TYPE), true, true);    
  }
  else{
    snprintf( LCD_Message, LCD_TEXT_BUFFER_LINE_LENGTH, "%03u ", Program_Line_Number( current_line));
    append_Message_String( LCD_Message, Program_Line_Body( current_line), false, true);
  }
  append_Message_String( LCD_Message, "^", false, true); // put marker at offending position
  *txtpos = tmp;
  append_Message_String( LCD_Message, txtpos, false, true); // and the end of offending line
  LCD_PrintString(LCD_Message);
  *LCD_Message = NULLCHAR;
  LCD_Message_Keep = false;
}

//
// prints a string from the txtpos
// if string starts with double quotes, single qoutes are printed
// and opposite way around (Python style)
// sets expression error as needed
//
static void LCD_PrintQuoted(){
  expression_error = false;
  unsigned char delimiter = *txtpos; // initial delimiter
  expression_error = (delimiter != '"') && (delimiter != '\'');
  if( expression_error) return;
  txtpos++;
  
  // corresponding delmiter exists before the end of line?
  int i=0;
  while(txtpos[i] != delimiter){
    if(txtpos[i] != NL && txtpos[i++] != NULLCHAR) continue;
    expression_error = true;
    return;
  }
  
  // print the characters
  size_t j = strlen(LCD_Message);
  size_t k = count_Unicode(LCD_Message);
  while(*txtpos != delimiter){
    if( j<LCD_TEXT_BUFFER_LINE_LENGTH-2){
      LCD_Message[j++] = *txtpos;
      LCD_Message[j] = NULLCHAR;
      if( *txtpos != 208 && *txtpos != 209) k++;
    }
    txtpos++;
    if( k > LCD_SCREEN_COLUMNS-1){
      LCD_Message[j] = NULLCHAR;
      LCD_PrintString( LCD_Message);
      LCD_Message[0] = ' ';
      LCD_Message[1] = NULLCHAR;
      j = 1;
      k = 1;
    }
  }  
  txtpos++; // Skip over the last delimiter
  ignore_Blanks();
}

//
// prints a numberm such as expression value
// TODO: change formats
//
static void LCD_PrintNumber( long a){
  size_t j = strlen(LCD_Message);
  snprintf( LCD_Message+j, LCD_TEXT_BUFFER_LINE_LENGTH-j, "%ld", a);  
  byte k = count_Unicode(LCD_Message);
  if( k > LCD_SCREEN_COLUMNS-1){
    LCD_PrintString( LCD_Message);
    LCD_Message[0] = ' ';
    LCD_Message[1] = NULLCHAR;
  }
}