/*
 *
 * aplog.h - A brief description goes here.
 *
 */

#ifndef _HEAD_APLOG_69E74D13_2F1D009B_17D8494C_H
#define _HEAD_APLOG_69E74D13_2F1D009B_17D8494C_H

#ifndef DLL_APP
#ifdef WIN32
#ifdef _USRDLL
#define DLL_APP _declspec(dllexport)
#else
#define DLL_APP _declspec(dllimport)
#endif
#else
#define DLL_APP
#endif
#endif

#define LGWRFLUSHNOW

#if defined(__cplusplus)
extern "C" void *__lgwr__handle;
#else
extern void *__lgwr__handle;
#endif


/* Define LGWR-relative macros.
 */
#define LGWROPEN(file, level, sz) __lgwr__handle = lgwr_open(file, level, sz)
#define LGWRCLOSE()		  { lgwr_close(__lgwr__handle); __lgwr__handle = NULL; }
#define SETLGWRFILE(newfile)	  lgwr_set_file(__lgwr__handle, newfile, 0, 1)
#define SETLGWRLEVEL(newlevel)	  lgwr_set_level(__lgwr__handle, newlevel)
#define SETLGWRSIZE(newsize)	  lgwr_set_size(__lgwr__handle, newsize)
#define LOGLEVELTITLE(loglevel)		(char*)lgwr_get_title(__lgwr__handle, loglevel)

#define LGWRFLUSHNOW

#define LGWRLEVELNOLEVEL  	-1
#define LGWRLEVELEMERGENCY	0
#define LGWRLEVELALERT		1
#define LGWRLEVELCRITICAL	2
#define LGWRLEVELERROR		3
#define LGWRLEVELWARN		4
#define LGWRLEVELNOTICE		5
#define LGWRLEVELINFO		6
#define LGWRLEVELDEBUG		7
#define LGWRLEVELS		8

#define LGWRSTREMERGENCY	"<0>"
#define LGWRSTRALERT		"<1>"
#define LGWRSTRCRITICAL		"<2>"
#define LGWRSTRERROR		"<3>"
#define LGWRSTRWARN		"<4>"
#define LGWRSTRNOTICE		"<5>"
#define LGWRSTRINFO		"<6>"
#define LGWRSTRDEBUG		"<7>"

#if defined(OS_LINUX) || defined(WIN32)
#define LGWR(data,len,fmt,...)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			fmt,##__VA_ARGS__)
#define LGWREMERGENCY(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,##__VA_ARGS__)
#define LGWRALERT(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,##__VA_ARGS__)
#define LGWRCRITICAL(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,##__VA_ARGS__)
#define LGWRERROR(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,##__VA_ARGS__)
#define LGWRWARN(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,##__VA_ARGS__)
#define LGWRNOTICE(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,##__VA_ARGS__)
#if SYS_LOG_LEVEL > 0
#define LGWRINFO(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,##__VA_ARGS__)
#else
#define LGWRINFO(data,len,fmt,...)	do { } while (0)
#endif
#if SYS_LOG_LEVEL > 1
#define LGWRDEBUG(data,len,fmt,...) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,##__VA_ARGS__)
#else
#define LGWRDEBUG(data,len,fmt,...)     do { } while (0)
#endif

/* I really do NOT like the definitions below. But I have no choice
 * because some C/C++ compilers do not support the ##__VA_ARGS__ grammer, like
 * SUN Workshop C/C++.
 */
#else

#define LGWR(data,len,msg)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,msg)
#define LGWREMERGENCY(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTREMERGENCY msg)
#define LGWRALERT(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRALERT msg)
#define LGWRCRITICAL(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRCRITICAL msg)
#define LGWRERROR(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRERROR msg)
#define LGWRWARN(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRWARN msg)
#define LGWRNOTICE(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRNOTICE msg)
#if SYS_LOG_LEVEL > 0
#define LGWRINFO(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRINFO msg)
#else
#define LGWRINFO(data,len,msg)      do { } while (0)
#endif
#if SYS_LOG_LEVEL > 1
#define LGWRDEBUG(data,len,msg) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__,LGWRSTRDEBUG msg)
#else
#define LGWRDEBUG(data,len,msg)     do { } while (0)
#endif

