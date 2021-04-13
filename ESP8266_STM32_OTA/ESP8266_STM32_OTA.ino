#include <SoftwareSerial.h>
#include <SD.h>

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



const char* ssid     = "redmi_4xxx";
const char* password = "87654321";

//http://cc33953.tmweb.ru/other/STM32.bin

String fileURL = "http://cc33953.tmweb.ru/other/STM32.bin";
String fileName = "STM32.bin";

HTTPClient Http;
File uploadFile;



#define BOOT_BAUD 57600
//#define DEBUG_BAUD 9600

//#define txPin 10 // GPIO10
//#define rxPin 9 // GPIO9

#define BOOT_PIN D3 // GPIO0
#define RESET_PIN D4 // GPIO2

//SoftwareSerial sSerial;

File myFile;

const int chipSelect = D8; // GPIO15

#define USART_STM32 Serial
//#define USART_DEBUG sSerial


#define WRITE_ADDR 0x08000000 // адрес для заливки прошивки
#define SIZE_WRITE 256
#define FIRMWARE "STM32.bin"



// Декларация для дисплея SSD1306, подключенного к I2C (контакты SDA, SCL)
Adafruit_SSD1306 display(128, 64, &Wire, LED_BUILTIN);

#define dlOLED 1000 // Время задержки OLED сообщений

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };




void setup() {

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Работа с 128x64 OLED дисплеем по адресу 0x3C
  display.display(); // Устанавливаем изображение
  delay(2000); // Задержка

  displayOLED ("Старт", 0, 0, 1000, 1);
  

//  USART_DEBUG.begin(DEBUG_BAUD, SWSERIAL_8N1, rxPin, txPin, false, 95, 11);
  USART_STM32.begin(BOOT_BAUD, SERIAL_8E1);

  
//  pinMode(rxPin, INPUT);
//  pinMode(txPin, OUTPUT);

  pinMode(chipSelect,OUTPUT);
  
  pinMode(BOOT_PIN, OUTPUT);
  digitalWrite(BOOT_PIN, LOW);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW); 



  // Соединение с сетью
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
//    USART_DEBUG.println("Connecting..");
    displayOLED("", 0, 0, 100, 1);
    displayOLED("Подключение к сети..", 0, 0, 100, 1);
  }
//  USART_DEBUG.println();
  displayOLED("Подключение к сети\nуспешно", 0, 0, dlOLED, 1);



  // Соединение с SD картой
  if (!SD.begin(chipSelect)) {
//    USART_DEBUG.println("Card failed, or not present");
    displayOLED("Ошибка подключения\nSD карты", 0, 0, dlOLED, 1);
    return;
  }
//  USART_DEBUG.println("Card initialized.");
  displayOLED("SD карта\nинициализирована", 0, 0, dlOLED, 1);



  displayOLED("Функция загрузки\nпрошивки", 0, 0, dlOLED, 1);
  if(!downloadFile()){
    return;
  }
  displayOLED("Функция загрузки\nпрошивки завершена", 0, 0, dlOLED, 1);



  displayOLED("Функция установки\nпрошивки", 0, 0, dlOLED, 1);
  programSTM32();
  displayOLED("Функция установки\nпрошивки завершена", 0, 0, dlOLED, 1);
}





void loop() {
  
}




int downloadFile() {
  
  int err = 0;
  
  // Открываем соединение
  Http.begin(fileURL);
  displayOLED("Открываем соединение", 0, 0, dlOLED, 1);
  
  // Код ответа
  int httpCode = Http.GET();
  displayOLED("Код ответа: " + String(httpCode), 0, 0, dlOLED, 1);
  
  if (httpCode > 0) {
  
    // Файл найден на сервере
    if (httpCode == HTTP_CODE_OK) {
      
      displayOLED("Файл найден на\nсервере", 0, 0, dlOLED, 1);

      // Удалить файл:
      SD.remove(fileName);
      displayOLED("Удаляем " + fileName + "\nс SD карты", 0, 0, dlOLED, 1);

       // Открываем в режиме записи
      uploadFile = SD.open(fileName, FILE_WRITE);
      displayOLED("Подготовка к загрузке\nновой прошивки", 0, 0, dlOLED, 1);

       // Получить длину документа
      int len = Http.getSize();
      displayOLED("Размер документа:\n" + String(len) + " byte", 0, 0, dlOLED, 1);

       // Создать буфер для чтения
      uint8_t buff[2048] = {0};
      
      // Получить TCP-поток
      WiFiClient *stream = Http.getStreamPtr();
    
      // Считывание всех данных с сервера
      while (Http.connected() && (len > 0 || len == -1)) {

        // Получить доступный размер данных
        size_t size = stream -> available();
        
        // Чтение до 128 байт
        if (size) { 
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

           // Запишем в файл
          uploadFile.write(buff, c);
          
          if (len > 0) len -= c;
        }
        delayMicroseconds(1);
      }
      
      displayOLED("Загрузка завершена", 0, 0, dlOLED, 1);  
      err = 1;
    }
    
    else {
      displayOLED("Ошибка соединения\nКод: " + String(httpCode), 0, 0, dlOLED, 1);
      err = 0;
    }
  }
  
  Http.end();
  displayOLED("Закрываем соединение: ", 0, 0, dlOLED, 1);
  
  uploadFile.close();
  displayOLED("Закрываем Файл: ", 0, 0, dlOLED, 1);

  if(err == 0){
    displayOLED("Ошибка соединения\nКод: " + String(httpCode), 0, 0, dlOLED, 1);
  }
  
  return err;
}






