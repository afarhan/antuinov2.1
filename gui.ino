int x1, y1, w, h, x2, y2;
#define X_OFFSET 18


#define MENU_CHANGE_MHZ 0
#define MENU_CHANGE_KHZ 1
#define MENU_CHANGE_HZ  2
#define MENU_MODE_SWR 3
#define MENU_MODE_PWR 4
#define MENU_MODE_SNA 5
#define MENU_SPAN 6
#define MENU_PLOT 7

#define ACTION_SELECT 1
#define ACTION_DESELECT 2
#define ACTION_UP 3
#define ACTION_DOWN 4

int uiFocus = MENU_CHANGE_MHZ, knob=0, uiSelected = -1;


//returns true if the button is pressed
int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

byte enc_state (void) {
    return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2: 0);
}

int enc_read(void) {
  int result = 0; 
  byte newState;
  int enc_speed = 0;
  
  long stop_by = millis() + 100;
  
  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state  
    
    if (newState != enc_prev_state)
      delay (1);
    
    if (enc_state() != newState || newState == enc_prev_state)
      continue; 
    //these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) || 
      (enc_prev_state == 2 && newState == 3) || 
      (enc_prev_state == 3 && newState == 1) || 
      (enc_prev_state == 1 && newState == 0)){
        result--;
      }
    //these transitions point o the enccoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) || 
      (enc_prev_state == 1 && newState == 3) || 
      (enc_prev_state == 3 && newState == 2) || 
      (enc_prev_state == 2 && newState == 0)){
        result++;
      }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    active_delay(1);
  }
  return(result/2);
}

void freqtoa(unsigned long f, char *s){
  char p[16];
  
  memset(p, 0, sizeof(p));
  ultoa(f, p, DEC);
  s[0] = 0;
//  strcpy(s, "FREQ: ");

  //one mhz digit if less than 10 M, two digits if more

   if (f >= 100000000l){
    strncat(s, p, 3);
    strcat(s, ".");
    strncat(s, &p[3], 3);
    strcat(s, ".");
    strncat(s, &p[6], 3);
  }
  else if (f >= 10000000l){
    strcat(s, " ");
    strncat(s, p, 2);
    strcat(s, ".");
    strncat(s, &p[2], 3);
    strcat(s, ".");
    strncat(s, &p[5], 3);
  }
  else {
    strcat(s, "  ");
    strncat(s, p, 1);
    strcat(s, ".");
    strncat(s, &p[1], 3);    
    strcat(s, ".");
    strncat(s, &p[4], 3);
  }
}

void updateMeter(){
  int percentage = 0;
  int vswr_reading, r;

  //draw the meter
  GLCD.FillRect(0, 15, 128, 8, WHITE);

  if (mode == MODE_ANTENNA_ANALYZER)
    strcat(c, "  ANT");
  else if (mode == MODE_MEASUREMENT_RX)
    strcat(c, "  MRX");
  else if (mode == MODE_NETWORK_ANALYZER)
    strcat(c, "  SNA");

  if (mode == MODE_ANTENNA_ANALYZER){
    r = analogRead(DBM_READING)/5;
    return_loss = openReading(frequency) - r;
    Serial.print("db:");
    Serial.println(r);
    if (return_loss > 30)
       return_loss = 30;
    if (return_loss < 0)
       return_loss = 0;
    Serial.println(return_loss);
    vswr_reading = pgm_read_word_near(vswr + return_loss);
    sprintf (c, " %d.%01d", vswr_reading/10, vswr_reading%10);
    percentage = vswr_reading - 10;
  }else if (mode == MODE_MEASUREMENT_RX){
    sprintf(c, "%ddbm", analogRead(DBM_READING)/5 + dbmOffset);
    percentage = 110 + analogRead(DBM_READING)/5 + dbmOffset;
  }
  else if (mode == MODE_NETWORK_ANALYZER) {
    sprintf(c, "%ddbm", analogRead(DBM_READING)/5 + dbmOffset);  
    percentage = 110 + analogRead(DBM_READING)/5 + dbmOffset;
  }

  GLCD.DrawString(c, 0, 15);  

/*  
    //debugging code, uncomment to get the absolute power reading in all modes
  //sprintf(c, "%ddbm", analogRead(DBM_READING)/5 + dbmOffset);
  sprintf(c, "%d ", r);
  GLCD.DrawString(c, 0, 24);
*/  
  //leave the offset to 37 pixels
  GLCD.DrawRoundRect(45, 15, 82, 6, 2);
  GLCD.FillRect(47, 17, (percentage * 8)/10, 2, BLACK); 
}

