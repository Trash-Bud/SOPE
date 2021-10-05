#ifndef SIGNALS_H
#define SIGNALS_H

void send_signal(int pid, int signal);

void signal_handler_default(int signo);

void signal_handler_SIGINT_child(int signo);

void signal_handler_SIGINT_parent(int signo);

void define_handlers_parent();

void signal_handler_SIGCONT(int signo);

void signal_handler_SIGTERM(int signo);

void define_signal_handler_children();


#endif