void programSTM32() {
  entr_bootloader();
  write_memory();
}



//////////////////////////// entr_bootloader /////////////////////////////////////
void entr_bootloader()
{
  on_off_boot(HIGH); // подтягиваем BOOT_0 к плюсу
  delay(500);
  on_reset(); // нажимаем ресет
  delay(200);

  while(USART_STM32.available()) { USART_STM32.read(); } // в приёмном буфере может быть мусор

  USART_STM32.write(0x7F); // первый запрос (для определения скорости) 
  if(ack_byte() == 0) {
//    USART_DEBUG.println(F("Bootloader - OK"));  // Log
    displayOLED ("Bootloader\n - OK", 0, 0, 1000, 1);
  }
  else {
//    USART_DEBUG.println(F("Bootloader - ERROR"));  // Log  
    displayOLED ("Bootloader\n - ERROR", 0, 0, 1000, 1);
  }
}

//////////////////////////// write_memory /////////////////////////////////////
void write_memory() {
  if(erase_memory() == 0) {
     File df = SD.open(FIRMWARE);
     
     if(df) {
       uint32_t size_file = df.size();
      
//       USART_DEBUG.print(F("Size file "));
//       USART_DEBUG.println(size_file);
       displayOLED ("Size file " + String(size_file), 0, 0, 1000, 1);
     
       uint8_t cmd_array[2] = {0x31, 0xCE}; // код Write Memory 
       uint32_t count_addr = 0;
       uint16_t len = 0;
       uint32_t seek_len = 0;

       while(true) {      
         if(send_cmd(cmd_array) == 0) {
           uint8_t ret_adr = send_adress(WRITE_ADDR + count_addr);
           count_addr = count_addr + SIZE_WRITE;
           
           if(ret_adr == 0) {       
             uint8_t write_buff[SIZE_WRITE] = {0,};
             len = df.read(write_buff, SIZE_WRITE);
             seek_len++;
             df.seek(SIZE_WRITE * seek_len);  
             //write_memory(write_buff, len);file.position()

             //USART_DEBUG.print(F("seek_len "));
             //USART_DEBUG.println(seek_len);
             //USART_DEBUG.println(df.position());
              
             uint8_t cs, buf[SIZE_WRITE + 2];
             uint16_t i, aligned_len;
              
             aligned_len = (len + 3) & ~3;
             cs = aligned_len - 1;
             buf[0] = aligned_len - 1;
              
             for(i = 0; i < len; i++) {
               cs ^= write_buff[i];
               buf[i + 1] = write_buff[i];
             }
              
             for(i = len; i < aligned_len; i++) {
               cs ^= 0xFF;
               buf[i + 1] = 0xFF;
             }
              
             buf[aligned_len + 1] = cs;
             USART_STM32.write(buf, aligned_len + 2);
             uint8_t ab = ack_byte();
          
             if(ab != 0) {
//               USART_DEBUG.println(F("Block not Write Memory - ERROR"));
               displayOLED ("Block not Write Memory\n - ERROR", 0, 0, 1000, 1);
               break;
             }
             
             if(size_file == df.position()) {
//               USART_DEBUG.println(F("End Write Memory2 - OK"));
               displayOLED ("End Write Memory2\n - OK", 0, 0, 1000, 1);
               boot_off_and_reset();
               break;
             }                 
           }
           else {
//             USART_DEBUG.println(F("Address Write Memory - ERROR")); // Log
               displayOLED ("Address Write Memory\n - ERROR", 0, 0, 1000, 1);
             break;
           }
         }
         else {
//           USART_DEBUG.println(F("Cmd cod Write Memory - ERROR")); // Log
           displayOLED ("Cmd cod Write Memory\n - ERROR", 0, 0, 1000, 1);
           break;
         }             
       } // end while
       
       df.close();
     }
     else {
//       USART_DEBUG.println(F("Not file - ERROR"));  
       displayOLED ("Not file\n - ERROR", 0, 0, 1000, 1);        
     }
  } 
  else {
//    USART_DEBUG.println(F("Not erase Write Memory - ERROR")); 
    displayOLED ("Not erase Write Memory\n - ERROR", 0, 0, 1000, 1);    
  }
}

