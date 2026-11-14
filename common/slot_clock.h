#ifndef SLOT_CLOCK_H
#define SLOT_CLOCK_H

enum { SLOT_YEAR, SLOT_MONTH, SLOT_DAY, SLOT_WEEKDAY, SLOT_HOUR, SLOT_MINUTE, SLOT_SECOND };

static unsigned Slot_DaysInMonth(unsigned year,unsigned month)
{
    static const unsigned char days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    return days[month-1]+(month==2 && year%4==0);
}

static void Slot_ReadDateTime(unsigned char datetime[7])
{
    static const unsigned char masks[]={0xff,0x1f,0x3f,0x07,0x3f,0x7f,0x7f};
    static const unsigned char maximum[]={99,12,31,6,23,59,59};
    unsigned i;
    rtc_enable();rtc_get(datetime);rtc_disenable();
    for(i=0;i<7;i++){
        unsigned bcd=datetime[i]&masks[i],value=UNBCD(bcd);
        datetime[i]=(bcd&15)<=9 && value<=maximum[i]?value:0;
    }
    if(!datetime[SLOT_MONTH])datetime[SLOT_MONTH]=1;
    if(!datetime[SLOT_DAY] || datetime[SLOT_DAY]>Slot_DaysInMonth(datetime[SLOT_YEAR],datetime[SLOT_MONTH]))
        datetime[SLOT_DAY]=1;
}

static void Slot_AdjustDateTime(unsigned char datetime[7],unsigned field,int direction)
{
    static const unsigned char minimum[]={0,1,1,0,0,0,0};
    static const unsigned char maximum[]={99,12,31,6,23,59,59};
    unsigned low=minimum[field],high=maximum[field],value=datetime[field],days;
    if(field==SLOT_DAY)high=Slot_DaysInMonth(datetime[SLOT_YEAR],datetime[SLOT_MONTH]);
    if(direction<0)value=value==low?high:value-1;
    else value=value==high?low:value+1;
    datetime[field]=value;
    days=Slot_DaysInMonth(datetime[SLOT_YEAR],datetime[SLOT_MONTH]);
    if(datetime[SLOT_DAY]>days)datetime[SLOT_DAY]=days;
}

static unsigned Slot_DateWeekday(const unsigned char datetime[7])
{
    unsigned year=datetime[SLOT_YEAR],month,days=year*365+(year+3)/4;
    /* January 1, 2000 was a Saturday. The RTC uses Sunday as zero. */
    for(month=1;month<datetime[SLOT_MONTH];month++)days+=Slot_DaysInMonth(year,month);
    return (days+datetime[SLOT_DAY]+5)%7;
}

#endif
