#include "useslowdown.h"

EUseSlowDownType GetUseSlowDownType(void)
{
	if (!cl_useslowdown)
		return USE_SLOWDOWN_OLD; // Значення за замовчуванням

	int val = (int)cl_useslowdown->value;
	if (val < USE_SLOWDOWN_MIN) val = USE_SLOWDOWN_MIN;
	if (val > USE_SLOWDOWN_MAX) val = USE_SLOWDOWN_MAX;
	return (EUseSlowDownType)val;
}
