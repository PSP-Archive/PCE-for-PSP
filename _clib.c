// clib-mimic


#include "_clib.h"



void _strcpy(char *d,const char *s)
{
	for (; *s!=0; ++s, ++d) *d=*s;
	*d=0;
}

void _strcat(char *d,const char *s)
{
	for (; *d!=0; ++d) ;
	for (; *s!=0; ++s, ++d) *d=*s;
	*d=0;
}

void _strncpy(char *d,const char *s,int n)
{
	for (; *s!=0 && n>0; ++s, ++d, --n) *d=*s;
	*d=0;
}


char *_strchr(const char *_s, int c)
{
	char *s=(char *)_s;
	for (; *s!=0; s++) {
		if (*s==(char)c) return s;
	}
	return NULL;
}

unsigned int _strlen(const char *s) 
{
	unsigned int r=0;
	for (; *s!=0; ++s, ++r) ;
	return r;
}


char* _strrchr(const char *src, int c)
{
	int len;
	
	len=_strlen(src);
	while(len>0){
		len--;
		if(*(src+len) == c)
			return (char*)(src+len);
	}
	
	return NULL;
}

void _memset(void *d,  long s, unsigned long n)
{
//	for (; n>0; n--) *(((char *)d)++)=s;
	{
		char a=s;
		char *p=d;
		while(n--)*p++=a;
	}
}

int _memcmp(const void *s1, const void *s2, unsigned long n)
{
	char *s1s=(char*)s1;
	char *s2s=(char*)s2;
	for (; n>0; n--) {
		if (*s1s>*s2s) return 1;
		if (*s1s++<*s2s++) return -1;
	}
	return 0;
}

void _memcpy(void *d,  void *s, unsigned long n)
{
/*
	for (; n>0; n--) {
		(*(((unsigned char *)d)++)=*(((unsigned char *)s)++));
	}
*/
	{
		char *g=s;
		char *p=d;
		while(n--)*p++=*g++;
	}

}