// this builds up the top line of the display with frequency and mode
void updateHeading() {
  int vswr_reading;
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(frequency, b, DEC);

  if (mode == MODE_ANTENNA_ANALYZER)
    strcpy(c, "SWR ");
  else if (mode == MODE_MEASUREMENT_RX)
    strcpy(c, "PWR ");
  else if (mode == MODE_NETWORK_ANALYZER)
    strcpy(c, "SNA ");
  
  //one mhz digit if less than 10 M, two digits if more

   if (frequency >= 100000000l){
    strncat(c, b, 3);
    strcat(c, ".");
    strncat(c, &b[3], 3);
    strcat(c, ".");
    strncat(c, &b[6], 3);
  }
  else if (frequency >= 10000000l){
    strncat(c, b, 2);
    strcat(c, ".");
    strncat(c, &b[2], 3);
    strcat(c, ".");
    strncat(c, &b[5], 3);
  }
  else {
    strncat(c, b, 1);
    strcat(c, ".");
    strncat(c, &b[1], 3);    
    strcat(c, ".");
    strncat(c, &b[4], 3);
  }

  GLCD.DrawString(c, 0, 0);

  itoa(spanFreq/10000, c, 10);
  strcat(c, "K/d");
  GLCD.DrawString(c, 128-(strlen(c)*6), 0);
}


void drawCalibrationMenu(int selection){

  GLCD.ClearScreen();
  GLCD.FillRect(0, 0, 128, 64, WHITE);
  GLCD.DrawString("##Calibration Menu##", 0,0);
  GLCD.DrawString("Freq Calibrate", 20, 20);
  GLCD.DrawString("Return Loss", 20, 40);

  if (selection == 0)
    GLCD.DrawRect(15,15,100,20);
  if (selection == 1)
    GLCD.DrawRect(15,35,100,20);  
}

void calibration_mode(){
  int i, select_item = 0;

 //wait for the button to be lifted
  while(btnDown())
    delay(100);
  delay(100);

  while (1){
    drawCalibrationMenu(select_item);
  
    while(!btnDown()){
      i = enc_read();
      
      if(i > 0 && select_item == 0){
        select_item = 1;
        drawCalibrationMenu(select_item);
      }
      else if (i < 0 && select_item == 1){
        select_item = 0; 
        drawCalibrationMenu(select_item);
      }
      delay(50);
    }  
  
    while(btnDown())
      delay(100);
    delay(100);
    
    if (!select_item)
      calibrateClock();
    else
      calibrateMeter();
  }
  //ytupdateScreen();
}