#endif  /* #ifdef OS_LINUX */

#define LGWR1(data,len,fmt,a)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a)
#define LGWR2(data,len,fmt,a,b)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b)
#define LGWR3(data,len,fmt,a,b,c)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b,c)
#define LGWR4(data,len,fmt,a,b,c,d)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b,c,d)
#define LGWR5(data,len,fmt,a,b,c,d,e)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b,c,d,e)
#define LGWR6(data,len,fmt,a,b,c,d,e,f)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b,c,d,e,f)
#define LGWR7(data,len,fmt,a,b,c,d,e,f,g)	\
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, fmt,a,b,c,d,e,f,g)

#define LGWREMERGENCY1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a)
#define LGWREMERGENCY2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b)
#define LGWREMERGENCY3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b,c)
#define LGWREMERGENCY4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b,c,d)
#define LGWREMERGENCY5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b,c,d,e)
#define LGWREMERGENCY6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b,c,d,e,f)
#define LGWREMERGENCY7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTREMERGENCY fmt,a,b,c,d,e,f,g)

#define LGWRALERT1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a)
#define LGWRALERT2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b)
#define LGWRALERT3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b,c)
#define LGWRALERT4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b,c,d)
#define LGWRALERT5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b,c,d,e)
#define LGWRALERT6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b,c,d,e,f)
#define LGWRALERT7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRALERT fmt,a,b,c,d,e,f,g)

#define LGWRCRITICAL1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a)
#define LGWRCRITICAL2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b)
#define LGWRCRITICAL3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b,c)
#define LGWRCRITICAL4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b,c,d)
#define LGWRCRITICAL5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b,c,d,e)
#define LGWRCRITICAL6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b,c,d,e,f)
#define LGWRCRITICAL7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRCRITICAL fmt,a,b,c,d,e,f,g)

#define LGWRERROR1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a)
#define LGWRERROR2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b)
#define LGWRERROR3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b,c)
#define LGWRERROR4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b,c,d)
#define LGWRERROR5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b,c,d,e)
#define LGWRERROR6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b,c,d,e,f)
#define LGWRERROR7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRERROR fmt,a,b,c,d,e,f,g)

#define LGWRWARN1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a)
#define LGWRWARN2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b)
#define LGWRWARN3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b,c)
#define LGWRWARN4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b,c,d)
#define LGWRWARN5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b,c,d,e)
#define LGWRWARN6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b,c,d,e,f)
#define LGWRWARN7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRWARN fmt,a,b,c,d,e,f,g)

#define LGWRNOTICE1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a)
#define LGWRNOTICE2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b)
#define LGWRNOTICE3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b,c)
#define LGWRNOTICE4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b,c,d)
#define LGWRNOTICE5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b,c,d,e)
#define LGWRNOTICE6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b,c,d,e,f)
#define LGWRNOTICE7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRNOTICE fmt,a,b,c,d,e,f,g)

#if SYS_LOG_LEVEL > 0
#define LGWRINFO1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a)
#define LGWRINFO2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b)
#define LGWRINFO3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b,c)
#define LGWRINFO4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b,c,d)
#define LGWRINFO5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b,c,d,e)
#define LGWRINFO6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b,c,d,e,f)
#define LGWRINFO7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRINFO fmt,a,b,c,d,e,f,g)
#else
#define LGWRINFO1(data,len,fmt,a)     do { } while (0)
#define LGWRINFO2(data,len,fmt,a,b)     do { } while (0)
#define LGWRINFO3(data,len,fmt,a,b,c)     do { } while (0)
#define LGWRINFO4(data,len,fmt,a,b,c,d)     do { } while (0)
#define LGWRINFO5(data,len,fmt,a,b,c,d,e)     do { } while (0)
#define LGWRINFO6(data,len,fmt,a,b,c,d,e,f)     do { } while (0)
#define LGWRINFO7(data,len,fmt,a,b,c,d,e,f,g)     do { } while (0)
#endif

