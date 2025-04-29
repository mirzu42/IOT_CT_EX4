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
#include "hardware/adc.h"
#include <cstdio>


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
float read_temperature();
void send_temperatures(float latest_temp, uint rotary_button, MQTT::Client<IPStack, Countdown> &client);
void blink_leds_temp(bool &state, float latest_temp, absolute_time_t &adc_timer);
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
        printf("Failed to parse JSON in reading\n");
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
char* create_json_message(const char* topic = "miro/led", const char* message = "Hello World!!!") {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "topic", cJSON_CreateString(topic));
    cJSON_AddItemToObject(json, "payload", cJSON_CreateString(message));
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
    const uint rotary_button = 12;

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
    gpio_set_dir(rotary_button, GPIO_IN);
    gpio_pull_up(rotary_button);






    // Initialize chosen serial port
    stdio_init_all();

    // Initialize adc
    adc_init(); // Ensure ADC is initialized
    adc_set_temp_sensor_enabled(true); // Enable temperature sensor
    adc_select_input(4); // Select temperature sensor channel

    absolute_time_t adc_timer = get_absolute_time();


    printf("\nBoot\n");

#ifdef USE_MQTT
    //const char *topic = "miro/led"; //EX4
    const char *topic = "miro/tmp"; // EX5
    //IPStack ipstack("SSID", "PASSWORD"); // example
    //IPStack ipstack("KME662", "Svoid send_temperatures(float latest_temp, uint rotary_button, MQTT::Client<IPStack, Countdown> &client)martIot"); // example
        IPStack ipstack("SmartIoTMQTT", "SmartIoT"); // example
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect("192.168.50.132", 1883);
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
    printf("MQTT subscribed to %s\n", topic);

    auto mqtt_send = make_timeout_time_ms(2000);
    int mqtt_qos = 0;
    int msg_count = 0;
#endif


    float latest_temp = read_temperature();
    bool state = true;
    while (true) {
        // EX5 stuff

        blink_leds_temp(state, latest_temp, adc_timer);


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
            const char* jsonmsg = create_json_message();
            printf("Button pressed\n");
            parse_and_publish_message(client, jsonmsg);
            free((void*)jsonmsg);
            sleep_ms(250);
        }

        // EX5 stuff


        send_temperatures(latest_temp, rotary_button, client);


        cyw43_arch_poll(); // obsolete? - see below
        client.yield(100); // socket that client uses calls cyw43_arch_poll()
#endif
        tight_loop_contents();
    }
}

float read_temperature() {



    uint16_t reading = adc_read();
    printf(" adc: %d\n", reading);
    // Convert to voltage (RP2040 ADC is 12-bit, 0-4095)
    // Maybe???
    float voltage = reading * 3.3f / 4095.0f;
    //float voltage = reading*3.3f / 65535;

    // Calculate temperature
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    return temperature;
}
void send_temperatures(float latest_temp, uint rotary_button, MQTT::Client<IPStack, Countdown> &client){
    if (button_pressed(rotary_button)){
        printf("Rotary encoder pressed\n");
        if(latest_temp >=30){
            char msg[15];
            snprintf(msg, sizeof(msg), "HIGH, %.2fC", latest_temp);
            const char* jsonmsg = create_json_message("miro/tmp", msg);
            parse_and_publish_message(client, jsonmsg);
            free((void*)jsonmsg);
            sleep_ms(250);
        }
        else if(latest_temp <=20){
            char msg[15];
            snprintf(msg, sizeof(msg), "LOW, %.2fC", latest_temp);
            const char* jsonmsg = create_json_message("miro/tmp", msg);
            parse_and_publish_message(client, jsonmsg);
            free((void*)jsonmsg);
            sleep_ms(250);
        }
        else {
            char msg[15];
            snprintf(msg, sizeof(msg), "NORMAL, %.2fC", latest_temp);
            const char* jsonmsg = create_json_message("miro/tmp", msg);
            parse_and_publish_message(client, jsonmsg);
            free((void*)jsonmsg);
            sleep_ms(250);
        }


    }
}
void blink_leds_temp(bool &state, float latest_temp, absolute_time_t &adc_timer){
    if (time_reached(adc_timer)){
        latest_temp = read_temperature();
        if (latest_temp >= 30){
            gpio_put(20, 1);
            gpio_put(22,0);
        }
        else if (latest_temp <= 20){
            gpio_put(22, 1);
            gpio_put(20,0);
        }
        else{
            gpio_put(20,0);
            gpio_put(22,0);
        }
        gpio_put(21,state);
        state = !state;
        adc_timer = delayed_by_ms(adc_timer, 1000);
    }
}
