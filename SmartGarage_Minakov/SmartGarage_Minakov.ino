#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#define PIN_RELAY 5 // Определяем пин, используемый для подключения реле
const char *ssid =  "TP-LINK_3122";  // Имя вайфай точки доступа
const char *pass =  "55930437"; // Пароль от точки доступа

const char *mqtt_server = "mqtt.by"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "The_Goofy"; // Логин от сервер
const char *mqtt_pass = "e3b1vvcj"; // Пароль от сервера

int rele;
String Otop_On = "Heating On / ";
String Otop_Off = "Heating Off / ";

OneWire ds(0); // Создаем объект OneWire для шины 1-Wire, с помощью которого будет осуществляться работа с датчиком


void callback(const MQTT::Publish& pub)     // Функция получения данных от сервера
{
    Serial.print(pub.topic());                // выводим в сериал порт название топика
    Serial.print(" => ");
    Serial.println(pub.payload_string());     // выводим в сериал порт значение полученных данных
    
    String payload = pub.payload_string();
    
    if(String(pub.topic()) == "/user/The_Goofy/room")    //  проверяем из нужного ли нам топика пришли данные 
    {
      rele = payload.toInt();
      if(rele == 0){
         digitalWrite(PIN_RELAY, LOW); // Отключаем реле
      }
      else{
        if(rele == 1){
          digitalWrite(PIN_RELAY, HIGH); // Включаем реле
        }
      }
    }
    
}



WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);

void setup() {
    Serial.begin(115200);

  pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле как выход
  digitalWrite(PIN_RELAY, LOW); // Выключаем реле

    
    delay(10);
}

void loop() {
    // подключаемся к wi-fi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Connecting to ");
        Serial.print(ssid);
        Serial.println("...");
        WiFi.begin(ssid, pass);
        
        if (WiFi.waitForConnectResult() != WL_CONNECTED)
            return;
        Serial.println("WiFi connected");
    }
    
    // подключаемся к MQTT серверу
    if (WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) {
            Serial.println("Connecting to MQTT server");
            if (client.connect(MQTT::Connect("arduinoClient2")
                                 .set_auth(mqtt_user, mqtt_pass))) {
                Serial.println("Connected to MQTT server");
                client.set_callback(callback);
                client.subscribe("/user/The_Goofy/room");                  // подписывааемся по топик с данными для светодиода
                client.subscribe("/user/The_Goofy/text_test");
            } else {
                Serial.println("Could not connect to MQTT server");   
            }
        }
        
        if (client.connected()){
            client.loop();
            TempSend();
        }
    }

    //Serial.print("ONOFF = ");
    //Serial.println(ONOFF);
} // конец основного цикла


// Функция отправки показаний с термодатчика
void TempSend(){
  // Определяем температуру от датчика DS18b20
  byte data[2]; // Место для значения температуры
  
  ds.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  ds.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  ds.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память
  
  delay(1000); // Микросхема измеряет температуру, а мы ждем.  
  
  ds.reset(); // Теперь готовимся получить значение измеренной температуры
  ds.write(0xCC); 
  ds.write(0xBE); // Просим передать нам значение регистров со значением температуры

  // Получаем и считываем ответ
  data[0] = ds.read(); // Читаем младший байт значения температуры
  data[1] = ds.read(); // А теперь старший

  // Формируем итоговое значение: 
  //    - сперва "склеиваем" значение, 
  //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
  float temperature =  ((data[1] << 8) | data[0]) * 0.0625;
  
  // Выводим полученное значение температуры в монитор порта
  //Serial.println(temperature);

 String textOn = Otop_On + temperature + " C";
 String textOff = Otop_Off + temperature + " C";

   if (rele == 0){
      //client.publish("/user/The_Goofy/text_test", String(textOff)); // отправляем в топик для термодатчика значение температуры
      client.publish("/user/The_Goofy/text_test", String(temperature)); // отправляем в топик для термодатчика значение температуры
   }
   if (rele == 1){
      //client.publish("/user/The_Goofy/text_test", String(textOn)); // отправляем в топик для термодатчика значение температуры
      client.publish("/user/The_Goofy/text_test", String(temperature)); // отправляем в топик для термодатчика значение температуры
   }

   delay(3000);  
}
