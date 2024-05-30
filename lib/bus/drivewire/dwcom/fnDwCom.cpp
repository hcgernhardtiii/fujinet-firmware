#ifdef BUILD_COCO

#include "fnDwCom.h"

#include "../../include/debug.h"
/*
 * DriveWire Communication class
 * (replacement for UARTManager fnUartBUS)
 * It uses DwPort for data exchange.
 * DwPort can be physical serial port (SerialDwPort) to communicate with real CoCo computer
 * or Becker port (DriveWire over TCP) for use with CoCo Emulators
 */

DwCom fnDwCom;

DwCom::DwCom() : _dw_mode(dw_mode::SERIAL), _dwPort(&_serialDw) {}

void DwCom::begin(int baud)
{
    if (baud)
        _dwPort->begin(baud);
    else
        _dwPort->begin(get_baudrate());
}

void DwCom::end() 
{ 
    _dwPort->end(); 
}

/*
 Poll the DriveWire port
 * ms = milliseconds to wait for "port event"
 * return true if port handling is needed
 */
bool DwCom::poll(int ms)
{
    return _dwPort->poll(ms);
}

void DwCom::set_baudrate(uint32_t baud) 
{ 
    _dwPort->set_baudrate(baud); 
}

uint32_t DwCom::get_baudrate()
{
    return _dwPort->get_baudrate(); 
}

int DwCom::available() 
{
    return _dwPort->available();
}

void DwCom::flush() 
{
    _dwPort->flush();
}

void DwCom::flush_input()
{
    _dwPort->flush_input();
}

// read single byte
int DwCom::read()
{
    return _dwPort->read();
}

// read bytes into buffer
size_t DwCom::read(uint8_t *buffer, size_t length)
{
    return _dwPort->read(buffer, length);
}

// alias to read
size_t DwCom::readBytes(uint8_t *buffer, size_t length)
{
    return  _dwPort->read(buffer, length);
}

// write single byte
ssize_t DwCom::write(uint8_t b)
{
    return _dwPort->write(b);
}

// write buffer
ssize_t DwCom::write(const uint8_t *buffer, size_t size) 
{
    return _dwPort->write(buffer, size);
}

// write C-string
ssize_t DwCom::write(const char *str)
{
    return _dwPort->write((const uint8_t *)str, strlen(str));
};

// print utility functions

size_t DwCom::_print_number(unsigned long n, uint8_t base)
{
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if(base < 2)
        base = 10;

    do {
        unsigned long m = n;
        n /= base;
        char c = m - base * n;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(n);

    return write(str);
}

size_t DwCom::print(const char *str)
{
    return write(str);
}

size_t DwCom::print(std::string str)
{
    return print(str.c_str());
}

size_t DwCom::print(int n, int base)
{
    return print((long) n, base);
}

size_t DwCom::print(unsigned int n, int base)
{
    return print((unsigned long) n, base);
}

size_t DwCom::print(long n, int base)
{
    if(base == 0) {
        return write(n);
    } else if(base == 10) {
        if(n < 0) {
            // int t = print('-');
            int t = print("-");
            n = -n;
            return _print_number(n, 10) + t;
        }
        return _print_number(n, 10);
    } else {
        return _print_number(n, base);
    }
}

size_t DwCom::print(unsigned long n, int base)
{
    if(base == 0) {
        return write(n);
    } else {
        return _print_number(n, base);
    }
}

// specific to SerialPort
#ifndef ESP_PLATFORM
void DwCom::set_serial_port(const char *device)
{
    Debug_printf("DwCom::set_serial_port %s", device ? device : "NULL");
    _serialDw.set_port(device);
};

const char* DwCom::get_serial_port()
{
    return _serialDw.get_port();
};
#endif

// specific to BeckerPort
void DwCom::set_becker_host(const char *host, int port) 
{
    _beckerDw.set_host(host, port);
}

const char* DwCom::get_becker_host(int &port) 
{
    return _beckerDw.get_host(port);
}

void DwCom::set_dw_mode(dw_mode mode)
{
    _dw_mode = mode;
    switch(mode)
    {
    case dw_mode::BECKER:
        _dwPort = &_beckerDw;
        break;
    default:
        _dwPort = &_serialDw;
    }
}

// toggle BeckerPort and SerialPort
void DwCom::reset_dw_port(dw_mode mode)
{
    uint32_t baud = get_baudrate();
    end();
    set_dw_mode(mode);
    begin(baud);
}

#endif // BUILD_COCO
