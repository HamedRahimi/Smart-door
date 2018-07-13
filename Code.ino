#include <Keypad.h>
#include <AddicoreRFID.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define  uchar unsigned char
#define uint  unsigned int

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

//////////////////////Analog Input For Reseting all/////////
int RST_Pin = A0;
int cntrl;
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

//4 bytes tag serial number, the first 5 bytes for the checksum byte
uchar serNumA[5];
uchar fifobytes;
uchar fifoValue;

AddicoreRFID myRFID; // create AddicoreRFID object to control the RFID module
/////////////////////////////////////////////////////////////////////
//set the pins
/////////////////////////////////////////////////////////////////////
const int chipSelectPin = 10;
int i = 0; ///For counting at functions
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'/', '=', '0', '.'},
  {'*', '9', '8', '7'},
  {'-', '6', '5', '4'},
  {'+', '3', '2', '1'}
};

///////////////////OUTPUT PIN////////////////////////////////////
const int RED =1 ; 
const int GREEN =0;////Lock too
const int BUZZER =16 ;

byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//Maximum length of the array
#define MAX_LEN 16

String MPSW = "1111" ;
String NEW_MPSW[2] = {""};
String String_CARD_No = String();
String Charge[4] = {""};
String Input = String();//

bool Pass_Allow = LOW ;
bool Break_Func0 = HIGH;
bool Break_Func3 = HIGH;
bool Break_Func1 = HIGH;
bool Break_Func2 = HIGH;
bool Iner_Break_Func2 = HIGH;
bool Iner_Break_Key = HIGH;
bool Iner_Break_Card = HIGH;

char CustomKey_Pass;
char CustomKey_Function;
char CustomKey_Iner;

int Time_Pass1 = 0;
int Time_Pass2 = 0;
int Time_Func1 = 0;
int Time_Func2 = 0;
int Time_Temp = 0;
int Iner_Time_Func1 = 0;
int Iner_Time_Func2 = 0;
int Iner_Time_Key1 = 0;
int Iner_Time_Key2 = 0;
int Iner_Time_Card1 = 0;
int Iner_Time_Card2 = 0;
int Time_RST1 = 0;
int Time_RST2 = 0;

const int  No_Users = 5 ;
const int delay_time = 10000; 

byte Credit[No_Users] = {0};
byte C_No[No_Users][5] = {0};
byte UPSW[No_Users][2] = {0} ;
byte Iner_Meth[No_Users] = {0} ;

void setup() {
  // initialize the LCD
  lcd.begin();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print("Initializing");
  lcd.setCursor(4, 1);
  lcd.print("Please Wait");
  delay(1000);
 // Serial.begin(9600);
  SPI.begin();
  pinMode(chipSelectPin, OUTPUT);             // Set digital pin 10 as OUTPUT to connect it to the RFID /ENABLE pin
  digitalWrite(chipSelectPin, LOW);         // Activate the RFID reader
  myRFID.AddicoreRFID_Init();
  for (int j = 0; j < No_Users; j++ )
  {
    Credit[j] = EEPROM.read(j);
  }

  for ( int c = 0 ; c < No_Users; c++ ) {
    for (int t = 0; t < 5; t++) {
      C_No[c][t] = EEPROM.read(No_Users + (c * 5) + t);
    }
  }

  for ( int p = 0; p < No_Users; p++ )
  {
    for (int f = 0; f < 2; f++) {
      UPSW[p][f] = EEPROM.read(6 * No_Users + (p * 2) + f);
    }
  }
  for ( int k = 0; k < No_Users; k++ )
  {
    Iner_Meth[k] =  EEPROM.read(8 * No_Users + k);
  }
  /////////////  TO READ PASSWORD /////////////////////////////
  if (EEPROM.read(9 * No_Users + 2) < 200) {
    if (100 < EEPROM.read(9 * No_Users + 2)) {
      MPSW = String(EEPROM.read(9 * No_Users) * 255 + EEPROM.read(9 * No_Users + 1));
    }
  }


  lcd.setCursor(8, 2);
  lcd.print("Done!");
  delay(1000);
  lcd.clear();
}

