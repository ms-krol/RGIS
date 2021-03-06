#include <cm.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <NCtime.h>

bool NCtimeLeapYear(int year) {
    return (year == 0 ? false : ((year & 0x03) != 0x00 ? false : ((year % 100) == 0 ? (((year / 100) & 0x03) == 0 ? true
                                                                                                                  : false)
                                                                                    : true)));
}

size_t NCtimeMonthLength(int year, size_t month) {
    int nDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    month = month - 1;
    if (year > 0) year += (int) month / 12;
    month = month % 12;
    return (nDays[month] + (((month == 1) && NCtimeLeapYear(year)) ? 1 : 0));
}

size_t NCtimeDayOfYear(int year, size_t month, size_t day) {
    size_t i;

    for (i = 1; i < month - 1; i++) day += NCtimeMonthLength(year, month);
    return (day);
}

/*
bool NCtimeLeapYear (utUnit *tUnit, double t)
{
	int year, month, day, hour, minute;
	float second;
	if (utCalendar (t, tUnit, &year, &month, &day, &hour, &minute, &second) != 0)
	{ CMmsgPrint (CMmsgAppError, "Time Conversion error in: %s %d",__FILE__,__LINE__); return (false); }
	return (NCtimeLeapYear (year));
}

size_t NCtimeMonthLength (utUnit *tUnit, double t)
{
	int nDays [] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int year, month, day, hour, minute;
	float second;

	if (utCalendar (t, tUnit, &year, &month, &day, &hour, &minute, &second) != 0)
	{ CMmsgPrint (CMmsgAppError, "Time Conversion error in: %s %d",__FILE__,__LINE__); return (0); }

	month = month - 1;
	if (year > 0) year += (int) month / 12;
	month = month % 12;

	return (nDays [month] + (_NCtimeLeapYear (year) ? 1 : 0));
}
*/

NCstate NCtimeParse(const char *timeStr, NCtimeStep timeStep, utUnit *tUnit, double *t) {
    double second;
    int year, month, day, hour, minute;

    second = 0.0;
    minute = hour = 0;
    day = 1;
    month = 1;
    if (sscanf(timeStr, "%04d", &year) != 1) return (NCfailed);
    else if (sscanf(timeStr + 5, "%02d", &month) != 1) month = 1;
    else if (sscanf(timeStr + 8, "%02d", &day) != 1) day = 1;
    else if (sscanf(timeStr + 11, "%02d", &hour) != 1) hour = 0;
    else if (sscanf(timeStr + 14, "%02d", &minute) != 1) minute = 0;
    if (utInvCalendar(year, month, day, hour, minute, second, tUnit, t) != 0) {
        CMmsgPrint(CMmsgAppError, "Time Conversion error in: %s %d", __FILE__, __LINE__);
        return (NCfailed);
    }
    return (NCsucceeded);
}

NCstate NCtimePrint(NCtimeStep timeStep, utUnit *tUnit, double t, char *timeStr) {
    float second;
    int year, month, day, hour, minute;

    if (utCalendar(t, tUnit, &year, &month, &day, &hour, &minute, &second) != 0) {
        CMmsgPrint(CMmsgAppError, "Time Conversion error in: %s %d", __FILE__, __LINE__);
        return (NCfailed);
    }
    switch (timeStep) {
        case NCtimeYear:
            sprintf (timeStr, "%04d", year);
            break;
        case NCtimeMonth:
            sprintf (timeStr, "%04d-%02d", year, month);
            break;
        case NCtimeDay:
            sprintf (timeStr, "%04d-%02d-%02d", year, month, day);
            break;
        case NCtimeHour:
            sprintf (timeStr, "%04d-%02d-%02d %02d", year, month, day, hour);
            break;
        case NCtimeMinute:
            sprintf (timeStr, "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
            break;
        case NCtimeSecond:
            sprintf (timeStr, "%04d-%02d-%02d %02d:%02d %.1f", year, month, day, hour, minute, second);
            break;
    }
    return (NCsucceeded);
}

const char *NCtimeStepString(NCtimeStep timeStep) {
    switch (timeStep) {
        case NCtimeYear:
            return ("year");
        case NCtimeMonth:
            return ("month");
        case NCtimeDay:
            return ("day");
        case NCtimeHour:
            return ("hour");
        case NCtimeMinute:
            return ("minute");
        case NCtimeSecond:
            return ("second");
    }
    return ((char *) NULL);
}
