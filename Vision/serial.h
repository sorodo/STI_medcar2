#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <wiringPi.h>
#include <wiringSerial.h>
#include <unistd.h>
#include <iostream>


class Serial
{
private:
    /* data */
public:
    Serial(/* args */);
    ~Serial();
    int my_buf_con = 0;
    char *msg_get;
    char my_buf[100];
    void send_msg(const char* msg);
    char* read_msg();
};


# endif