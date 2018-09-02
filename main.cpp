#include "main.h"

#define DEBUG false
#define DEBUG_EEPROM true
#define DEBUG_NETWORK false

static byte mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x32};

byte Ethernet::buffer[800];

/*
* Последняя отправка сработки датчика движения
*/

long previousMillis;

/*
* Время включения питания реле (как долго будет светиться свет)
* Указывается в мсек
*/

bool releState;

/*
* Заполнитель буфера
*/

BufferFiller bfill;

uint16_t lux;
BH1750 lightMeter;
String authHash;
String ip;
String gtw;
String dns;
String net;
int del;
int sens;

void (*resetFunc)(void) = 0;
static word controlPage();
static word resetPage();
static word http_Found();
static word httpNotFound();
static word httpUnauthorized();
static word arduinoSettings();
static word loginPage();

void setup()
{
  releState = false;
  Wire.begin();
  lightMeter.begin();
  pinMode(4, INPUT);
  // pinMode(5, INPUT);
  pinMode(3, OUTPUT);
  pinMode(2, INPUT);
#if DEBUG || DEBUG_EEPROM || DEBUG_NETWORK
  Serial.begin(9600);
  Serial.println("Ready or Crashed");
#endif
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
  {
    Serial.println(F("Failed to access Ethernet controller"));
  }

  for (int i = 0; i < 15; i++)
  {
    if (EEPROM.read(i) != 0)
    {
      ip += (char)EEPROM.read(i);
    }
  }

  for (int i = 0; i < 15; i++)
  {
    if (EEPROM.read(i + 15) != 0)
    {
      gtw += (char)EEPROM.read(i + 15);
    }
  }

  for (int i = 0; i < 15; i++)
  {
    if (EEPROM.read(i + 30) != 0)
    {
      dns += (char)EEPROM.read(i + 30);
    }
  }

  for (int i = 0; i < 15; i++)
  {
    if (EEPROM.read(i + 45) != 0)
    {
      net += (char)EEPROM.read(i + 45);
    }
  }
  String temp;
  for (int i = 0; i < 5; i++)
  {
    if (EEPROM.read(i + 74) != 0)
    {
      temp += (char)EEPROM.read(i + 74);
    }
  }

  sens = temp.toInt();
  temp = "";
  for (int i = 0; i < 5; i++)
  {
    if (EEPROM.read(i + 80) != 0)
    {
      temp += (char)EEPROM.read(i + 80);
    }
  }

  del = temp.toInt();
#if DEBUG_EEPROM
  Serial.print(F("\nIp: "));
  Serial.print(ip);
  Serial.print(F("\nGtw: "));
  Serial.print(gtw);
  Serial.print(F("\nDns: "));
  Serial.print(dns);
  Serial.print(F("\nNet: "));
  Serial.print(net);
  Serial.print(F("\nSens: "));
  Serial.print(sens);
  Serial.print(F("\nDelay: "));
  Serial.print(del);
  Serial.println();
#endif
  for (int i = 0; i < 6; i++)
  {
    if (EEPROM.read(61 + i) != 0)
    {
      authHash += (char)EEPROM.read(i + 61);
    }
    else
      break;
  }
  authHash += ":";
  for (int i = 0; i < 6; i++)
  {
    if (EEPROM.read(67 + i) != 0)
    {
      authHash += (char)EEPROM.read(i + 67);
    }
    else
      break;
  }
#if DEBUG_EEPROM
  Serial.println(authHash);
#endif
  authHash = encodeBase64(authHash);
#if DEBUG_EEPROM
  for (int i = 0; i < 100; i++)
  {
    Serial.print((char)EEPROM.read(i));
  }
#endif
  setupNetwork();
}

