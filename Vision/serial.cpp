#include "serial.h"
#define BAUDRATE 115200
using namespace std;

Serial::Serial(/* arg */)
{
}

Serial::~Serial()
{
}

void Serial::send_msg(const char* msg)
{
    int fd;
    if(wiringPiSetup() < 0){
        cout <<"wiringPi setup failed."<< endl;
        return;
    }
    if((fd = serialOpen("/dev/ttyAMA0", BAUDRATE)) < 0){
        cout <<"serial open failed." << endl;
        return;
    }
    if(serialDataAvail(fd) == 0 and msg[0]<'z' and msg[0]>'0')
    {
         serialPrintf(fd, msg);
    }
    serialClose(fd);
}

char* Serial::read_msg()
{
    int fd;
    if(wiringPiSetup() < 0){
        cout <<"wiringPi setup failed."<< endl;
        my_buf[0] = '\0';
    }
    if((fd = serialOpen("/dev/ttyAMA0", BAUDRATE)) < 0){
        cout <<"serial open failed." << endl;
        my_buf[0] = '\0';
    }
    if (serialDataAvail(fd)>0)
    {
        my_buf[my_buf_con] = serialGetchar(fd);
        // cout << my_buf[my_buf_con] << "  " << my_buf_con << endl;
        if (my_buf[my_buf_con] == '?')
        {
            my_buf[my_buf_con] = '\0';
            msg_get = my_buf;
        }
        else if(my_buf[my_buf_con] == '!')
        {
            my_buf_con = 0;
            msg_get[0] = '\0';
        }
        else{
            my_buf_con++;
        }
        
    }

    serialFlush(fd);
    return msg_get;
}
