#ifndef WIFI_H_
#define WIFI_H_

typedef void (*wifi_callback_t)(void);

void wifi_init(wifi_callback_t on_connected,
               wifi_callback_t on_disconnected);

#endif /* WIFI_H_ */

