#include <Arduino.h>
#include <Base64.h>
#include <EtherCard.h>
#include <EEPROM.h>
#include <BH1750.h>
#include <Wire.h>

/*
* Функция checkSensor()
* Принимает сигналы с датчиков и возращает истинну, если хотя бы один датчик сработал 
*/

bool checkSensor();

/*
* Функция resetNetwork()
* Загружает в EEPROM сетевые реквизиты по умолчанию
*/

void resetNetwork();

/*
* Функция setupNetwork()
* Устанавливает сетевые реквизиты, полученных из EEPROM
*/

void setupNetwork();

/*
* Функция encodeBase64(char *text)
* char *text - текст, который нужно закодировать
* Возвращает закодированную в Base64 строку String без учитывания символа \0
*/

String encodeBase64(String text);

/*
* Функция void authHandler(String log, String pass)
* Сравнивает login и password с дефолтными login и password
*   *Если login и password соотвуствуют (авторизация произошла успешно
*                                         запускает страницу controlPage
*/

void authHandler(char *request);

/*
* Функция void requestHandler(char* request)
* Определяет какой запрос пришел (GET или POST);
* Передает значение request соотвествующей функции (в зависимости от типа запроса);
*/

void requestHandler(char* request);

/*
* Функция void getHandler(char* request)
* Обрабатывает GET запросы:
*   * Выход из controlPage и переход в httpUnauthorized
*   * Включение\Выключение реле
*/

void getHandler(char* request);

/*
* Функция void postHandler
* Обрабатывает POST запросы:
*   * Получение сетевых параметров
*/

void postHandler(char * request);