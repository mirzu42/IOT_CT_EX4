#include <stdio.h>
#include <string.h>
#include "cJSON/cJSON.h"
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "uart/PicoUart.h"

#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "ssd1306.h"
#include "epd154.h"
#include "FreeMono12pt7b.h"
#include "st7789nobuf.h"

#include "PicoI2CDevice.h"
#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"
#include "rgb_palette.h"


// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 1 // for simulator
//#define STOP_BITS 2 // for real system

//#define USE_MODBUS
#define USE_MQTT
//#define USE_SSD1306
//#define USE_EPD154
//#define USE_ST7789


#if defined(USE_SSD1306) || defined(USE_EPD154) || defined(USE_ST7789)
static const uint8_t raspberry26x32[] =
        {0x0, 0x0, 0xe, 0x7e, 0xfe, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xfc, 0xfe,
         0xfe, 0xff, 0xff,0xff, 0xff, 0xff, 0xfe, 0x7e,
         0x1e, 0x0, 0x0, 0x0, 0x80, 0xe0, 0xf8, 0xfd,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff,0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd,
         0xf8, 0xe0, 0x80, 0x0, 0x0, 0x1e, 0x7f, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0x7f, 0x1e, 0x0, 0x0,
         0x0, 0x3, 0x7, 0xf, 0x1f, 0x1f, 0x3f, 0x3f,
         0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f,
         0x3f, 0x1f, 0x1f, 0xf, 0x7, 0x3, 0x0, 0x0 };
#endif

void messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;
    printf("Message arrived YAYAYYAAYAYAYAYAYAYAYAY\n\n\n");

    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n",
           message.qos, message.retained, message.dup, message.id);
    printf("Payload %s\n", (char *) message.payload);


    // Null-terminate payload for parsing
    char json_string[128];
    snprintf(json_string, sizeof(json_string), "%.*s", message.payloadlen, (char *)message.payload);

    // Parse the JSON
    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        printf("Failed to parse JSON\n");
        return;
    }

    const cJSON *led = cJSON_GetObjectItem(root, "led");
    const cJSON *state = cJSON_GetObjectItem(root, "state");

    if (!cJSON_IsString(led) || !cJSON_IsString(state)) {
        printf("Invalid format\n");
        cJSON_Delete(root);
        return;
    }

    printf("LED: %s\n", led->valuestring);
    printf("State: %s\n", state->valuestring);

    int gpio = -1;
    if (strcmp(led->valuestring, "D1") == 0) gpio = 20;
    else if (strcmp(led->valuestring, "D2") == 0) gpio = 21;
    else if (strcmp(led->valuestring, "D3") == 0) gpio = 22;
    if (gpio != -1) {
        if (strcmp(state->valuestring, "ON") == 0) {
            gpio_put(gpio, 1);
        } else if (strcmp(state->valuestring, "OFF") == 0) {
            gpio_put(gpio, 0);
        } else if (strcmp(state->valuestring, "TOGGLE")==0) {
            gpio_put(gpio, !gpio_get(gpio));
        }
    }




}
 const char* create_json_message() {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "topic", cJSON_CreateString("miro/led"));
    cJSON_AddItemToObject(json, "payload", cJSON_CreateString("Hello World!!!!!!!!!!!!!!!!!!!!"));
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return json_str;
}
void parse_and_publish_message(MQTT::Client<IPStack, Countdown> &client, const char* json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        printf("Error parsing JSON\n");
        return;
    }
    cJSON *topic = cJSON_GetObjectItem(json, "topic");
    cJSON *payload = cJSON_GetObjectItem(json, "payload");
    if (topic != NULL && payload != NULL) {
        MQTT::Message message;
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void *) payload->valuestring;
        message.payloadlen = strlen(payload->valuestring) + 1;
        client.publish(topic->valuestring, message);
    }
    cJSON_Delete(json);

}
bool button_pressed(uint button) {
   return gpio_get(button) == 0;
}




int main() {

    const uint led_pin1 = 20;
    const uint led_pin2 = 21;
    const uint led_pin3 = 22;

    const uint button = 9;

    // Initialize LED pin
    gpio_init(led_pin1);
    gpio_set_dir(led_pin1, GPIO_OUT);
    gpio_init(led_pin2);
    gpio_set_dir(led_pin2, GPIO_OUT);
    gpio_init(led_pin3);
    gpio_set_dir(led_pin3, GPIO_OUT);
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);


    // Initialize chosen serial port
    stdio_init_all();


    printf("\nBoot\n");
