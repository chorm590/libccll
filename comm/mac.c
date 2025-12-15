#include <ctype.h>
#include <string.h>

#include "def.h"


/*
 * 	00:11:22:33:44:55
 * 	00-11-22-33-44-55
 * */
static bool _chk_type1(const char *mac)
{
    int i = 0;
    int colonCount = 0;
    int dashCount = 0;

    if (strlen(mac) != 17)
	{
        return false;
    }

    for (i = 0; i < 17; i++)
	{
        if (i % 3 != 2)
		{
            if (!isxdigit(mac[i]))
			{
                return false;
            }
        }
		else
		{
            if (mac[i] == ':')
			{
                colonCount++;
            }
			else if (mac[i] == '-')
			{
                dashCount++;
            }
			else
			{
                return false;
            }
        }
    }

    if (!(colonCount == 5 || dashCount == 5))
	{
        return false;
    }

    return true;
}

/*
 * 001122334455
 * */
static bool _chk_type2(const char *mac)
{
    int len = strlen(mac);

    if (len != 12)
	{
        return false;
    }

    for (int i = 0; i < 12; i++)
	{
        if (!isxdigit(mac[i]))
		{
            return false;
        }
    }

    return true;
}

bool cl_is_mac_valid(const char *mac)
{
	if(mac == NULL || *mac == 0) return false;

	if(_chk_type1(mac)) return true;
	if(_chk_type2(mac)) return true;

	return false;
}

