#include <Stepper.h>

#include <Keypad.h>
#include <EEPROM.h>
#include <SIM800L.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>


//Naming the pin numbers
const byte IR_Sens_Pin = 42;
const byte alarm_Pin = 6;

//Initializing modules upon first use
//Index 0 will be times per day, index 1 will be time intervals in hours, index 2 will be the time it was last dispensed, 
//index 3 is the designated pin of the motor assigned to this module, index 4 will store how much time has passed since the module has last dispensed a pill, index 5 is if the
//module has dispensed any pills throughout its lifetime, index 6 contains what kind of info it was dispensing last, index 7 is if it has dipensed and is waiting for confirmation
//that the pill has been taken
long modules[5][8]= {
  {0, 0, 0, 1, 0, 1, 0, 0}, //Module 1
  {0, 0, 0, 2, 0, 1, 0, 0}, //Module 2
  {0, 0, 0, 3, 0, 1, 0, 0}, //Module 3
  {0, 0, 0, 0, 0, 1, 0, 0}, //Module 4
  {0, 0, 0, 0, 0, 1, 0, 0}, //Module 5
};

//Define variables for use in module manipulation
const byte dose_Index = 0;
const byte interval_Index = 1;
const byte time_Index = 2;
const byte motor_Pin_Index = 3;
const byte time_Passed_Index = 4;
const byte first_Time_Index = 5;
const byte info_Display_Index = 6;
const byte recently_Dispensed = 7;


//Initializing variables
int module_Id = 0; //Module that is being conifgured
int dose = 0; //Set dose of a module
int interval = 0; //Set interval of a module
int last_Print = 0; //When the LCD last printed a change in the TLT


const int max_Minutes = 180; //Change to use hours later; 180 Minutes = 3hrs
const int max_Dose = 12;
const int stepsPerRevolution = 1024;

bool config = false;
bool display_Info = false;
bool display_TLT = false;

//Keypad inputs
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'C'}, //Config, press to start the setup for a module, 1 is increase dosage per day, 2 is increase time interval
  {'4', '5', '6', '#'}, //# button for switching between modules, 4 is decrease dosage per day, 5 is decrease time interval
  {'7', '8', '9', 'Z'}, //3, 6, 7, 8, 9, z is the dev tool debug button-inator 4000
  {'Y', '0', 'N', 'X'} //Y = yes, N = no, x is undefined
};

byte rowPins[ROWS] = {30, 31, 32, 33}; 
byte colPins[COLS] = {34, 35, 36, 37}; 

Keypad key_Map = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
LiquidCrystal_I2C  lcd = LiquidCrystal_I2C(0x27, 16, 2);
Stepper myStepper1(stepsPerRevolution, 8, 10, 9, 11);   
Stepper myStepper2(stepsPerRevolution, 23, 27, 25, 29);
Stepper myStepper3(stepsPerRevolution, 22, 28, 24, 28);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void setup(){

  pinMode(IR_Sens_Pin, INPUT);
  pinMode(alarm_Pin, OUTPUT);
  lcd.init();
  //lcd.setCursor(0,0);
  //lcd.print("POTATOES");
  //lcd.setCursor(0,1);
  //lcd.print("ARE");
  //lcd.setCursor(0,2);
  //lcd.print("BEST");
  lcd.backlight();
  
  Serial.begin(9600);
  myStepper1.setSpeed(30);
  myStepper2.setSpeed(30);
  myStepper3.setSpeed(30);

  for (int i = 0; i <= 5; i++) {
    pinMode(modules[i][motor_Pin_Index], OUTPUT);
  }
  lcd.print("Hello");
}
  