#ifdef USE_SSD1306
    auto bus = std::make_shared<PicoI2CBus>(1, 14, 15);
    auto dev = std::make_shared<PicoI2CDevice>(bus, 0x3C);
    ssd1306 display(dev);
    display.fill(0);
    display.text("Hello", 0, 0);
    mono_vlsb rb(raspberry26x32, 26, 32);
    display.blit(rb, 20, 20);
    display.rect(15, 15, 35, 45, 1);
    display.line(60, 5, 120, 60, 1);
    display.line(60, 60, 120, 5, 1);
    display.show();
#if 1
    for(int i = 0; i < 128; ++i) {
        sleep_ms(50);
        display.scroll(1, 0);
        display.show();
    }
    display.text("Done", 20, 30);
    display.show();
#endif
    display.setfont(&FreeMono12pt7b);
    display.text("Free Mono", 0, 20,1);
    display.show();
#endif

#ifdef USE_EPD154
static const unsigned char binary_data[] = {
    // font edit begin : monohlsb : 48 : 60 : 48
    0x00, 0x3F, 0x00, 0x00, 0xFC, 0x00, 0x07, 0xFF,
    0xE0, 0x07, 0xFF, 0xE0, 0x1F, 0x85, 0x78, 0x1F,
    0x85, 0xF8, 0x3E, 0x00, 0x1C, 0x38, 0x00, 0x3C,
    0x10, 0x00, 0x06, 0x60, 0x00, 0x08, 0x38, 0x00,
    0x02, 0x60, 0x00, 0x1C, 0x30, 0x20, 0x03, 0xC0,
    0x04, 0x0C, 0x18, 0x0C, 0x01, 0xC0, 0x30, 0x18,
    0x18, 0x03, 0x01, 0x80, 0x40, 0x18, 0x1C, 0x00,
    0x81, 0x81, 0x80, 0x38, 0x0C, 0x00, 0x63, 0xC6,
    0x00, 0x10, 0x0E, 0x00, 0x37, 0xEC, 0x00, 0x70,
    0x06, 0x00, 0x1F, 0xF8, 0x00, 0x60, 0x03, 0x80,
    0x0F, 0xF8, 0x00, 0xC0, 0x03, 0x80, 0x1F, 0xF8,
    0x01, 0xC0, 0x00, 0xC0, 0x3F, 0xFC, 0x03, 0x00,
    0x00, 0xF9, 0xFF, 0xFF, 0x8F, 0x00, 0x00, 0x7F,
    0xF8, 0x1F, 0xFE, 0x00, 0x00, 0xF0, 0x60, 0x06,
    0x1F, 0x00, 0x01, 0xC0, 0xC0, 0x03, 0x03, 0x80,
    0x03, 0x80, 0xC0, 0x03, 0x81, 0xC0, 0x03, 0x01,
    0xC0, 0x03, 0xC0, 0xC0, 0x07, 0x07, 0xE0, 0x03,
    0xE0, 0x60, 0x06, 0x0F, 0xF8, 0x0F, 0xF0, 0x60,
    0x06, 0x1F, 0xFF, 0xF8, 0x78, 0x60, 0x06, 0x7C,
    0x0F, 0xF0, 0x1E, 0x60, 0x07, 0xF0, 0x07, 0xE0,
    0x0F, 0x70, 0x0F, 0xE0, 0x03, 0xC0, 0x07, 0xF0,
    0x1F, 0xE0, 0x01, 0xC0, 0x03, 0xB8, 0x39, 0xC0,
    0x01, 0xC0, 0x03, 0x8C, 0x71, 0xC0, 0x01, 0x80,
    0x01, 0x8E, 0x60, 0xC0, 0x01, 0xC0, 0x01, 0x06,
    0xE0, 0xC0, 0x01, 0xC0, 0x01, 0x07, 0xC1, 0xC0,
    0x03, 0xC0, 0x01, 0x83, 0xC1, 0xC0, 0x03, 0xE0,
    0x03, 0x83, 0xC1, 0xC0, 0x07, 0xF0, 0x03, 0x83,
    0xC1, 0xE0, 0x0F, 0xF8, 0x07, 0x83, 0xE1, 0xF0,
    0x1C, 0x1E, 0x0F, 0x87, 0x63, 0xFF, 0xF0, 0x0F,
    0xFF, 0xC6, 0x73, 0xFF, 0xE0, 0x03, 0xFE, 0xCE,
    0x3F, 0xFF, 0xC0, 0x03, 0xF8, 0x7C, 0x1C, 0x1F,
    0x80, 0x01, 0xF0, 0x38, 0x1C, 0x07, 0x80, 0x01,
    0xE0, 0x38, 0x1C, 0x03, 0x80, 0x01, 0xC0, 0x38,
    0x0C, 0x03, 0x80, 0x01, 0x80, 0x30, 0x0C, 0x01,
    0xC0, 0x01, 0x80, 0x30, 0x0C, 0x01, 0xC0, 0x03,
    0x00, 0x30, 0x0E, 0x00, 0xE0, 0x07, 0x00, 0x70,
    0x06, 0x00, 0xF0, 0x0F, 0x00, 0x60, 0x07, 0x00,
    0xFC, 0x3F, 0x00, 0xE0, 0x03, 0x80, 0xFF, 0xFF,
    0x01, 0xC0, 0x01, 0xE1, 0xFF, 0xFF, 0x07, 0x80,
    0x00, 0x7F, 0xF0, 0x07, 0xFE, 0x00, 0x00, 0x3F,
    0xC0, 0x01, 0xFC, 0x00, 0x00, 0x0F, 0x80, 0x01,
    0xF0, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0, 0x00,
    0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8,
    0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00
    // font edit end
};
#if 0
    static uint8_t inv_raspberry26x32[sizeof(raspberry26x32)];
    for(size_t i = 0; i < sizeof(raspberry26x32); ++i) {
        inv_raspberry26x32[i] = ~raspberry26x32[i];
    }
