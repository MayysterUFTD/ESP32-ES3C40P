#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <initializer_list>
#include <Wire.h>
#include <lvgl.h>
#include <ui/ui.h>

namespace
{

    constexpr int kDisplayNativeWidth = 320;
    constexpr int kDisplayNativeHeight = 480;
    constexpr int kScreenWidth = 480;
    constexpr int kScreenHeight = 320;
    constexpr uint8_t kDisplayRotation = 1;
    constexpr int kBacklightPin = 45;
    constexpr int kDisplayChipSelectPin = 10;
    constexpr int kDisplayDataCommandPin = 46;
    constexpr int kDisplayClockPin = 12;
    constexpr int kDisplayMosiPin = 11;
    constexpr int kDisplayMisoPin = 13;
    constexpr int kTouchSdaPin = 16;
    constexpr int kTouchSclPin = 15;
    constexpr int kTouchResetPin = 18;
    constexpr int kTouchInterruptPin = 17;
    constexpr uint8_t kTouchAddress = 0x38;
    constexpr uint32_t kDisplaySpiFrequency = 20000000;

    class Es3c40pDisplay : public Arduino_ST7796
    {
    public:
        using Arduino_ST7796::Arduino_ST7796;

        void setRotation(uint8_t rotation) override
        {
            Arduino_ST7796::setRotation(rotation);
            static constexpr uint8_t madctlValues[] = {0x48, 0x28, 0xC8, 0xA8};
            writeRegister(0x36, madctlValues[rotation % 4]);
        }

    protected:
        void tftInit() override
        {
            _bus->sendCommand(0x01);
            delay(150);

            writeRegister(0xF0, 0xC3);
            writeRegister(0xF0, 0x96);
            writeRegister(0x36, 0x48);
            writeRegister(0x3A, 0x05);
            writeRegister(0xB0, 0x80);
            writeRegister(0xB6, {0x00, 0x02});
            writeRegister(0xB5, {0x02, 0x03, 0x00, 0x04});
            writeRegister(0xB1, {0x80, 0x10});
            writeRegister(0xB4, 0x00);
            writeRegister(0xB7, 0xC6);
            writeRegister(0xC5, 0x1C);
            writeRegister(0xE4, 0x31);
            writeRegister(0xE8, {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33});
            writeRegister(0xC2, 0xA7);
            writeCommand(0xA7);
            writeRegister(0xE0, {0xF0, 0x09, 0x13, 0x12, 0x12, 0x2B, 0x3C,
                                 0x44, 0x4B, 0x1B, 0x18, 0x17, 0x1D, 0x21});
            writeRegister(0xE1, {0xF0, 0x09, 0x13, 0x0C, 0x0D, 0x27, 0x3B,
                                 0x44, 0x4D, 0x0B, 0x17, 0x17, 0x1D, 0x21});
            writeCommand(0x21);
            writeRegister(0xF0, 0x3C);
            writeRegister(0xF0, 0x69);
            writeCommand(0x13);
            writeCommand(0x11);
            delay(120);
            writeCommand(0x29);
        }

    private:
        void writeCommand(uint8_t command)
        {
            _bus->sendCommand(command);
        }

        void writeRegister(uint8_t command, uint8_t value)
        {
            _bus->beginWrite();
            _bus->writeC8D8(command, value);
            _bus->endWrite();
        }

        void writeRegister(uint8_t command, std::initializer_list<uint8_t> values)
        {
            _bus->beginWrite();
            _bus->writeCommand(command);
            for (uint8_t value : values)
            {
                _bus->write(value);
            }
            _bus->endWrite();
        }
    };

    Arduino_DataBus *displayBus = new Arduino_ESP32SPI(
        kDisplayDataCommandPin,
        kDisplayChipSelectPin,
        kDisplayClockPin,
        kDisplayMosiPin,
        kDisplayMisoPin,
        FSPI);