void loop() {
  if (Pass_Allow == LOW) {
    delay(100);
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Enter *+MPSW");
    lcd.setCursor(6, 2);
    lcd.print("PUT TAG");
    lcd.setCursor(5, 1);
    lcd.print("Enter UPSW");
    lcd.setCursor(0, 3);
    lcd.print(Input);
  }
  if (Pass_Allow) {
    delay(100);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("0-RESET");
    lcd.setCursor(8, 0);
    lcd.print("1-Chnge MPSW");
    lcd.setCursor(0, 2);
    lcd.print("2-SAVE TAG");
    lcd.setCursor(11, 2);
    lcd.print("3-SAVE ID");
    lcd.setCursor(0, 3);
    lcd.print(Input);
  }
  Time_Pass1 = millis();
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////Card Part////////////////////////////////////
  uchar i, tmp, checksum1;
  uchar status;
  uchar str[MAX_LEN];
  uchar RC_size;
  uchar blockAddr;  //Selection operation block address 0 to 63
  String mynum = "";


  //Find tags, return tag type
  status = myRFID.AddicoreRFID_Request(PICC_REQIDL, str);
  //Anti-collision, return tag serial number 4 bytes
  status = myRFID.AddicoreRFID_Anticoll(str);


  if (status == MI_OK)
  {
    checksum1 = str[0] ^ str[1] ^ str[2] ^ str[3];
    Serial.println("The tag's number is  : ");
    //Serial.print(2);
    Serial.print(str[0]);
    Serial.print(" , ");
    Serial.print(str[1]);
    Serial.print(" , ");
    Serial.print(str[2]);
    Serial.print(" , ");
    Serial.print(str[3]);
    Serial.print(" , ");
    Serial.print(str[4]);
    Serial.println();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("The tag's number : ");
    lcd.setCursor(0, 1);
    lcd.print(str[0]);
    lcd.print(" ");
    lcd.print(str[1]);
    lcd.print(" ");
    lcd.print(str[2]);
    lcd.print(" ");
    lcd.print(str[3]);
    lcd.print(" ");
    lcd.print(str[4]);
    delay(1000);
    for (i = 0; i <= No_Users; i++) {

      if (C_No[i][0] == str[0] && C_No[i][1] == str[1] && C_No[i][2] == str[2] && C_No[i][3] == str[3] && C_No[i][4] == str[4])
      {
        delay(1000);
        lcd.setCursor(0, 2);
        lcd.print("Card number is: ");
        lcd.print(i);
        Serial.println("Card number is:");
        Serial.println(i);
        Serial.println("Credit is :");
        Serial.println(Credit[i]);
        lcd.setCursor(0, 3);
        lcd.print("Credit is :");
        lcd.print(Credit[i]);
        delay(1000);
        if (Credit[i] > 0) {
          if (Iner_Meth[i] == 1) {
            if (Credit[i] < 99)
            {
              
              Serial.println("CARD CAN OPEN THE DOOR");
              Credit[i]--;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("CARD CAN OPEN DOOR");
              lcd.setCursor(2, 1);
              lcd.print("Credit reduced : ");
              lcd.setCursor(9, 2);
              lcd.print(Credit[i]);
              Serial.println("Credit reduced :");
              Serial.println(Credit[i]);
              Open_Door();

            }

            else if ( Credit[i] == 99)
            {
              
              Serial.println(" CARD CAN OPEN THE DOOR");
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("CARD CAN OPEN DOOR");
              Open_Door();
            }
            EEPROM_Write();
            delay(2000);
          }
          if (Iner_Meth[i] == 2)
          {
            Time_Temp = millis();
            Iner_Time_Card1 = Time_Temp;
            Iner_Time_Card2 = Time_Temp;
            Serial.println("insert Your PassWord ");

            while (Iner_Break_Card)
            {
              delay(100);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Insert Your Password ");
              lcd.setCursor(0, 1);
              lcd.print(Input);
              Iner_Time_Card1 = millis();
              CustomKey_Iner = customKeypad.getKey();
              if (CustomKey_Iner) {
                if ( CustomKey_Iner == '/') {
                  Serial.println(Input);
                  Time_Temp = millis();
                  Iner_Time_Card1 = Time_Temp;
                  Iner_Time_Card2 = Time_Temp;
                }
                else {
                  Input = Input + CustomKey_Iner ;
                  Serial.println(Input);
                  Time_Temp = millis();
                  Iner_Time_Card1 = Time_Temp;
                  Iner_Time_Card2 = Time_Temp;

                }
              }
              if (CustomKey_Iner == '/') {
                if (Input.toInt() == (UPSW[i][0] * 255) + UPSW[i][1]) {
                  if (Credit[i] < 99)
                  {
                    
                    Serial.println(" CARD CAN OPEN THE DOOR");
                    Credit[i]--;
                    Serial.println("Credit reduced :");
                    Serial.println(Credit[i]);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("CARD CAN OPEN DOOR");
                    lcd.setCursor(2, 1);
                    lcd.print("Credit reduced : ");
                    lcd.setCursor(9, 2);
                    lcd.print(Credit[i]);
                    Open_Door();
                    EEPROM_Write();
                    delay(1000);
                  }

                  else if ( Credit[i] == 99)
                  {
                    Open_Door();
                    Serial.println(" CARD CAN OPEN DOOR");
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("CARD CAN OPEN DOOR");
                    Open_Door();
                    delay(2000);
                  }
                }
                Iner_Break_Card = LOW;
                Input = "";
              }

              if (Iner_Time_Card1 - Iner_Time_Card2 >= delay_time)
              {
                Iner_Break_Card = LOW ;
              }
            }
            Iner_Break_Card = HIGH ;
          }
        }
        else  {
          Serial.println(" CARD CAN not OPEN THE DOOR");
          lcd.clear();
          lcd.setCursor(2, 1);
          lcd.print("CARD   CAN   NOT");
          lcd.setCursor(3, 2);
          lcd.print("OPEN THE DOOR");
          Error();
          delay(1000);
        }
      }
    }
  }


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////Key Part////////////////////////////////////
  CustomKey_Pass = customKeypad.getKey();

  if (CustomKey_Pass) {
    if (CustomKey_Pass == '/') {
      Serial.println(Input);
      Time_Temp = millis();
      Time_Pass1 = Time_Temp;
      Time_Pass2 = Time_Temp;

      if (Pass_Allow == LOW) {
        if (Input == "*" + MPSW) {
          Serial.println("Pass is Correct");
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Pass is Correct !");
          Pass_Allow = HIGH;
          Input = "";
          delay(1000);
        }
        else {

          for (int Count = 0; Count < No_Users; Count++)
          {
            if (Input.toInt() == (UPSW[Count][0] * 255) + UPSW[Count][1]) {
              if (Credit[Count] > 0) {

                if (Iner_Meth[Count] == 0)
                {
                  if (Credit[Count] < 99)
                  {
                    Serial.println(" CARD CAN OPEN THE DOOR");
                    Credit[Count]--;
                    Serial.println("Credit reduced :");
                    Serial.println(Credit[Count]);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("CARD CAN OPEN DOOR");
                    lcd.setCursor(2, 1);
                    lcd.print("Credit reduced : ");
                    lcd.setCursor(9, 2);
                    lcd.print(Credit[i]);
                    Open_Door();
                    EEPROM_Write();
                    delay(1000);
                  }

                  else if ( Credit[Count] == 99)
                  {
                    
                    Serial.println(" CARD CAN OPEN THE DOOR");
                    Serial.println(Credit[Count]);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("CARD CAN OPEN DOOR");
                    Open_Door();
                    delay(1000);
                  }

                  else {
                    Serial.println(" CARD CAN not OPEN THE DOOR");
                    lcd.clear();
                    lcd.setCursor(2, 1);
                    lcd.print("CARD   CAN   NOT");
                    lcd.setCursor(3, 2);
                    lcd.print("OPEN THE DOOR");
                    Error();
                    delay(2000);
                  }
                }
                else  if (Iner_Meth[Count] == 2)
                {
                  Time_Temp = millis();
                  Iner_Time_Key1 = Time_Temp;
                  Iner_Time_Key2 = Time_Temp;
                  Serial.println("insert card ");
                  while (Iner_Break_Key)
                  {
                    delay(100);
                    lcd.clear();
                    lcd.setCursor(2, 0);
                    lcd.print("Insert Your TAG");
                    Iner_Time_Key1 = millis();
                    uchar i, tmp, checksum1;
                    uchar status;
                    uchar str[MAX_LEN];
                    uchar RC_size;
                    uchar blockAddr;  //Selection operation block address 0 to 63
                    String mynum = "";


                    //Find tags, return tag type
                    status = myRFID.AddicoreRFID_Request(PICC_REQIDL, str);
                    //Anti-collision, return tag serial number 4 bytes
                    status = myRFID.AddicoreRFID_Anticoll(str);

                    if (status == MI_OK)
                    {
                      checksum1 = str[0] ^ str[1] ^ str[2] ^ str[3];
                      Serial.println("The tag's number is  : ");
                      //Serial.print(2);
                      Serial.print(str[0]);
                      Serial.print(" , ");
                      Serial.print(str[1]);
                      Serial.print(" , ");
                      Serial.print(str[2]);
                      Serial.print(" , ");
                      Serial.print(str[3]);
                      Serial.print(" , ");
                      Serial.print(str[4]);
                      Serial.println();

                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("The tag's number : ");
                      lcd.setCursor(0, 1);
                      lcd.print(str[0]);
                      lcd.print(" ");
                      lcd.print(str[1]);
                      lcd.print(" ");
                      lcd.print(str[2]);
                      lcd.print(" ");
                      lcd.print(str[3]);
                      lcd.print(" ");
                      lcd.print(str[4]);
                      delay(1000);

                      if (C_No[Count][0] == str[0] && C_No[Count][1] == str[1] && C_No[Count][2] == str[2] && C_No[Count][3] == str[3] && C_No[Count][4] == str[4])
                      {
                        delay(1000);
                        lcd.setCursor(0, 2);
                        lcd.print("Card number is: ");
                        lcd.print(Count);
                        Serial.println("Card number is:");
                        Serial.println(Count);
                        Serial.println("Credit is :");
                        Serial.println(Credit[Count]);
                        lcd.setCursor(0, 3);
                        lcd.print("Credit is :");
                        lcd.print(Credit[Count]);
                        delay(1000);
                        if (Credit[Count] > 0) {
                          if (Credit[Count] < 99)
                          {
                            Serial.println(" CARD CAN OPEN THE DOOR");
                            Credit[Count]--;
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("CARD CAN OPEN DOOR");
                            lcd.setCursor(2, 1);
                            lcd.print("Credit reduced : ");
                            lcd.setCursor(9, 2);
                            lcd.print(Credit[Count]);
                            Serial.println("Credit reduced :");
                            Serial.println(Credit[Count]);
                            Open_Door();

                          }

                          else if ( Credit[Count] == 99)
                          {
                            Serial.println(" CARD CAN OPEN THE DOOR");
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("CARD CAN OPEN DOOR");
                            Open_Door();
                          }
                          EEPROM_Write();
                          delay(1000);
                        }
                        else {

                          Serial.println(" CARD CAN not OPEN THE DOOR");
                          lcd.clear();
                          lcd.setCursor(2, 1);
                          lcd.print("CARD   CAN   NOT");
                          lcd.setCursor(3, 2);
                          lcd.print("OPEN THE DOOR");
                          Error();
                          delay(1000);
                        }
                      }
                      else {
                        lcd.clear();
                        lcd.setCursor(0, 1);
                        lcd.print(" CARD IS NOT RIGHT !");
                        Error();
                        delay(1000);
                        Serial.println("Card is not right");
                      }
                      delay(1000);
                      Iner_Break_Key = LOW ;
                    }


                    if (Iner_Time_Key1 - Iner_Time_Key2 >= delay_time)
                    {
                      Iner_Break_Key = LOW ;
                    }
                  }

                  Iner_Break_Key = HIGH ;
                }
              }
            }
          }
        }
        Input = "";
      }

      else if (Pass_Allow == HIGH)
      {

        if (Input == "0")
        {
          Serial.println("func0");
          Input = "";
          FUNC_0();
          Break_Func0 = HIGH;
        }
        else if (Input == "1") {
          Serial.println("func1");
          Input = "";
          FUNC_1();
          Break_Func1 = HIGH;
        }
        else if (Input == "2") {
          Serial.println("func2");
          Input = "";
          FUNC_2();
          Break_Func2 = HIGH;
          Iner_Break_Func2 = HIGH;
        }
        else if (Input == "3") {
          Serial.println("func3");
          Input = "";
          FUNC_3();
          Break_Func3 = HIGH;
        }

      }
      Input = "";
    }

    else {
      Input = Input + CustomKey_Pass ;
      Serial.println(Input);
      Time_Temp = millis();
      Time_Pass1 = Time_Temp;
      Time_Pass2 = Time_Temp;
    }
  }


  if (Time_Pass1 - Time_Pass2  >= delay_time)
  {
    Input = "";
    Pass_Allow = LOW;
  }
  myRFID.AddicoreRFID_Halt();
  Time_Temp = millis();
  Time_RST1 = Time_Temp;
  Time_RST2 = Time_Temp;
  cntrl = analogRead(RST_Pin);
  while ( cntrl > 900)
  {
    Time_RST1 = millis();
    if (Time_RST1 - Time_RST2 > delay_time) {
      Reset_All();
      delay(3000);
      Time_Temp = millis();
      Time_RST1 = Time_Temp;
      Time_RST2 = Time_Temp;
      break;
    }
     cntrl = analogRead(RST_Pin);
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void FUNC_0(void)
{



  Time_Temp = millis();
  Time_Func1 = Time_Temp;
  Time_Func2 = Time_Temp;
  Serial.println("Func 0 Started");
  while (Break_Func0)
  {
    delay(200);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("0*MPSW TO RESET ALL");
    lcd.setCursor(0, 1);
    lcd.print("ENTER CARD NUMBER");
    lcd.setCursor(0, 3);
    lcd.print(Input);
    Time_Func1 = millis();
    CustomKey_Function = customKeypad.getKey();
    if (CustomKey_Function) {
      if (CustomKey_Function == '/') {
        Serial.println(Input);
        Serial.println("func0 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;
      }
      else {
        Input = Input + CustomKey_Function ;
        Serial.println(Input);
        Serial.println("func0 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;

      }
    }
    if (CustomKey_Function == '/') {

      if (Input == "0*" + MPSW) {
        Reset_All();

        delay(3000);
      }
      else if (Input.toInt() > 0) {
        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print("Credit[");
        lcd.print(Input.toInt());
        lcd.print("] = 0");
        Credit[Input.toInt()] = 0;
        Serial.print("Credit[");
        Serial.print(Input.toInt());
        Serial.println("] = 0");
        EEPROM_Write();
        delay(3000);

      }
      else if (Input.toInt() == 0) {
        if (Input == "0") {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Credit[");
          lcd.print(Input.toInt());
          lcd.print("] = 0");
          Credit[Input.toInt()] = 0;
          Serial.print("Credit[");
          Serial.print(Input.toInt());
          Serial.println("] = 0");
          EEPROM_Write();
          delay(3000);

        }
      }
      Input = "";
      Break_Func0 = LOW ;
    }
    if (Time_Func1 - Time_Func2 >= delay_time)
    {
      Input = "";
      Break_Func0 = LOW ;
    }
  }
  i = 0;
}
//////////////////////////////////////////////////////////////////////////////////
void FUNC_2(void)
{


  Time_Temp = millis();
  Time_Func1 = Time_Temp;
  Time_Func2 = Time_Temp;
  Serial.println("Func 2 Started");
  while (Break_Func2)
  {
    delay(100);
    lcd.clear();

    if (i == 0) {
      lcd.setCursor(4, 0);
      lcd.print("ENTER Credit");
    }
    else if (i == 1) {
      lcd.setCursor(1, 0);
      lcd.print("ENTER CARD Number");
    }
    else if (i == 2) {
      lcd.setCursor(5, 0);
      lcd.print("ENTER UPSW");
    }
    else if (i == 3) {
      lcd.setCursor(0, 0);
      lcd.print("ENTER INER METHOD :");
    }
    lcd.setCursor(0, 1);
    lcd.print(Input);

    Time_Func1 = millis();
    CustomKey_Function = customKeypad.getKey();
    if (CustomKey_Function) {
      if (CustomKey_Function == '/') {
        Serial.println(Input);
        Serial.println("func2 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;
      }
      else {
        Input = Input + CustomKey_Function ;
        Serial.println(Input);
        Serial.println("func2 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;

      }
    }
    if (CustomKey_Function == '/') {
      if (Input.toInt() > 0) {

        i++ ;
        Charge[i - 1] = Input;

      }
      else  if (Input.toInt() == 0) {
        if (Input == "0") {
          i++ ;
          Charge[i - 1] = Input;
        }
      }
      Input = "";
    }
    if (i == 4) {

      Time_Temp = millis();
      Iner_Time_Func1 = Time_Temp;
      Iner_Time_Func2 = Time_Temp;
      Serial.println("insert card ");
      while (Iner_Break_Func2)
      {
        delay(100);
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print("Insert Your TAG");

        Iner_Time_Func1 = millis();
        uchar i, tmp, checksum1;
        uchar status;
        uchar str[MAX_LEN];
        uchar RC_size;
        uchar blockAddr;  //Selection operation block address 0 to 63
        String mynum = "";


        //Find tags, return tag type
        status = myRFID.AddicoreRFID_Request(PICC_REQIDL, str);
        //Anti-collision, return tag serial number 4 bytes
        status = myRFID.AddicoreRFID_Anticoll(str);


        if (status == MI_OK)
        {

          checksum1 = str[0] ^ str[1] ^ str[2] ^ str[3];
          Serial.println("The tag's number is  : ");

          Serial.print(str[0]);
          Serial.print(" , ");
          Serial.print(str[1]);
          Serial.print(" , ");
          Serial.print(str[2]);
          Serial.print(" , ");
          Serial.print(str[3]);
          Serial.print(" , ");
          Serial.print(str[4]);
          Serial.println();

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("The tag's number : ");
          lcd.setCursor(0, 1);
          lcd.print(str[0]);
          lcd.print(" ");
          lcd.print(str[1]);
          lcd.print(" ");
          lcd.print(str[2]);
          lcd.print(" ");
          lcd.print(str[3]);
          lcd.print(" ");
          lcd.print(str[4]);


          Credit[Charge[1].toInt()] = Charge[0].toInt();
          UPSW[Charge[1].toInt()][0] = Charge[2].toInt() / 255;
          UPSW[Charge[1].toInt()][1] = Charge[2].toInt() %  255;

          Iner_Meth[Charge[1].toInt()] = Charge[3].toInt();
          for (i = 0; i <= 4; i++) {
            C_No[Charge[1].toInt()][i] = str[i];
          }

          Break_Func3 = LOW ;
          Iner_Break_Func2 = LOW;
          String_CARD_No  = "";
          Input = "";
          i = 0;
          Time_Temp = millis();
          Iner_Time_Func1 = Time_Temp;
          Iner_Time_Func2 = Time_Temp;
          EEPROM_Write();
          Serial.println("Card Charged and Saved");
          lcd.setCursor(0, 3);
          lcd.print("TAG Charged & Saved");
          delay(3000);
        }

        if (Iner_Time_Func1 - Iner_Time_Func2 >= delay_time)
        {
          String_CARD_No  = "";
          Iner_Break_Func2 = LOW ;
        }
      }

      i = 0;
    }
    if (Time_Func1 - Time_Func2 >= delay_time)
    {
      Input = "";
      Break_Func2 = LOW ;
    }
  }
  i = 0;
}
/////////////////////////////////////////////////////////////////////////////////

void FUNC_3(void)
{
  int i = 0 ;

  Time_Temp = millis();
  Time_Func1 = Time_Temp;
  Time_Func2 = Time_Temp;
  Serial.println("func3 Started");
  while (Break_Func3)
  {
    delay(100);
    lcd.clear();

    if (i == 0) {
      lcd.setCursor(5, 0);
      lcd.print("ENTER Credit");
    }
    else if (i == 1) {
      lcd.setCursor(1, 0);
      lcd.print("ENTER CARD Number");
    }
    else if (i == 2) {
      lcd.setCursor(5, 0);
      lcd.print("ENTER UPSW");
    }

    lcd.setCursor(0, 1);
    lcd.print(Input);

    Time_Func1 = millis();
    CustomKey_Function = customKeypad.getKey();
    if (CustomKey_Function) {
      if (CustomKey_Function == '/') {
        Serial.println(Input);
        Serial.println("func3 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;
      }
      else {
        Input = Input + CustomKey_Function ;
        Serial.println(Input);
        Serial.println("func3 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;

      }
    }
    if (CustomKey_Function == '/') {
      if (Input.toInt() > 0) {

        i++ ;
        Charge[i - 1] = Input;

      }
      else  if (Input.toInt() == 0) {
        if (Input == "0") {
          i++ ;
          Charge[i - 1] = Input;
        }
      }
      Input = "";
    }
    if (i == 3) {

      Credit[Charge[1].toInt()] = Charge[0].toInt();
      UPSW[Charge[1].toInt()][0] = (Charge[2].toInt()) / 255 ; /// KHAREJE GHESMAT
      UPSW[Charge[1].toInt()][1] = Charge[2].toInt() %  255  ; /// BAGHIMANDE
      Iner_Meth[Charge[1].toInt()] = 0;
      EEPROM_Write();
      Serial.println("Card Charged and Saved");
      lcd.clear();
      lcd.setCursor(7, 0);
      lcd.print("KEY  ID");
      lcd.setCursor(7, 1);
      lcd.print("Charged");
      lcd.setCursor(9, 2);
      lcd.print("And");
      lcd.setCursor(8, 3);
      lcd.print("Saved");
      delay(3000);
      Input = "";
      Break_Func3 = LOW ;

    }
    if (Time_Func1 - Time_Func2 >= delay_time)
    {
      Input = "";
      Break_Func3 = LOW ;
    }
  }
  Charge[0, 1, 2, 3] = "";
  i = 0;
}

///////////////////////////////////////////////////////////////////////////////////////

void FUNC_1(void)
{
  int i = 0 ;
  Time_Temp = millis();
  Time_Func1 = Time_Temp;
  Time_Func2 = Time_Temp;
  Serial.println("Func1 Started");
  while (Break_Func1)
  {
    delay(100);
    lcd.clear();

    if (i == 0) {
      lcd.setCursor(3, 0);
      lcd.print("ENTER NEW MPSW");
    }
    else if (i == 1) {
      lcd.setCursor(0, 0);
      lcd.print("ENTER NEW MPSW Again");
    }

    lcd.setCursor(0, 1);
    lcd.print(Input);
    Time_Func1 = millis();
    CustomKey_Function = customKeypad.getKey();
    if (CustomKey_Function) {
      if (CustomKey_Function == '/') {
        Serial.println(Input);
        Serial.println("func1 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;
      }
      else {
        Input = Input + CustomKey_Function ;
        Serial.println(Input);
        Serial.println("func1 in");
        Time_Temp = millis();
        Time_Func1 = Time_Temp;
        Time_Func2 = Time_Temp;

      }
    }
    if (CustomKey_Function == '/') {
      i++ ;
      NEW_MPSW[i - 1] = Input;
      Input = "";
    }
    if (i == 2) {
      if (NEW_MPSW[0] == NEW_MPSW[1]) {
        MPSW = NEW_MPSW[0];
        Serial.println("Pass Changed");
        Input = "";
        Break_Func1 = LOW ;
        EEPROM_Write_Password(MPSW);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("MPSW Changed");
        delay(3000);
      }
      else  {
        Break_Func1 = LOW ;
        Serial.println("Pass can not Change");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MPSW Didn't Changed");
        delay(3000);
      }
    }
    if (Time_Func1 - Time_Func2 >= delay_time)
    {
      Input = "";
      Break_Func1 = LOW ;
    }
  }
  NEW_MPSW[0, 1] = "";
  i = 0;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////Write data to EEPROM////////////////////////////////////////////
void EEPROM_Write(void) {
  Serial.println("Write to EEPROM");
  for (int j = 0; j < No_Users; j++ )
  {

    EEPROM.write(j, Credit[j]);
  }

  for ( int c = 0 ; c < No_Users; c++ ) {
    for (int t = 0; t < 5; t++) {

      EEPROM.write(No_Users + (c * 5) + t, C_No[c][t]);
    }
  }

  for ( int p = 0; p < No_Users; p++ )
  {
    for (int h = 0; h < 2; h++) {

      EEPROM.write(60 + (p * 2) + h, UPSW[p][h]);
    }

  }

  for ( int k = 0; k < No_Users; k++ )
  {
    EEPROM.write(8 * No_Users + k, Iner_Meth[k]);
  }
}
////////////////////Write PassWord to EEPROM////////////////////
void EEPROM_Write_Password(String MPSW) {
  EEPROM.write(9 * No_Users, MPSW.toInt() / 255);
  EEPROM.write(9 * No_Users + 1, MPSW.toInt() % 255);
  if (MPSW == "1111") {
    EEPROM.write(9 * No_Users + 2, 0);
  }
  else EEPROM.write(9 * No_Users + 2, 111);
}
////////////////////////////Reset All To The Factory Mode/////////////////////////////
void Reset_All(void) {
  for (int i = 0; i <= No_Users; i++)
  {
    Credit[i] = 0;
    Iner_Meth[i] = 0 ;
    for (int j = 0; j <= 4; j++) {
      C_No[i][j] = 0;
    }
    for (int h = 0; h <= 2; h++) {
      UPSW[i][h] = 0;
    }
  }
  MPSW = "1111";
  EEPROM_Write_Password("1111");
  EEPROM_Write();
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("All Reset");
  Serial.println("All Reset");

}
/////////////////////////////////////////////////
void Open_Door(void)
{
  digitalWrite(GREEN ,LOW);
  digitalWrite(BUZZER ,LOW);
  delay(2000);
   digitalWrite(GREEN ,HIGH);
  digitalWrite(BUZZER ,HIGH);
}
/////////////////////////////////////////////////
void Error (void)
{
   digitalWrite(RED ,HIGH);
  digitalWrite(BUZZER ,HIGH);
  delay(500);
   digitalWrite(RED ,LOW);
  digitalWrite(BUZZER ,LOW);
  delay(200);
  digitalWrite(RED ,HIGH);
  digitalWrite(BUZZER ,HIGH);
  delay(500);
   digitalWrite(RED ,LOW);
  digitalWrite(BUZZER ,LOW);  
}

