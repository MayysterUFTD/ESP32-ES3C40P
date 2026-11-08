# ESP32-ES3C40P

> **Status: publiczny projekt-szkielet w trakcie rozwoju.** To punkt wyjścia do dalszych prac, a nie gotowy produkt ani firmware do zastosowań produkcyjnych. Obsługa wyświetlacza i dotyku jest eksperymentalna i wymaga potwierdzenia na urządzeniu.

Projekt zawiera bazową konfigurację PlatformIO dla modułu LCDWIKI ES3C40P z ESP32-S3. Aktualny program uruchamia panel ST7796S, inicjalizuje dotyk FT6336U i tworzy prosty ekran demonstracyjny LVGL. Funkcje właściwej aplikacji będą rozwijane później.

## Sprzęt

| Element | Układ / interfejs | Piny ESP32-S3 |
|---|---|---|
| Wyświetlacz 320 × 480 | ST7796S, SPI | CS: GPIO10, DC: GPIO46, SCK: GPIO12, MOSI: GPIO11, MISO: GPIO13 |
| Podświetlenie | sterowanie GPIO | GPIO45 |
| Ekran dotykowy | FT6336U, I²C, adres `0x38` | SDA: GPIO16, SCL: GPIO15 |
| Reset i przerwanie dotyku | GPIO | RESET: GPIO18, INT: GPIO17 |

Reset wyświetlacza jest współdzielony z resetem modułu ESP32-S3. Sekwencja inicjalizacji ST7796S w kodzie bazuje na materiale LCDWIKI dla ES3C40P.

## Wymagania

- PlatformIO Core lub PlatformIO w VS Code
- ESP32-S3 ES3C40P z PSRAM
- Kabel USB obsługujący transmisję danych

Zależności LVGL i Arduino_GFX są pobierane przez PlatformIO zgodnie z `platformio.ini`.

## Budowanie, wgrywanie i monitor

Polecenia uruchom w katalogu projektu:

```sh
pio run -e es3c40p
pio run -e es3c40p -t upload
pio device monitor -e es3c40p
```

Monitor szeregowy skonfigurowany jest na `115200` baud. Jeśli PlatformIO nie wykryje właściwego portu, ustaw `upload_port` i `monitor_port` dla środowiska `es3c40p` w lokalnej konfiguracji.

## Rotacja

W konstruktorze `display` w `src/main.cpp` trzeci argument (po wskaźniku magistrali i pinie resetu) wybiera rotację: `0`, `1`, `2` lub `3`. Obrót o 90°/270° zmienia orientację i wymiary ekranu; przed użyciem takiej orientacji trzeba dopasować rozdzielczość LVGL, układ UI i współrzędne dotyku. Obecna konfiguracja wyświetlacza i UI nie jest automatycznie synchronizowana przy zmianie tego argumentu.

## Stan i ograniczenia

- Projekt jest szkieletem — demonstracyjny ekran LVGL nie stanowi jeszcze docelowego interfejsu aplikacji.
- Kompilacja środowiska `es3c40p` została sprawdzona, ale poprawne wyświetlanie obrazu na fizycznym module nie jest jeszcze potwierdzone.
- Obsługa dotyku i gestów pozostaje eksperymentalna; wymagane są dalsze testy na urządzeniu.
- Zmiana rotacji, mapowania dotyku i parametrów panelu wymaga testów sprzętowych.

## Dokumentacja sprzętu

- [Instrukcja użytkownika ES3C40P](https://www.lcdwiki.com/res/ES3C40P/4.0inch_IPS_ESP32-S3_ES3C40P_User_Manual.pdf)
- [Sekwencja inicjalizacji ST7796S dla ES3C40P](https://www.lcdwiki.com/res/ES3C40P/ST7796_Init.txt)