#endif
    epd154 display;
    display.fill(1);
    display.show();
    display.text("Hello", 0, 0);
    mono_vlsb rb(raspberry26x32, 26, 32);
    mono_horiz rpi(binary_data,48,60);
    display.blit(rb, 20, 20);
    display.blit(rpi, 120, 120);
    display.rect(15, 15, 35, 45, 0);
    display.line(60, 5, 120, 60, 0);
    display.line(60, 60, 120, 5, 0);
    display.show();
#if 0
    for(int i = 0; i < 128; ++i) {
        sleep_ms(50);
        display.scroll(1, 0);
        //display.show();
    }
    display.text("Done", 20, 20);
    display.show();
#endif

#endif
#ifdef USE_ST7789
static const unsigned char binary_data[] = {
    // font edit begin : monohlsb : 48 : 60 : 48
    0x00, 0x3F, 0x00, 0x00, 0xFC, 0x00, 0x07, 0xFF,
    0xE0, 0x07, 0xFF, 0xE0, 0x1F, 0x85, 0x78, 0x1F,
    0x85, 0xF8, 0x3E, 0x00, 0x1C, 0x38, 0x00, 0x3C,
    0x10, 0x00, 0x06, 0x60, 0x00, 0x08, 0x38, 0x00,
    0x02, 0x60, 0x00, 0x1C, 0x30, 0x20, 0x03, 0xC0,
    0x04, 0x0C, 0x18, 0x0C, 0x01, 0xC0, 0x30, 0x18,
    0x18, 0x03, 0x01, 0x80, 0x40, 0x18, 0x1C, 0x00,
    0x81, 0x81, 0x80, 0x38, 0x0C, 0x00, 0x63, 0xC6,
    0x00, 0x10, 0x0E, 0x00, 0x37, 0xEC, 0x00, 0x70,
    0x06, 0x00, 0x1F, 0xF8, 0x00, 0x60, 0x03, 0x80,
    0x0F, 0xF8, 0x00, 0xC0, 0x03, 0x80, 0x1F, 0xF8,
    0x01, 0xC0, 0x00, 0xC0, 0x3F, 0xFC, 0x03, 0x00,
    0x00, 0xF9, 0xFF, 0xFF, 0x8F, 0x00, 0x00, 0x7F,
    0xF8, 0x1F, 0xFE, 0x00, 0x00, 0xF0, 0x60, 0x06,
    0x1F, 0x00, 0x01, 0xC0, 0xC0, 0x03, 0x03, 0x80,
    0x03, 0x80, 0xC0, 0x03, 0x81, 0xC0, 0x03, 0x01,
    0xC0, 0x03, 0xC0, 0xC0, 0x07, 0x07, 0xE0, 0x03,
    0xE0, 0x60, 0x06, 0x0F, 0xF8, 0x0F, 0xF0, 0x60,
    0x06, 0x1F, 0xFF, 0xF8, 0x78, 0x60, 0x06, 0x7C,
    0x0F, 0xF0, 0x1E, 0x60, 0x07, 0xF0, 0x07, 0xE0,
    0x0F, 0x70, 0x0F, 0xE0, 0x03, 0xC0, 0x07, 0xF0,
    0x1F, 0xE0, 0x01, 0xC0, 0x03, 0xB8, 0x39, 0xC0,
    0x01, 0xC0, 0x03, 0x8C, 0x71, 0xC0, 0x01, 0x80,
    0x01, 0x8E, 0x60, 0xC0, 0x01, 0xC0, 0x01, 0x06,
    0xE0, 0xC0, 0x01, 0xC0, 0x01, 0x07, 0xC1, 0xC0,
    0x03, 0xC0, 0x01, 0x83, 0xC1, 0xC0, 0x03, 0xE0,
    0x03, 0x83, 0xC1, 0xC0, 0x07, 0xF0, 0x03, 0x83,
    0xC1, 0xE0, 0x0F, 0xF8, 0x07, 0x83, 0xE1, 0xF0,
    0x1C, 0x1E, 0x0F, 0x87, 0x63, 0xFF, 0xF0, 0x0F,
    0xFF, 0xC6, 0x73, 0xFF, 0xE0, 0x03, 0xFE, 0xCE,
    0x3F, 0xFF, 0xC0, 0x03, 0xF8, 0x7C, 0x1C, 0x1F,
    0x80, 0x01, 0xF0, 0x38, 0x1C, 0x07, 0x80, 0x01,
    0xE0, 0x38, 0x1C, 0x03, 0x80, 0x01, 0xC0, 0x38,
    0x0C, 0x03, 0x80, 0x01, 0x80, 0x30, 0x0C, 0x01,
    0xC0, 0x01, 0x80, 0x30, 0x0C, 0x01, 0xC0, 0x03,
    0x00, 0x30, 0x0E, 0x00, 0xE0, 0x07, 0x00, 0x70,
    0x06, 0x00, 0xF0, 0x0F, 0x00, 0x60, 0x07, 0x00,
    0xFC, 0x3F, 0x00, 0xE0, 0x03, 0x80, 0xFF, 0xFF,
    0x01, 0xC0, 0x01, 0xE1, 0xFF, 0xFF, 0x07, 0x80,
    0x00, 0x7F, 0xF0, 0x07, 0xFE, 0x00, 0x00, 0x3F,
    0xC0, 0x01, 0xFC, 0x00, 0x00, 0x0F, 0x80, 0x01,
    0xF0, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0, 0x00,
    0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8,
    0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00
    // font edit end
};
    auto spi = std::make_shared<PicoSPIBus>(0, 18, 19);
    auto dev = std::make_shared<PicoSPIDevice>(spi, 17);
    st7789nobuf display(dev, 27); // old 16
    display.fill(0xFFFF);
    display.show();
    display.text("Hello", 0, 0, 0xFFFF);
    mono_vlsb rb(raspberry26x32, 26, 32);
    mono_horiz rpi(binary_data,48,60);
    display.blit(rb, 20, 20, 0, rgb_palette(2, 0xFC00));
    display.blit(rpi, 120, 120, 0, rgb_palette(2, 0xF81F));
    display.rect(15, 15, 35, 45, 0x07E0);
    display.line(60, 5, 120, 60, 0xF800);
    display.line(60, 60, 120, 5, 0xF800);
    display.setfont(&FreeMono12pt7b);
    display.text("Free Mono", 10, 100,0xF800);
    display.show();