void uiFreq(int action){

  GLCD.FillRect(0, 25, 128, 11, WHITE);  
  freqtoa(centerFreq, b);
  GLCD.DrawString(b, 39, 27);
  if (uiFocus == MENU_CHANGE_MHZ)
    GLCD.DrawRect(38,25,18,11);
  else if (uiFocus == MENU_CHANGE_KHZ)
    GLCD.DrawRect(62,25,18,11);
  else if (uiFocus == MENU_CHANGE_HZ)
    GLCD.DrawRect(86,25,18,11);

  Serial.print("uiFreq action:");
  Serial.println(action);
  if (!action)
    return; 

  if (action == ACTION_SELECT) {
    //invert the selection
    //GLCD.InvertRect(55, 49, 24, 8);

    if (uiFocus == MENU_CHANGE_MHZ)
      GLCD.InvertRect(38,25,18,11);
    else if (uiFocus == MENU_CHANGE_KHZ)
      GLCD.InvertRect(62,25,18,11);
    else if (uiFocus == MENU_CHANGE_HZ)
      GLCD.InvertRect(86,25,18,11);

    //wait for the button to be released    
    while(btnDown())
      delay(100);
    //wait for a bit to debounce it
    delay(300);
     
    while(!btnDown()){
      int r = analogRead(DBM_READING);
      if (r != prev){
        takeReading(centerFreq);
        updateMeter();
        //Serial.println(r/5);
        prev = r;
      }
      int i = enc_read();
      if (i < 0 && centerFreq > 1000000l){
        if (uiFocus == MENU_CHANGE_MHZ)
          centerFreq += 1000000l * i;
        else if (uiFocus == MENU_CHANGE_KHZ)
          centerFreq += 1000l * i;
        else if (uiFocus == MENU_CHANGE_HZ)
          centerFreq += i;
        if (centerFreq < 4000000000l && centerFreq > 150000000l)
          centerFreq = 150000000l;
        delay(200);
      }
      else if (i > 0 && centerFreq < 499000000l){
        if (uiFocus == MENU_CHANGE_MHZ)
          centerFreq += 1000000l * i;
        else if (uiFocus == MENU_CHANGE_KHZ)
          centerFreq += 1000l * i;
        else if (uiFocus == MENU_CHANGE_HZ)
          centerFreq += i;
        delay(200);
      }
      else 
        continue;
    
    GLCD.FillRect(0, 25, 128, 11, WHITE);  
    freqtoa(centerFreq, b);
    GLCD.DrawString(b, 39, 27);

 
      if (uiFocus == MENU_CHANGE_MHZ)
        GLCD.InvertRect(38,25,18,11);
      else if (uiFocus == MENU_CHANGE_KHZ)
        GLCD.InvertRect(62,25,18,11);
      else if (uiFocus == MENU_CHANGE_HZ)
        GLCD.InvertRect(86,25,18,11);
    }
    delay(200); //wait for the button to debounce

    GLCD.FillRect(0, 25, 128, 11, WHITE);  
    freqtoa(centerFreq, b);
    GLCD.DrawString(b, 39, 27);
    if (uiFocus == MENU_CHANGE_MHZ)
      GLCD.DrawRect(38,25,18,11);
    else if (uiFocus == MENU_CHANGE_KHZ)
      GLCD.DrawRect(62,25,18,11);
    else if (uiFocus == MENU_CHANGE_HZ)
      GLCD.DrawRect(86,25,18,11);
    
  }  
}

void uiSWR(int action){
  GLCD.FillRect(7,38,20,10, WHITE);
  GLCD.DrawString("SWR", 9, 40);

  if (action == ACTION_SELECT){
    mode = MODE_ANTENNA_ANALYZER;
    uiPWR(0);
    uiSNA(0);
    updateScreen();
    EEPROM.put(LAST_MODE, mode);
  }
  
  if (uiFocus == MENU_MODE_SWR)
    GLCD.DrawRect(7,38,20,10);

  if (mode == MODE_ANTENNA_ANALYZER)
    GLCD.InvertRect(8,39,18,8);    

  takeReading(centerFreq);
  updateMeter();
}

void uiPWR(int action){
  GLCD.FillRect(31,38,20,10, WHITE);    
  GLCD.DrawString("PWR", 33, 40);

  if (action == ACTION_SELECT){
    mode = MODE_MEASUREMENT_RX;
    uiSWR(0);
    uiSNA(0);
    updateScreen();
    EEPROM.put(LAST_MODE, mode);
  }
  if (uiFocus == MENU_MODE_PWR)
    GLCD.DrawRect(31,38,20,10);
  if (mode == MODE_MEASUREMENT_RX)
    GLCD.InvertRect(32,39,18,8);
  takeReading(centerFreq);
  updateMeter();
}


void uiSNA(int action){
  GLCD.FillRect(55,38,20,10, WHITE);
  GLCD.DrawString("SNA", 57, 40);

  if (action == ACTION_SELECT){
    mode = MODE_NETWORK_ANALYZER;
    uiSWR(0);
    uiPWR(0);
    updateScreen();
    EEPROM.put(LAST_MODE, mode);
  }
  if (uiFocus == MENU_MODE_SNA)
    GLCD.DrawRect(55,38,20,10);
  if (mode == MODE_NETWORK_ANALYZER)
    GLCD.InvertRect(56,39,18,8);
  takeReading(centerFreq);
  updateMeter();
}