#if SYS_LOG_LEVEL > 1
#define LGWRDEBUG1(data,len,fmt,a) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a)
#define LGWRDEBUG2(data,len,fmt,a,b) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b)
#define LGWRDEBUG3(data,len,fmt,a,b,c) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b,c)
#define LGWRDEBUG4(data,len,fmt,a,b,c,d) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b,c,d)
#define LGWRDEBUG5(data,len,fmt,a,b,c,d,e) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b,c,d,e)
#define LGWRDEBUG6(data,len,fmt,a,b,c,d,e,f) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b,c,d,e,f)
#define LGWRDEBUG7(data,len,fmt,a,b,c,d,e,f,g) \
	lgwr_prt(__lgwr__handle,data,len,__FILE__,__LINE__, \
			LGWRSTRDEBUG fmt,a,b,c,d,e,f,g)
#else
#define LGWRDEBUG1(data,len,fmt,a)     do { } while (0)
#define LGWRDEBUG2(data,len,fmt,a,b)     do { } while (0)
#define LGWRDEBUG3(data,len,fmt,a,b,c)     do { } while (0)
#define LGWRDEBUG4(data,len,fmt,a,b,c,d)     do { } while (0)
#define LGWRDEBUG5(data,len,fmt,a,b,c,d,e)     do { } while (0)
#define LGWRDEBUG6(data,len,fmt,a,b,c,d,e,f)     do { } while (0)
#define LGWRDEBUG7(data,len,fmt,a,b,c,d,e,f,g)     do { } while (0)
#endif

#if SYS_LOG_LEVEL > 0
#define LOGINFODECL(statement)	statement
#else
#define LOGINFODECL(statement)
#endif

#if SYS_LOG_LEVEL > 1
#define LOGDEBUGDECL(statement)	statement
#else
#define LOGDEBUGDECL(statement)
#endif

#if defined(OS_LINUX) || defined(WIN32)
#define LOG(fmt,...)                  LGWR(NULL,0,fmt,##__VA_ARGS__)
#define LOGEMERGENCY(fmt,...)         LGWREMERGENCY(NULL,0,fmt,##__VA_ARGS__)
#define LOGALERT(fmt,...)             LGWRALERT(NULL,0,fmt,##__VA_ARGS__)
#define LOGCRITICAL(fmt,...)          LGWRCRITICAL(NULL,0,fmt,##__VA_ARGS__)
#define LOGERROR(fmt,...)             LGWRERROR(NULL,0,fmt,##__VA_ARGS__)
#define LOGWARN(fmt,...)              LGWRWARN(NULL,0,fmt,##__VA_ARGS__)
#define LOGNOTICE(fmt,...)            LGWRNOTICE(NULL,0,fmt,##__VA_ARGS__)
#define LOGINFO(fmt,...)              LGWRINFO(NULL,0,fmt,##__VA_ARGS__)
#define LOGDEBUG(fmt,...)             LGWRDEBUG(NULL,0,fmt,##__VA_ARGS__)
#else
#define LOG(msg)                         LGWR(NULL,0,msg)
#define LOGEMERGENCY(msg)                LGWREMERGENCY(NULL,0,msg)
#define LOGALERT(msg)                    LGWRALERT(NULL,0,msg)
#define LOGCRITICAL(msg)                 LGWRCRITICAL(NULL,0,msg)
#define LOGERROR(msg)                    LGWRERROR(NULL,0,msg)
#define LOGWARN(msg)                     LGWRWARN(NULL,0,msg)
#define LOGNOTICE(msg)                   LGWRNOTICE(NULL,0,msg)
#define LOGINFO(msg)                     LGWRINFO(NULL,0,msg)
#define LOGDEBUG(msg)                    LGWRDEBUG(NULL,0,msg)
#endif