void loop()
{
  #if DEBUG
  if ((millis()*1000) % 5)
  {
    if (!releState) Serial.println("ReleState is OFF");
  }
  #endif
  word pos = ether.packetLoop(ether.packetReceive());
  if ((millis() % 10000) == 0)
  {
    lux = lightMeter.readLightLevel();
  }

  if (digitalRead(2) == HIGH)
  {
    resetNetwork();
  }
  // Если хотя бы один датчик сработал -> присваеваем значение последней отправки значение время работы ардуино.
  if (checkSensor())
  {
    previousMillis = millis();
  }

  // Секундомер
  if (lux <= sens)
  {
    if ((millis() - previousMillis <= (del * 1000 * 60)) && releState)
    {
      digitalWrite(3, 1);
    }
    else
    {
      digitalWrite(3, 0);
    }
  } else {
    digitalWrite(3,0);
  } 

  // Если пришел запрос - начинаем обрабатывать его
  if (pos)
  {
    bfill = ether.tcpOffset();
#if DEBUG
    Serial.println((char *)Ethernet::buffer + pos);
#endif
    requestHandler((char *)Ethernet::buffer + pos);
  }
}

void setupNetwork()
{
  static uint8_t ipS[4];
  static uint8_t netmaskS[4];
  static uint8_t gtwS[4];
  static uint8_t dnsS[4];
  char *cstr1 = new char[ip.length() + 1];
  strcpy(cstr1, ip.c_str());
  ether.parseIp(ipS, cstr1);
  char *cstr2 = new char[gtw.length() + 1];
  strcpy(cstr2, gtw.c_str());
  ether.parseIp(gtwS, cstr2);
  char *cstr3 = new char[dns.length() + 1];
  strcpy(cstr3, dns.c_str());
  ether.parseIp(dnsS, cstr3);
  char *cstr4 = new char[net.length() + 1];
  strcpy(cstr4, net.c_str());
  ether.parseIp(netmaskS, cstr4);

  ether.staticSetup(ipS, gtwS, dnsS, netmaskS);
#if DEBUG_NETWORK
  Serial.println(F("----------------CONFIG----------------"));
  ether.printIp("Ip: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW Ip:", ether.gwip);
  ether.printIp("DNS Ip:", ether.dnsip);
  Serial.println(F("--------------------------------------"));
#endif
  delete[] cstr1;
  delete[] cstr2;
  delete[] cstr3;
  delete[] cstr4;
}