    Es3c40pDisplay *display = new Es3c40pDisplay(
        displayBus,
        GFX_NOT_DEFINED,
        kDisplayRotation,
        true,
        kDisplayNativeWidth,
        kDisplayNativeHeight);

    alignas(LV_DRAW_BUF_ALIGN) uint16_t drawBuffer[kScreenWidth * 20];
    bool touchPressed = false;
    uint8_t touchReleaseSamples = 0;
    uint32_t lastTouchTime = 0;
    lv_point_t lastTouchPoint = {0, 0};

    bool readTouchRegister(uint8_t reg, uint8_t *value)
    {
        Wire.beginTransmission(kTouchAddress);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        if (Wire.requestFrom(kTouchAddress, 1) != 1)
        {
            while (Wire.available())
            {
                Wire.read();
            }
            return false;
        }

        *value = static_cast<uint8_t>(Wire.read());
        return true;
    }

    bool readTouchRegisters(uint8_t *buffer, size_t length)
    {
        Wire.beginTransmission(kTouchAddress);
        Wire.write(0x02);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        if (Wire.requestFrom(kTouchAddress, length) != length)
        {
            while (Wire.available())
            {
                Wire.read();
            }
            return false;
        }

        for (size_t i = 0; i < length; ++i)
        {
            buffer[i] = static_cast<uint8_t>(Wire.read());
        }
        return true;
    }

    void flushDisplay(lv_display_t *lvDisplay, const lv_area_t *area, uint8_t *pixels)
    {
        const int16_t width = lv_area_get_width(area);
        const int16_t height = lv_area_get_height(area);
        auto *colorBuffer = reinterpret_cast<uint16_t *>(pixels);
        static bool firstFlush = true;
        if (firstFlush)
        {
            Serial.printf("LVGL flush: x=%d..%d y=%d..%d, RGB565\n",
                          area->x1, area->x2, area->y1, area->y2);
            firstFlush = false;
        }

        if ((static_cast<uint32_t>(width) * height) & 1U)
        {
            if (height > 1)
            {
                display->draw16bitRGBBitmap(
                    area->x1, area->y1, colorBuffer, width, height - 1);
            }

            uint16_t *lastRow = colorBuffer + static_cast<size_t>(width) * (height - 1);
            if (width > 1)
            {
                display->draw16bitRGBBitmap(
                    area->x1, area->y2, lastRow, width - 1, 1);
            }
            display->drawPixel(area->x2, area->y2, lastRow[width - 1]);
        }
        else
        {
            display->draw16bitRGBBitmap(
                area->x1, area->y1, colorBuffer, width, height);
        }
        lv_display_flush_ready(lvDisplay);
    }

    void readTouch(lv_indev_t *, lv_indev_data_t *data)
    {
        uint8_t touchData[5] = {};
        if (readTouchRegisters(touchData, sizeof(touchData)))
        {
            if ((touchData[0] & 0x0F) > 0)
            {
                const uint16_t rawX =
                    (static_cast<uint16_t>(touchData[1] & 0x0F) << 8) | touchData[2];
                const uint16_t rawY =
                    (static_cast<uint16_t>(touchData[3] & 0x0F) << 8) | touchData[4];

                lastTouchPoint.x = rawY < kScreenWidth ? rawY : kScreenWidth - 1;
                lastTouchPoint.y = rawX < kDisplayNativeWidth
                                       ? kDisplayNativeWidth - 1 - rawX
                                       : 0;
                if (!touchPressed)
                {
                    Serial.printf("Dotyk: x=%u y=%u\n",
                                  lastTouchPoint.x, lastTouchPoint.y);
                }
                touchPressed = true;
                touchReleaseSamples = 0;
                lastTouchTime = millis();
            }
            else if (touchPressed && ++touchReleaseSamples >= 3)
            {
                touchPressed = false;
                touchReleaseSamples = 0;
            }
        }
        else if (millis() - lastTouchTime > 150)
        {
            touchPressed = false;
            touchReleaseSamples = 0;
        }

        data->point = lastTouchPoint;
        data->state = touchPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }

    void logGesture(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_GESTURE)
        {
            return;
        }

        const lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_active());
        const char *name = "unknown";
        switch (direction)
        {
        case LV_DIR_LEFT:
            name = "left";
            break;
        case LV_DIR_RIGHT:
            name = "right";
            break;
        case LV_DIR_TOP:
            name = "up";
            break;
        case LV_DIR_BOTTOM:
            name = "down";
            break;
        default:
            break;
        }
        Serial.printf("Gest: %s\n", name);
    }

    void logDisplayRefresh(lv_event_t *)
    {
        static bool logged = false;
        if (!logged)
        {
            Serial.println("LVGL: odswiezanie ekranu");
            logged = true;
        }
    }

    uint32_t tick()
    {
        return millis();
    }

    void halt(const char *message)
    {
        Serial.println(message);
        Serial.flush();
        while (true)
        {
            delay(1000);
        }
    }

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("ES3C40P: start");

    if (!psramFound())
    {
        halt("Brak PSRAM: sprawdz konfiguracje pamieci OPI");
    }
    Serial.println("PSRAM: OK");

    if (!display->begin(kDisplaySpiFrequency))
    {
        halt("Nie udalo sie uruchomic wyswietlacza ST7796S");
    }
    Serial.println("ST7796S: OK");

    pinMode(kBacklightPin, OUTPUT);
    digitalWrite(kBacklightPin, HIGH);
    display->fillScreen(RGB565_BLACK);

    Wire.begin(kTouchSdaPin, kTouchSclPin, 400000);

    pinMode(kTouchInterruptPin, OUTPUT);
    digitalWrite(kTouchInterruptPin, LOW);
    pinMode(kTouchResetPin, OUTPUT);
    digitalWrite(kTouchResetPin, LOW);
    delay(10);
    digitalWrite(kTouchResetPin, HIGH);
    delay(120);
    pinMode(kTouchInterruptPin, INPUT_PULLUP);

    uint8_t touchChipId = 0;
    if (readTouchRegister(0xA3, &touchChipId))
    {
        Serial.printf("FT6336U I2C 0x%02X, chip ID 0x%02X\n",
                      kTouchAddress, touchChipId);
    }
    else
    {
        Serial.println("Brak odpowiedzi kontrolera dotyku FT6336U pod adresem 0x38");
    }

    lv_init();
    lv_tick_set_cb(tick);

    lv_display_t *lvDisplay = lv_display_create(kScreenWidth, kScreenHeight);
    lv_display_set_default(lvDisplay);
    lv_display_set_color_format(lvDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lvDisplay, flushDisplay);
    lv_display_set_buffers(
        lvDisplay, drawBuffer, nullptr, sizeof(drawBuffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, readTouch);
    lv_indev_set_gesture_min_distance(touch, 25);
    lv_indev_set_gesture_min_velocity(touch, 3);

    ui_init();
    lv_obj_t *screen = lv_display_get_screen_active(lvDisplay);
    if (screen == nullptr)
    {
        halt("LVGL nie zaladowal ekranu");
    }
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x102030), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_event_cb(screen, logGesture, LV_EVENT_GESTURE, nullptr);
    for (uint32_t i = 0; i < lv_obj_get_child_count(screen); ++i)
    {
        lv_obj_set_gesture_bubble(lv_obj_get_child(screen, i), true);
    }
    lv_display_add_event_cb(lvDisplay, logDisplayRefresh, LV_EVENT_REFR_START, nullptr);
    lv_obj_invalidate(screen);
    lv_refr_now(lvDisplay);
    Serial.println("ES3C40P LVGL 9.6.0 ready");
}

void loop()
{
    ui_tick();
    lv_timer_handler();
    delay(5);
}