#define LOG1(fmt,a)                      LGWR1(NULL,0,fmt,a)
#define LOGEMERGENCY1(fmt,a)             LGWREMERGENCY1(NULL,0,fmt,a)
#define LOGALERT1(fmt,a)                 LGWRALERT1(NULL,0,fmt,a)
#define LOGCRITICAL1(fmt,a)              LGWRCRITICAL1(NULL,0,fmt,a)
#define LOGERROR1(fmt,a)                 LGWRERROR1(NULL,0,fmt,a)
#define LOGWARN1(fmt,a)                  LGWRWARN1(NULL,0,fmt,a)
#define LOGNOTICE1(fmt,a)                LGWRNOTICE1(NULL,0,fmt,a)
#define LOGINFO1(fmt,a)                  LGWRINFO1(NULL,0,fmt,a)
#define LOGDEBUG1(fmt,a)                 LGWRDEBUG1(NULL,0,fmt,a)

#define LOG2(fmt,a,b)                    LGWR2(NULL,0,fmt,a,b)
#define LOGEMERGENCY2(fmt,a,b)           LGWREMERGENCY2(NULL,0,fmt,a,b)
#define LOGALERT2(fmt,a,b)               LGWRALERT2(NULL,0,fmt,a,b)
#define LOGCRITICAL2(fmt,a,b)            LGWRCRITICAL2(NULL,0,fmt,a,b)
#define LOGERROR2(fmt,a,b)               LGWRERROR2(NULL,0,fmt,a,b)
#define LOGWARN2(fmt,a,b)                LGWRWARN2(NULL,0,fmt,a,b)
#define LOGNOTICE2(fmt,a,b)              LGWRNOTICE2(NULL,0,fmt,a,b)
#define LOGINFO2(fmt,a,b)                LGWRINFO2(NULL,0,fmt,a,b)
#define LOGDEBUG2(fmt,a,b)               LGWRDEBUG2(NULL,0,fmt,a,b)

#define LOG3(fmt,a,b,c)                  LGWR3(NULL,0,fmt,a,b,c)
#define LOGEMERGENCY3(fmt,a,b,c)         LGWREMERGENCY3(NULL,0,fmt,a,b,c)
#define LOGALERT3(fmt,a,b,c)             LGWRALERT3(NULL,0,fmt,a,b,c)
#define LOGCRITICAL3(fmt,a,b,c)          LGWRCRITICAL3(NULL,0,fmt,a,b,c)
#define LOGERROR3(fmt,a,b,c)             LGWRERROR3(NULL,0,fmt,a,b,c)
#define LOGWARN3(fmt,a,b,c)              LGWRWARN3(NULL,0,fmt,a,b,c)
#define LOGNOTICE3(fmt,a,b,c)            LGWRNOTICE3(NULL,0,fmt,a,b,c)
#define LOGINFO3(fmt,a,b,c)              LGWRINFO3(NULL,0,fmt,a,b,c)
#define LOGDEBUG3(fmt,a,b,c)             LGWRDEBUG3(NULL,0,fmt,a,b,c)

#define LOG4(fmt,a,b,c,d)                LGWR4(NULL,0,fmt,a,b,c,d)
#define LOGEMERGENCY4(fmt,a,b,c,d)       LGWREMERGENCY4(NULL,0,fmt,a,b,c,d)
#define LOGALERT4(fmt,a,b,c,d)           LGWRALERT4(NULL,0,fmt,a,b,c,d)
#define LOGCRITICAL4(fmt,a,b,c,d)        LGWRCRITICAL4(NULL,0,fmt,a,b,c,d)
#define LOGERROR4(fmt,a,b,c,d)           LGWRERROR4(NULL,0,fmt,a,b,c,d)
#define LOGWARN4(fmt,a,b,c,d)            LGWRWARN4(NULL,0,fmt,a,b,c,d)
#define LOGNOTICE4(fmt,a,b,c,d)          LGWRNOTICE4(NULL,0,fmt,a,b,c,d)
#define LOGINFO4(fmt,a,b,c,d)            LGWRINFO4(NULL,0,fmt,a,b,c,d)
#define LOGDEBUG4(fmt,a,b,c,d)           LGWRDEBUG4(NULL,0,fmt,a,b,c,d)

