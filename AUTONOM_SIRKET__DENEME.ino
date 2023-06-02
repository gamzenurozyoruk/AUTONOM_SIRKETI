#include <stdio.h>
#include <string.h>

// CRC polinomu
#define CRC_POLYNOM 0x8408

// LED pin ve durum bilgileri
#define LED_PIN 13
int ledState = 0;
unsigned long previousMillis = 0;
unsigned long onInterval = 1000;
unsigned long offInterval = 1000;

// Yeniden iletim için parametreler
int maxRetries = 3; // Maksimum yeniden iletim sayısı
int retryCount = 0; // Mevcut yeniden iletim sayısı


// UART ayarları
#define BAUD_RATE 115200
#define DATA_BITS 8
#define PARITY 'N'
#define STOP_BITS 1

// UART veri alımı için buffer
#define BUFFER_SIZE 64
char rxBuffer[BUFFER_SIZE];
int rxIndex = 0;
bool newData = false;

// Echo taskının durumu
bool echoTaskRunning = true;

// UART veri gönderme fonksiyonu
void sendData(const char* data)
{
  // CRC hesapla
    unsigned short crc = 0xFFFF;
    for (int i = 0; i < strlen(data); i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ CRC_POLYNOM;
            else
                crc >>= 1;
        }

        // CRC'yi veriye ekle
    char crcStr[6];
    sprintf(crcStr, "%04X", crc);
    strcat(data, crcStr);
    //veriyi gönder
    Serial.println(data);
}

// Echo taskını kontrol eden fonksiyon
void echoTask()
{
  if (newData)
    {  
    // Verinin CRC kontrolünü yap
        unsigned short receivedCrc = strtol(rxBuffer + strlen(rxBuffer) - 4, NULL, 16);
        char dataCopy[BUFFER_SIZE];
        strncpy(dataCopy, rxBuffer, strlen(rxBuffer) - 4);
        dataCopy[strlen(rxBuffer) - 4] = '\0'; 
        unsigned short calculatedCrc = 0xFFFF;
        for (int i = 0; i < strlen(dataCopy); i++)
        {
            calculatedCrc ^= dataCopy[i];
            for (int j = 0; j < 8; j++)
            {
                if (calculatedCrc & 0x0001)
                { 
                  calculatedCrc = (calculatedCrc >> 1) ^ CRC_POLYNOM;
                }
                    
                else 
                {
                  calculatedCrc >>= 1;
                }
                    
            }
        }
    
        // İletişim hatası durumunda yeniden iletim
       if (retryCount > 0)
       {
            sendData(rxBuffer);
            retryCount--;
       }
        else
        {   
        // CRC doğrulaması yap
        if (receivedCrc == calculatedCrc){
            if (strcmp(dataCopy, "stop") == 0)
            {
                echoTaskRunning = false;
                toggleLED();
            }
            else if (strcmp(dataCopy, "start") == 0)
            {
                echoTaskRunning = true;
            }
            else if (strncmp(dataCopy, "ledon=", 6) == 0)
            {
                unsigned long newOnInterval = atol(dataCopy + 6);
                if (newOnInterval > 0)
                {
                    onInterval = newOnInterval;
                }
            }
            else if (strncmp(rxBuffer, "ledoff=", 7) == 0)
            {
                unsigned long newOffInterval = atol(dataCopy + 7);
                if (newOffInterval > 0)
                {
                    offInterval = newOffInterval;
                }
            }
            else
            {
                sendData(dataCopy);
                sendData("Gecersiz Komut!");
            }
            }
            else 
            {
             // CRC hatası
            Serial.println("CRC error!"); 
            }
            }

            newData = false;
        }

    }
        
}

// LED taskını kontrol eden fonksiyon
void ledTask()
{
    unsigned long currentMillis = millis();

    if (ledState == HIGH && currentMillis - previousMillis >= onInterval)
    {
        toggleLED();
    }
    else if (ledState == LOW && currentMillis - previousMillis >= offInterval)
    {
        toggleLED();
    }
}

// LED'in durumunu değiştiren fonksiyon
void toggleLED()
{
    if (ledState == LOW)
    {
        ledState = HIGH;
        onInterval = 500;  // Varsayılan LED on süresi (ms)
        digitalWrite(LED_PIN, ledState);
        previousMillis = millis();
    }
    else
    {
        ledState = LOW;
        offInterval = 500;  // Varsayılan LED off süresi (ms)
        digitalWrite(LED_PIN, ledState);
        previousMillis = millis();
    }
}

// UART veri alımını işleyen kesme (interrupt) fonksiyonu
void serialEvent()
{
    
    while (Serial.available())
    {
        char receivedChar = Serial.read();
        

        if (receivedChar == '\n')
        {
            receivedChar.toLowerCase();
            rxBuffer[rxIndex] = '\0';
            rxIndex = 0;
            newData = true;
        }
        else
        {
            receivedChar.toLowerCase();
            rxBuffer[rxIndex] = receivedChar;
            rxIndex++;
            if (rxIndex >= BUFFER_SIZE)
            {
                rxIndex = BUFFER_SIZE - 1;
            }
        }
    }
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(BAUD_RATE);
}

void loop()
{
    if (echoTaskRunning)
    {
        echoTask();
    }

    ledTask();
}
