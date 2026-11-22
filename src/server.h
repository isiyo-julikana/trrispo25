#ifndef TRRISPO25_SERVER_H
#define TRRISPO25_SERVER_H

/* Запускает HTTP-сервер на указанном порту.
 * Возвращает 0 при успешном завершении (например, по сигналу),
 * ненулевой код при ошибке. */
int run_server(int port);

#endif
