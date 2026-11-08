#ifndef SLOT_USERNAME_H
#define SLOT_USERNAME_H
#define SLOT_USERNAME_MAX 10
/* Read only the first line. The DS Style font supports printable ASCII.
   Unsupported UTF-8 characters occupy one '?' cell, rather than leaking bytes. */
static void Slot_ParseUsername(const unsigned char *data,unsigned bytes,char *out)
{
    unsigned i=0,n=0;out[0]=0;
    if(bytes>=2 && ((data[0]==255 && data[1]==254) || (data[0]==254 && data[1]==255)))return;
    if(bytes>=3 && data[0]==239 && data[1]==187 && data[2]==191)i=3;
    while(i<bytes && (data[i]==' ' || data[i]=='\t'))i++;
    while(i<bytes && n<SLOT_USERNAME_MAX) {
        unsigned c=data[i++];
        if(!c || c=='\r' || c=='\n')break;
        if(c>=128){while(i<bytes && (data[i]&192)==128)i++;c='?';}
        if(c=='\t')c=' ';
        if(c<32 || c>126)c='?';
        out[n++]=(char)c;
    }
    while(n && out[n-1]==' ')n--;
    out[n]=0;
}
#endif