////////////////////////////// erase_memory ////////////////////////////////////
uint8_t erase_memory() {
  uint8_t cmd_array[2] = {0x43, 0xBC}; // команда на стирание 

  if(send_cmd(cmd_array) == 0) {
    uint8_t cmd_array[2] = {0xFF, 0x00}; // код стирания (полное) 

    if(send_cmd(cmd_array) == 0) {
//      USART_DEBUG.println(F("Erase Memory - OK")); // Log
      displayOLED ("Erase Memory\n - OK", 0, 0, 1000, 1); 
      return 0;
    } 
    else {
//      USART_DEBUG.println(F("Cmd cod Erase Memory - ERROR")); // Log     
      displayOLED ("Cmd cod Erase Memory\n - ERROR", 0, 0, 1000, 1);         
    }
  }
  else {
//    USART_DEBUG.println(F("Cmd start Erase Memory - ERROR")); // Log
      displayOLED ("Cmd start Erase Memory\n - ERROR", 0, 0, 1000, 1);  
  }

  return 1;
}

////////////////////////////// send_cmd ////////////////////////////////////
uint8_t send_cmd(uint8_t *cmd_array) {
  USART_STM32.write(cmd_array, 2); 
  if(ack_byte() == 0) {
    return 0;
  }
  else {
    return 1;
  }
}

/////////////////////////////// ack_byte ////////////////////////////////////
uint8_t ack_byte() { 
  for(uint16_t i = 0; i < 500; i++) {
    if(USART_STM32.available()) {
      uint8_t res = USART_STM32.read();
      if(res == 0x79) {
        return 0;         
      }
    } 
    delay(1);  
  }

  return 1;
}

///////////////////////////// send_adress ////////////////////////////////////
uint8_t send_adress(uint32_t addr) {
  uint8_t buf[5] = {0,};  
  buf[0] = addr >> 24;
  buf[1] = (addr >> 16) & 0xFF;
  buf[2] = (addr >> 8) & 0xFF;
  buf[3] = addr & 0xFF;
  buf[4] = buf[0] ^ buf[1] ^ buf[2] ^ buf[3];
  
  USART_STM32.write(buf, 5);
  if(ack_byte() == 0) {
    return 0;
  }
  else {
    return 1;  
  }
}

////////////////////////////// on_reset //////////////////////////////////////
void on_reset() {
  digitalWrite(RESET_PIN, HIGH); // reset
  delay(50);    
  digitalWrite(RESET_PIN, LOW); 
}

////////////////////////////// on_off_boot ///////////////////////////////////
void on_off_boot(uint8_t i) {
  digitalWrite(BOOT_PIN, i); 
}

//////////////////////////// boot_off_and_reset //////////////////////////////
void boot_off_and_reset() {
  on_off_boot(LOW);
  delay(500);
  on_reset(); 
//  USART_DEBUG.println(F("Boot off and reset")); // Log
  displayOLED ("Boot off and reset", 0, 0, 1000, 1);  
}

////////////////////////////// on_boot ///////////////////////////////////
void on_boot() {
  digitalWrite(BOOT_PIN, HIGH); 
}

////////////////////////////// off_boot ///////////////////////////////////
void off_boot() {
  digitalWrite(BOOT_PIN, LOW); 
}





// OLED работа с русским текстом 
String utf8rus(String source) {
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };
  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x2F;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB7; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x6F;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}



// Вывод сообщения на OLED дисплей
void displayOLED(String str, int x, int y, int delayOLED, int language) {
  
  if(x > 128 || x < 0) x = 0;
  if(y > 64 || y < 0) y = 0;
  
  display.clearDisplay(); // Чистим дисплей
  display.setTextSize(1); // Устанавливаем размер шрифта (размер пикселей)
  display.setTextColor(WHITE); // Устанавливаем цвет текста
  
  display.setCursor(x, y); // Устанавливаем курсор  
  
  // Вводим русский текст
  if(language == 1) {
    display.println(utf8rus(str));
  }
  
  // Вводим английский текст
  if(language == 0) {
    display.println(str);
  }

  display.display(); // Устанавливаем изображение
  delay(delayOLED); // Задержка
}