#endif


#ifdef USE_MQTT
    const char *topic = "miro/led";
    //IPStack ipstack("SSID", "PASSWORD"); // example
    //IPStack ipstack("KME662", "SmartIot"); // example
    IPStack ipstack("PicoTesting", "VerySecureP!c0P@ssw0rd"); // example
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect("192.168.1.114", 1883);
    if (rc != 1) {
        printf("rc from TCP connect is %d\n", rc);
    }

    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *) "PicoW-sample";
    //data.username.cstring = (char *)"keijo";
    //data.password.cstring = (char *)"test";
    rc = client.connect(data);
    if (rc != 0) {
        printf("rc from MQTT connect is %d\n", rc);
        while (true) {
            tight_loop_contents();
        }
    }
    printf("MQTT connected\n");

    // We subscribe QoS2. Messages sent with lower QoS will be delivered using the QoS they were sent with
    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("rc from MQTT subscribe is %d\n", rc);
    }
    printf("MQTT subscribed\n");

    auto mqtt_send = make_timeout_time_ms(2000);
    int mqtt_qos = 0;
    int msg_count = 0;
#endif
    const char* jsonmsg = create_json_message();


    while (true) {
#ifdef USE_MQTT
        if (time_reached(mqtt_send)) {
            mqtt_send = delayed_by_ms(mqtt_send, 2000);
            if (!client.isConnected()) {
                printf("Not connected...\n");
                rc = client.connect(data);
                if (rc != 0) {
                    printf("rc from MQTT connect is %d\n", rc);
                }

            }
        }
        if (button_pressed(button)) {
            printf("Button pressed\n");
            parse_and_publish_message(client, jsonmsg);
            free((void*)jsonmsg);
            sleep_ms(250);
        }
        cyw43_arch_poll(); // obsolete? - see below
        client.yield(100); // socket that client uses calls cyw43_arch_poll()
#endif
        tight_loop_contents();
    }
}