void resetNetwork()
{
  for (int i = 0; i < 1024; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.write(0, '1');
  EEPROM.write(1, '9');
  EEPROM.write(2, '2');
  EEPROM.write(3, '.');
  EEPROM.write(4, '1');
  EEPROM.write(5, '6');
  EEPROM.write(6, '8');
  EEPROM.write(7, '.');
  EEPROM.write(8, '1');
  EEPROM.write(9, '.');
  EEPROM.write(10, '1');
  EEPROM.write(11, '3');

  EEPROM.write(61, '1');
  EEPROM.write(62, '2');
  EEPROM.write(63, '3');
  EEPROM.write(64, 0);
  EEPROM.write(65, 0);
  EEPROM.write(66, 0);
  EEPROM.write(67, '1');
  EEPROM.write(68, '2');
  EEPROM.write(69, '3');
  resetFunc();
}

String encodeBase64(String text)
{

  char temp[text.length()];
  strcpy(temp, text.c_str());
  int encodedLength = Base64.encodedLength(sizeof(temp));
  char encodedString[encodedLength];
  Base64.encode(encodedString, temp, sizeof(temp));
  return String(encodedString);
}

void authHandler(char *request)
{
#if DEBUG
  Serial.print(F("\n Authorization: "));
  Serial.print(authHash);
  Serial.println(F(""));
#endif
  if (strstr(request, authHash.c_str()) != NULL)
  {
    ether.httpServerReply(controlPage());
  }
  else
    ether.httpServerReply(httpUnauthorized());
}

void postHandler(char *request)
{
  if (strstr(request, "activate") != NULL)
  {
    digitalWrite(3, 1);
    releState = true;
  }
  if (strstr(request, "disactivate") != NULL)
  {
    digitalWrite(3, 0);
    releState = false;
  }
  if (strlen(strstr(request, "sens=")) > 12)
  {
    for (int i = 74; i < 100; i++)
    {
      EEPROM.write(i, 0);
    }
    char *str = strstr(request, "sens=");
    int i = 0, k = 0, g = 0;
    char substr[5];
    while (str[i] != '\0')
    {
      if (str[i] != '=')
      {
        i++;
        continue;
      }
      i++;
      while (str[i] != '&' && str[i] != '\0')
      {
        substr[k] = str[i];
        i++;
        k++;
      }
      substr[k] = '\0';
      switch (g)
      {
      case 0:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 5; i++)
          {
            if (isdigit(substr[i]))
            {
              EEPROM.write(i + 74, substr[i]);
            }
            else
              break;
          }
          break;
        }
      }
      case 1:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 5; i++)
          {
            if (isdigit(substr[i]))
            {
              EEPROM.write(i + 80, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      default:
      {
        Serial.println("Error");
      }
      }
      k = 0;
      g++;
      substr[k] = '\0';
    }
  }
  if (strlen(strstr(request, "log=")) > 9)
  {
    for (int i = 61; i < 73; i++)
    {
      EEPROM.write(i, 0);
    }
    char *str = strstr(request, "log=");
    int i = 0, k = 0, g = 0;
    char substr[6];
    while (str[i] != '\0')
    {
      if (str[i] != '=')
      {
        i++;
        continue;
      }
      i++;
      while (str[i] != '&' && str[i] != '\0')
      {
        substr[k] = str[i];
        i++;
        k++;
      }
      substr[k] = '\0';
      switch (g)
      {
      case 0:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 6; i++)
          {
            if (isalpha(substr[i]) || isdigit(substr[i]))
            {
              EEPROM.write(i + 61, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      case 1:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 6; i++)
          {
            if (isalpha(substr[i]) || isdigit(substr[i]))
            {
              EEPROM.write(i + 67, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      default:
        Serial.println("Error");
      }
      k = 0;
      g++;
      substr[k] = '\0';
    }
    // resetFunc();
  }
  if (strlen(strstr(request, "ip=")) > 28)
  {

    /*
    * Как убрать костыли
    * Динамический массив - тогда условия проверки на число и точку отпадут
    * Функция - тогда отпадет повторяющий код (в функции указываем начало записи (0-ip,15-gtw,30-dns,45-netmask) в EEPROM)
    * Но это потом, я хочу спать)
    */

    for (int i = 0; i < 60; i++)
    {
      EEPROM.write(i, 0);
    }
    char *str = strstr(request, "ip=");
    int i = 0, k = 0, g = 0;
    char substr[6];
    while (str[i] != '\0')
    {
      if (str[i] != '=')
      {
        i++;
        continue;
      }
      i++;
      while (str[i] != '&' && str[i] != '\0')
      {
        substr[k] = str[i];
        i++;
        k++;
      }
      substr[k] = '\0';
      switch (g)
      {
      case 0:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 15; i++)
          {
            if (isdigit(substr[i]) || substr[i] == '.')
            {
              EEPROM.write(i, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      case 1:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 15; i++)
          {
            if (isdigit(substr[i]) || substr[i] == '.')
            {
              EEPROM.write(i + 15, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      case 2:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 15; i++)
          {
            if (isdigit(substr[i]) || substr[i] == '.')
            {
              EEPROM.write(i + 30, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      case 3:
      {
        if (substr[0] != 0)
        {
          for (int i = 0; i < 15; i++)
          {
            if (isdigit(substr[i]) || substr[i] == '.')
            {
              EEPROM.write(i + 45, substr[i]);
            }
            else
              break;
          }
        }
        break;
      }
      default:
        Serial.println("Error");
      }
      k = 0;
      g++;
      substr[k] = '\0';
    }
    // resetFunc();
  }
  if (strstr(request, "logsettings") != NULL)
  {
    ether.httpServerReply(loginPage());
  }
  if (strstr(request, "netsettings") != NULL)
  {
    ether.httpServerReply(resetPage());
  }
  if (strstr(request, "ardsettings") != NULL)
  {
    ether.httpServerReply(arduinoSettings());
  }
  if (strstr(request, authHash.c_str()) != NULL)
  {
    authHandler(request);
  }
  if (strstr(request, "restart") != NULL)
  {
    resetFunc();
  }
}

void getHandler(char *request)
{
  if (strstr(request, authHash.c_str()) != NULL)
  {
    ether.httpServerReply(controlPage());
  }
  else
    authHandler(request);
}

/*
* Если найдено совпадение с запросом GET, то передаем управление функции getHandler(char* request)
* Тоже самое и с запросом POST
* Если нет ниодного совпадения - отображаем страницу 404 Not Found
*/

void requestHandler(char *request)
{
  if (strstr(request, "GET /") != NULL)
  {
    getHandler(request);
  }
  else if (strstr(request, "POST /") != NULL)
  {
    postHandler(request);
  }
  else
  {
    ether.httpServerReply(httpNotFound());
  }
}

static word arduinoSettings()
{
  bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<center>"
      "<form method='post'>"
      "<p> <em> Sensitivity light Sensor (lux) </em> </p>"
      "<p> <input type = 'number' name = 'sens'> </p>"
      "<p> <em> Timer (ms)</em> </p>"
      "<p> <input type = 'number' name = 'timer'> </p>"
      "<p> <input type = 'submit' value = 'Submit'> </p>"
      "</form>"
      "</center>"

      ));
  return bfill.position();
}

static word httpNotFound()
{
  bfill.emit_p(PSTR(
      "HTTP/1.0 404 Not Found"));
  return bfill.position();
}

static word resetPage()
{
  bfill.emit_p(PSTR(
      "HTTP/1.0 202 Accepted\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<title> Reset Page </title>"
      "<center>"
      "<form method = 'post'>"
      "<p> <em> IP address: </em> </p>"
      "<p> <input type = 'text' name = 'ip' size = 20> </p>"
      "<p> <em> Gateway address: </em> </p>"
      "<input type = 'text' name = 'gat' size = 20>"
      "<p> <em> DNS address: </em> </p>"
      "<input type = 'text' name = 'dns' size = 20>"
      "<p> <em> Subnet mask </em> </p>"
      "<p> <input type = 'text' name = 'sub' size = 20>"
      "<p> <input type = 'submit' value = 'Submit'> </p>"
      "</center>"

      ));
  return bfill.position();
}

static word httpUnauthorized()
{
  bfill.emit_p(PSTR(
      "HTTP/1.0 401 Unauthorized\r\n"
      "WWW-Authenticate: Basic realm=\"Arduino\""
      "Content-Type: text/html\r\n\r\n"));
  return bfill.position();
}

static word controlPage()
{
  bfill.emit_p(PSTR(
                   "HTTP/1.0 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Pragma: no-cache\r\n"
                   "\r\n"
                   "<meta http-equiv='refresh' content='1'>"
                   "<title> Control Page </title>"
                   "<center>"
                   "<p> Light: $D </p>"
                   "<p> RELE: $S </p>"
                   "<p> Sensor: $S </p>"
                   "<form method='post'>"
                   "<p> <input type = 'submit' value = 'Activate rele' name = 'activate'> </p>"
                   "<p> <input type = 'submit' value = 'Disactivate rele' name = 'disactivate'> </p>"
                   "<p> <input type = 'submit' value = 'LOGIN SETTINGS' name= 'logsettings'> </p>"
                   "<p> <input type = 'submit' value = 'NETWORK SETTINGS' name= 'netsettings'> </p>"
                   "<p> <input type = 'submit' value = 'ARDUINO SETTINGS' name= 'ardsettings' </p>"
                   "<p> <input type = 'submit' value = 'RESTART' name= 'restart'> </p>"
                   "</form>"
                   "<p> <a href='http://log:out@$S/'> EXIT </p>"
                   "</center>"),
               lux, releState ? "ON" : "OFF", checkSensor() ? "ON" : "OFF", ip.c_str());
  return bfill.position();
}

static word loginPage()
{
  bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<center>"
      "<form method = 'post'>"
      "<p> Login </p>"
      "<p> <input type = 'text' name = 'log'> </p>"
      "<p> Password </p>"
      "<p> <input type = 'text' name = 'pas'> </p>"
      "<p> <input type = 'submit' value = 'Submit'> </p>"
      "</form>"
      "</center>"));
  return bfill.position();
}
// Возращает true, если хотя бы один из датчиков сработал.
bool checkSensor()
{
  if (digitalRead(4) == HIGH || digitalRead(5) == HIGH)
  {
    return true;
  }
  else
    return false;
}