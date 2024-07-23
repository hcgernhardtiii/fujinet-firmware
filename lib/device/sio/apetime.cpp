#ifdef BUILD_ATARI

#include "apetime.h"

#include <cstring>
#include <ctime>
#include "compat_string.h"

#include "../../include/debug.h"


#define SIO_APETIMECMD_GETTIME 0x93
#define SIO_APETIMECMD_SETTZ 0x99
#define SIO_APETIMECMD_GETTZTIME 0x9A

char * ape_timezone = NULL;

void sioApeTime::_sio_get_time(bool use_timezone)
{
    char *tz_env = nullptr;
    char old_tz[64];
#if defined(_WIN32)
    // We have to use putenv() on Windows/MinGW/MSYS2
    static char tz_eq_str[64+3] = "TZ="; // prepare for putenv()
#endif

    if (use_timezone) 
    {
        Debug_println("APETIME time query (timezone)");
    }
    else 
    {
        Debug_println("APETIME time query (classic)");
    }

    uint8_t sio_reply[6] = { 0 };

    time_t tt = time(nullptr);

    if (ape_timezone != NULL && use_timezone) 
    {
        Debug_printf("Using time zone %s\n", ape_timezone);
        // Save current TZ env. variable
        tz_env = getenv("TZ");
        if (tz_env)
        {
            strlcpy(old_tz, tz_env, sizeof(old_tz));
        }
        // Set our TZ
#if defined(_WIN32)
        // Use putenv() on Windows/MinGW/MSYS2
        strlcpy(tz_eq_str+3, ape_timezone, sizeof(tz_eq_str)-3);
        putenv(tz_eq_str);
#else
        setenv("TZ", ape_timezone, 1);
#endif
        tzset();
    }

    struct tm * now = localtime(&tt);

    if (ape_timezone != NULL && use_timezone) 
    {
        // Restore TZ env. variable
        Debug_printf("Restoring TZ to: %s\n", tz_env ? old_tz : "<unset>");
        if (tz_env)
        {
#if defined(_WIN32)
            strcpy(tz_eq_str+3, old_tz); // "TZ=..."
            putenv(tz_eq_str);
#else
            setenv("TZ", old_tz, 1);
#endif
        }
        else
        {
#if defined(_WIN32)
            tz_eq_str[3] = '\0'; // "TZ="
            putenv(tz_eq_str);
#else
            unsetenv("TZ");
#endif
        }
        tzset();
    }

    now->tm_mon++;
    now->tm_year-=100;

    sio_reply[0] = now->tm_mday;
    sio_reply[1] = now->tm_mon;
    sio_reply[2] = now->tm_year;
    sio_reply[3] = now->tm_hour;
    sio_reply[4] = now->tm_min;
    sio_reply[5] = now->tm_sec;

    Debug_printf("Returning %02d/%02d/%02d %02d:%02d:%02d\n", now->tm_year, now->tm_mon, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

    bus_to_computer(sio_reply, sizeof(sio_reply), false);
}

void sioApeTime::_sio_set_tz()
{
    int bufsz;

    Debug_println("APETIME set TZ request");

    if (ape_timezone != NULL) 
    {
        free(ape_timezone);
    }

    bufsz = sio_get_aux();
    if (bufsz > 0) 
    {
        ape_timezone = (char *) malloc((bufsz + 1) * sizeof(char));

        uint8_t ck = bus_to_peripheral((uint8_t *) ape_timezone, bufsz);
        if (sio_checksum((uint8_t *) ape_timezone, bufsz) != ck) 
        {
            sio_error();
        } 
        else
        {
            ape_timezone[bufsz] = '\0';
            sio_complete();
            Debug_printf("TZ set to <%s>\n", ape_timezone); 
        }
    }
    else
    {
        Debug_printf("TZ unset\n");
    }
}

void sioApeTime::sio_process(uint32_t commanddata, uint8_t checksum)
{
    cmdFrame.commanddata = commanddata;
    cmdFrame.checksum = checksum;

    switch (cmdFrame.comnd)
    {
    case SIO_APETIMECMD_GETTIME:
        sio_ack();
        _sio_get_time(false);
        break;
    case SIO_APETIMECMD_SETTZ:
        sio_late_ack();
        _sio_set_tz();
        break;
    case SIO_APETIMECMD_GETTZTIME:
        sio_ack();
        _sio_get_time(true);
        break;
    default:
        sio_nak();
        break;
    };
}
#endif /* BUILD_ATARI */