void loop(){
  
char key_Pressed = key_Map.getKey();

if (display_TLT) {
    
    if (get_time_passed_gen(last_Print) >= 1) {
      modules[module_Id][info_Display_Index] = 2;
      display_info(2);
    if (get_time_passed_gen(last_Print) != last_Print) {
      last_Print = millis();
    }
    }
}

if (key_Pressed) 
  //Serial.println(key_Pressed);
{
  if (key_Pressed != '3') {
    display_TLT = false;
  }
  switch (key_Pressed) {
    case 'C': //Check if going to configure
      config = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Config Mode");
      Serial.print("Config Mode");
    break;

    case 'Y': //Check if done configuring
      config = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("End Config Mode");
      Serial.print("End Config Mode");
    break;

    case 'N': //Dispense now
      //dispense_pill(255, 5, 13);
      Serial.println(modules[module_Id][first_Time_Index]);
      modules[module_Id][first_Time_Index] = false;
      Serial.println(modules[module_Id][first_Time_Index]);
      //dispense_pill(modules[module_Id][motor_Pin_Index]);
      modules[module_Id][time_Index] = millis();
      //modules[module_Id][recently_Dispensed] = true;
    break;

    case '9':
    //change_info_index();
    display_info(modules[module_Id][info_Display_Index]);
  }

  if (config) 
  {
    //Check what config action is being done as of the moment
    switch (key_Pressed) 
    {
      case '#':
        
        //dose = 0;
        //interval = 0; //haha fuck you future raj
        change_Module_Id();
        display_info(modules[module_Id][info_Display_Index]);
        
        dose = modules[module_Id][dose_Index];
        interval = modules[module_Id][interval_Index];
      break;

      case 'Z': //Debug button, prints ten new lines to declutter the console
        print_debug_info();  
      break;
      case '1':
        add_Dose(module_Id);
        modules[module_Id][info_Display_Index] = 0;
        display_info(0);
      break;

      case '2':
        add_Interval(module_Id);
        modules[module_Id][info_Display_Index] = 1;
        display_info(1);
      break;

      case '4':
        decrease_Dose(module_Id);
        modules[module_Id][info_Display_Index] = 0;
        display_info(0);
      break;

      case '5':
        decrease_Interval(module_Id);
        modules[module_Id][info_Display_Index] = 1;
        display_info(1);
      break;

      case '3':
        if (display_TLT) {
          display_TLT = false; 
          lcd.clear();
        } else {
          display_TLT = true; 
        }
      break;

      case '6':
        lcd.clear();
      break;
    }
  }

  
}
  
  check_dispense();
  check_Taken();
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Check if a module is supposed to dispense a pill
void check_dispense() {
for (int i = 0; i <= 5; i++) {

  get_time_passed(i);

  if (modules[i][time_Passed_Index] >= modules[i][interval_Index]) {
    
    if (modules[i][first_Time_Index] == true || modules[i][recently_Dispensed] == true) {
      continue;
    }
      digitalWrite(alarm_Pin, HIGH);
      modules[i][recently_Dispensed] = true;
      dispense_pill(modules[i][motor_Pin_Index]);
    }
  }
}

//Setting which module to configure
int change_Module_Id() {

    if (module_Id < 4)
    { 
      module_Id++; 
    } else { 
      module_Id = 0;
    }
  Serial.print("Module #");
  Serial.print(module_Id);
  Serial.print("\n");
  return module_Id;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_Dose(int module_Id) {

  if (dose < max_Dose)
  {
    dose++;
  } else {
    dose = 0;
  }
  modules[module_Id][dose_Index] = dose; //Store the dose
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void decrease_Dose(int module_Id) {

  if (dose > 0)
  {
    dose--;
  } else {
    dose = 12;
  }
  modules[module_Id][dose_Index] = dose; //Store the dose
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_Interval(int module_Id) {

  if (interval < max_Minutes)
  {
    interval++;
  } else {
    interval = 0;
  }

  modules[module_Id][interval_Index] = interval; //Store the interval
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void decrease_Interval(int module_Id) {

  if (interval > 0)
  {
    interval--;
  } else {
    interval = 72;
  }
  modules[module_Id][interval_Index] = interval; //Store the interval

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void dispense_pill(int motor_Pin) {
  
  switch (motor_Pin) {
  
  case 1:  
    myStepper1.step(stepsPerRevolution * 2);
  break;

  case 2:
    myStepper2.step(stepsPerRevolution * 2);
  break;

  case 3:
    myStepper3.step(stepsPerRevolution * 2);
  break;
  }
  
}

void check_Taken() { 

  bool IR_Sens = true; //digitalRead(IR_Sens_Pin); //Check IR sensor if the pill was taken

  if (IR_Sens) { //remove ! when actual testing
  
  digitalWrite(alarm_Pin, LOW); //Turn off alarm pin

    for (int i = 0; i < 5; i++) {
      
      if (modules[i][recently_Dispensed] == true) {
      modules[i][recently_Dispensed] = false; //confirm to machine pill has been taken
      modules[i][time_Index] = millis(); //Set time_index to current time
      }
      
    }
    
  }

}
 

void get_time_passed(int module) {

  float current_Time = millis();
  float time_Passed = current_Time - modules[module][time_Index];
  time_Passed /= 1000; //60000 is for minutes (debug), use 3600000 for actual testing

  modules[module][time_Passed_Index] = time_Passed;

}

int get_time_passed_gen(int last_Dis) {
  float current_Time = millis();
  int time_Passed = current_Time - last_Dis;
  time_Passed /= 1000;

  return time_Passed;
}

void print_debug_info() {
  for (int i = 0; i < 10; i++)
      {
        Serial.print("\n");
      }
        Serial.print("Module #");
        Serial.print(module_Id);
        Serial.print("\n");
        Serial.print("Doses per day: ");
        Serial.print(modules[module_Id][dose_Index]);
        Serial.print("\n");
        Serial.print("Interval (Hours): ");
        Serial.print(modules[module_Id][interval_Index]);
        Serial.print("\n");
        Serial.print("Last Dispensed: ");
        Serial.print(modules[module_Id][time_Index]);
        Serial.print("\n");
        Serial.print("Has dispensed: "); //THIS LINE OF CODE BREAKS THE MACHINE FOR SOME STUPID FUCKING REASON AND I DON'T UNDERSTAND WHY
        Serial.print(modules[module_Id][first_Time_Index]);
        Serial.print("\n");
      
}

void display_info(int info_Index) {
  
  lcd.clear();
  lcd.setCursor(0, 0);
  
    switch (info_Index) {
    
    case -1:
    lcd.print("Module #");
    lcd.print(module_Id);
    break;

    case 0:
    lcd.print("Module #");
    lcd.print(module_Id);
    lcd.setCursor(0, 1);
    lcd.print("Doses per day:");
    lcd.print(modules[module_Id][dose_Index]);
    Serial.print("Doses per day:");
    Serial.print(modules[module_Id][dose_Index]);
    break;

    case 1:
    lcd.print("Module #");
    lcd.print(module_Id);
    lcd.setCursor(0, 1);
    lcd.print("Interval:"); //Change to (hours) when final testing
    lcd.setCursor(9, 1); //Remove this line of code when final testing
    lcd.print(modules[module_Id][interval_Index]);
    Serial.print("Interval:"); 
    Serial.print(modules[module_Id][interval_Index]);
    break;

    case 2:
    display_TLT = true;
    lcd.print("Module #");
    lcd.print(module_Id);
    lcd.setCursor(0, 1);
    if (modules[module_Id][first_Time_Index] == false) {
      lcd.print("TLT: "); //TLT means time last dispensed
      lcd.print(modules[module_Id][time_Passed_Index] + 1);
      lcd.print("s ago "); 
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Not setup!");
    }
    Serial.print("TLT: "); //TLT means time last dispensed
    Serial.print(modules[module_Id][time_Passed_Index]);
    Serial.print("\n");
    break;

    case 3:
    lcd.print("Module #");
    lcd.print(module_Id);
    lcd.setCursor(0, 1);
    lcd.print("Has Dispensed: "); 
    lcd.print(modules[module_Id][first_Time_Index]);
    Serial.print("Has Dispensed: "); 
    Serial.print(modules[module_Id][first_Time_Index]);
    break;

    default:
    lcd.clear();
    break;
    }

}
/*
void change_info_index() {

  if (info_Index < 3)
  {
    info_Index++;
  } else {
    info_Index = 0;
  }

  modules[module_Id][display_Info_Index] = info_Index;
}
*/

/*
  _    _                          _    _____          _      
 | |  | |                        | |  / ____|        | |     
 | |  | |_ __  _   _ ___  ___  __| | | |     ___   __| | ___ 
 | |  | | '_ \| | | / __|/ _ \/ _` | | |    / _ \ / _` |/ _ \
 | |__| | | | | |_| \__ \  __/ (_| | | |___| (_) | (_| |  __/
  \____/|_| |_|\__,_|___/\___|\__,_|  \_____\___/ \__,_|\___|

  (a.k.a code graveyard) -lynd

char numbers[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

for (int i = 0; i <= 10; i++) //Checks if a number is being pressed
{
  if (key_Pressed == numbers[i])
  {
    set_value(key_Pressed, module_Id, config_Id); //Set value
    break;
  }
}

//Setting config mode
int config_mode() {

    if (config_Id < 1)
    { 
      config_Id++; 
    } else { 
      config_Id = 0;
    }

  Serial.print(config_Id);
  Serial.print("\n");
  return config_Id;
}

void set_value(char key_Pressed, int module_id, int config_Id) {
  
  modules[module_id][config_Id] = int(key_Pressed)*3600000;
  
}

void store_time_passed() {

float time_Passed; //Declare variable

  for (int i = 0; i < 5; i++) {
    time_Passed = millis() - modules[i][time_Index]; //Get how much time has passed since last dispense
    modules[i][time_Passed_Index] = time_Passed; //Store how much time has passed since last dispense
  }

}

void convert_time_units(float original_Time, char time_Unit) {
  
  float converted_time;

  switch (time_Unit) {
    
    case 1: //Seconds
      converted_time = original_Time/1000;
    break;

    case 2: //Minutes
      converted_time = original_Time/60000;
    break;

    case 3: //Hours
      converted_time = original_Time/3600000;
    break;

    default: //Milliseconds
      converted_time = original_Time;
  }
return converted_time;
}

void decrease_info_index() {

  if (info_Index > 0)
  {
    info_Index--;
  } else {
    info_Index = 3;
  }
}




*/
