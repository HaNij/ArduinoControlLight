# ArduinoControlLight
Arduino contol light with ENC28j60 ethernet module

В результате тестов, было определенно, что система работает корректно, но не стабильно, ввиду малого количества ОЗУ памяти в Arduino Nano (Atmega 328p) 2 КБ.
Чтобы решить эту проблему, нужно разделить Arduino и серверную часть, т.к в данный момент всё Arduino, который работает в роли сервера и digital signal handler (цифрового сигнального приемника).
Проект временно заброшен, т.к нет денег на Rasberry Pi, который будет в роли сервера.
