# TransportCatalogue
Транспортный справочник.

- Принимает на вход данные JSON-формата и выдает ответ в виде SVG-файла, визуализирующего остановки и маршруты.
- Находит кратчайший маршрут между остановками.
- Для ускорения вычислений сделана сериализация базы справочника через Google Protobuf.
- Реализован конструктор JSON, позволяющий находить неправильную последовательность методов на этапе компиляции.