#define LOG5(fmt,a,b,c,d,e)              LGWR5(NULL,0,fmt,a,b,c,d,e)
#define LOGEMERGENCY5(fmt,a,b,c,d,e)     LGWREMERGENCY5(NULL,0,fmt,a,b,c,d,e)
#define LOGALERT5(fmt,a,b,c,d,e)         LGWRALERT5(NULL,0,fmt,a,b,c,d,e)
#define LOGCRITICAL5(fmt,a,b,c,d,e)      LGWRCRITICAL5(NULL,0,fmt,a,b,c,d,e)
#define LOGERROR5(fmt,a,b,c,d,e)         LGWRERROR5(NULL,0,fmt,a,b,c,d,e)
#define LOGWARN5(fmt,a,b,c,d,e)          LGWRWARN5(NULL,0,fmt,a,b,c,d,e)
#define LOGNOTICE5(fmt,a,b,c,d,e)        LGWRNOTICE5(NULL,0,fmt,a,b,c,d,e)
#define LOGINFO5(fmt,a,b,c,d,e)          LGWRINFO5(NULL,0,fmt,a,b,c,d,e)
#define LOGDEBUG5(fmt,a,b,c,d,e)         LGWRDEBUG5(NULL,0,fmt,a,b,c,d,e)

#define LOG6(fmt,a,b,c,d,e,f)            LGWR6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGEMERGENCY6(fmt,a,b,c,d,e,f)   LGWREMERGENCY6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGALERT6(fmt,a,b,c,d,e,f)       LGWRALERT6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGCRITICAL6(fmt,a,b,c,d,e,f)    LGWRCRITICAL6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGERROR6(fmt,a,b,c,d,e,f)       LGWRERROR6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGWARN6(fmt,a,b,c,d,e,f)        LGWRWARN6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGNOTICE6(fmt,a,b,c,d,e,f)      LGWRNOTICE6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGINFO6(fmt,a,b,c,d,e,f)        LGWRINFO6(NULL,0,fmt,a,b,c,d,e,f)
#define LOGDEBUG6(fmt,a,b,c,d,e,f)       LGWRDEBUG6(NULL,0,fmt,a,b,c,d,e,f)

#define LOG7(fmt,a,b,c,d,e,f,g)          LGWR7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGEMERGENCY7(fmt,a,b,c,d,e,f,g) LGWREMERGENCY7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGALERT7(fmt,a,b,c,d,e,f,g)     LGWRALERT7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGCRITICAL7(fmt,a,b,c,d,e,f,g)  LGWRCRITICAL7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGERROR7(fmt,a,b,c,d,e,f,g)     LGWRERROR7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGWARN7(fmt,a,b,c,d,e,f,g)      LGWRWARN7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGNOTICE7(fmt,a,b,c,d,e,f,g)    LGWRNOTICE7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGINFO7(fmt,a,b,c,d,e,f,g)      LGWRINFO7(NULL,0,fmt,a,b,c,d,e,f,g)
#define LOGDEBUG7(fmt,a,b,c,d,e,f,g)     LGWRDEBUG7(NULL,0,fmt,a,b,c,d,e,f,g)

#define DECLARE_LOGHANDLE void *__lgwr__handle = NULL

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *lgwr_open(char *file, int level, int size);
extern DLL_APP void  lgwr_close(void *hd);
extern void  lgwr_set_file(void *hd, char *s, int mode, int lockme);
extern DLL_APP void  lgwr_set_level(void *hd, int newlevel);
extern void  lgwr_set_size(void *hd, int newsize);
extern DLL_APP void  lgwr_prt(void *hd, const void *data, int len,
		const char *file, int line, const char *fmt, ...);
extern DLL_APP const void *lgwr_get_title(void *hd, int logLevel);
extern DLL_APP void lgwr_set_handle(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_APLOG_69E74D13_2F1D009B_17D8494C_H */
