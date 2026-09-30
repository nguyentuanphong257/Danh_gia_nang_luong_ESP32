#ifndef MQTT_H
#define MQTT_H

#include <stdbool.h>


#define MQTT_BROKER_URI "mqtt://broker.hivemq.com:1883"

#define MQTT_TOPIC_DATA "vias/air_monitor/data"

void mqtt_manager_init(void);


bool mqtt_manager_is_connected(void);

bool mqtt_manager_publish(
    const char *topic,
    const char *payload,
    int qos
);

#endif