void uiSpan(int action){
  
  GLCD.FillRect(55, 49, 24, 12, WHITE);
  if (spanFreq >= 1000000l)
    sprintf(b, "SPAN +/-%3ldM", spanFreq/1000000l);
  else
    sprintf(b, "SPAN +/-%3ldK", spanFreq/1000l);
    
  GLCD.DrawString(b, 6,52);
  if (uiFocus == MENU_SPAN)
    GLCD.DrawRect(55, 50, 24, 10);

  if (action == ACTION_SELECT) {
      //invert the selection
      GLCD.InvertRect(55, 51, 24, 8);

    //wait for the button to be released    
    while(btnDown())
      delay(100);
    //wait for a bit to debounce it
    delay(300);
     
    while(!btnDown()){
      int i = enc_read();
      if (selectedSpan > 0 && i < -1){
        selectedSpan--;
        spanFreq = spans[selectedSpan];
        EEPROM.put(LAST_SPAN, selectedSpan);
        delay(200);
      }
      else if (selectedSpan < MAX_SPANS && i > 0){
        selectedSpan++;
        spanFreq = spans[selectedSpan];
        EEPROM.put(LAST_SPAN, selectedSpan);
        delay(200);
      }
      else 
        continue;
         
      GLCD.FillRect(55, 49, 24, 10, WHITE);
      if (spanFreq >= 1000000l)
        sprintf(b, "SPAN +/-%3ldM", spanFreq/1000000l);
      else
        sprintf(b, "SPAN +/-%3ldK", spanFreq/1000l);
      GLCD.DrawString(b, 6,52);
      GLCD.InvertRect(55, 51, 24, 8);
    }
    delay(200);
  }
}

void uiPlot(int action){
  GLCD.FillRect(90, 42, 37,20, WHITE);
  GLCD.DrawRect(90, 42, 37,20);
  GLCD.DrawString("PLOT", 98, 49);

  if (uiFocus == MENU_PLOT)
    GLCD.DrawRect(92, 44, 33, 16);

  if (action == ACTION_SELECT){
    if (mode == MODE_ANTENNA_ANALYZER)
      setupVSWRGrid();
    else 
      plotPower();
    updateScreen();
  }
}


void uiMessage(int id, int action){

  switch(id){
    case MENU_CHANGE_MHZ:
    case MENU_CHANGE_KHZ:
    case MENU_CHANGE_HZ:
      uiFreq(action);
      break;
    case MENU_MODE_SWR:
      uiSWR(action);
      break;
    case MENU_MODE_PWR:
      uiPWR(action);
      break;
    case MENU_MODE_SNA:
      uiSNA(action);
      break;
    case MENU_SPAN:
      uiSpan(action);
      break;
    case MENU_PLOT:
      uiPlot(action);
      break;
    default:
      return;
  }
}

void updateScreen(){

  // draw the title bar
  strcpy(b, "#Antuino - ");

  GLCD.ClearScreen();
  switch (mode){
    case MODE_ANTENNA_ANALYZER:
      strcat(b, "SWR");
      break;
    case MODE_MEASUREMENT_RX:
      strcat(b, "PWR");
      break;
    case MODE_NETWORK_ANALYZER:
     strcat(b, "SNA");
     break;
  }

  strcat(b, "   v2.1");
  GLCD.DrawString(b, 1, 1);  
  GLCD.InvertRect(0,0, 128,9);

  //update all the elements in the display
  updateMeter();
  uiFreq(0);
  uiSWR(0);
  uiPWR(0);
  uiSNA(0);
  uiSpan(0);
  uiPlot(0);
}

void doMenu(){
  unsigned long last_freq;
  int i = enc_read();
  
  //btnState = btnDown();
  if (btnDown()){

    //on every button, save the freq.
    EEPROM.get(LAST_FREQ, last_freq);
    if (last_freq != centerFreq){
      EEPROM.put(LAST_FREQ, centerFreq);
    }

    Serial.print("btn before:");Serial.println(uiSelected);
    if (uiSelected == -1)
      uiMessage(uiFocus, ACTION_SELECT);
    if (uiSelected != -1){
      uiMessage(uiFocus, ACTION_DESELECT);
    Serial.print("btn after:");Serial.println(uiSelected);

    }
  }

  if (i == 0)
    return;
    
  if (i > 0 && knob < 80){
        knob += i;
  }
  if (i < 0 && knob >= 0){
      knob += i;      //caught ya
  }

  if (uiFocus != knob/10){
    int prev = uiFocus;
    uiFocus = knob/10;
    uiMessage(prev, 0);
    uiMessage(uiFocus, 0);
  }
}
