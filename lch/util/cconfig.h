/*
 *
 * cconfig.h - Configuration file manipulation tools.
 *
 */

#ifndef _HEAD_CCONFIG_30473C2D_744CDECE_426C0B80_H
#define _HEAD_CCONFIG_30473C2D_744CDECE_426C0B80_H

#include <stdarg.h>
#include <stdio.h>

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

#define TYPE_SECTION		1
#define TYPE_TOKEN_SCALAR	2
#define TYPE_TOKEN_VECTOR	3
#define TYPE_COMMENT		4
#define TYPE_BLANK		5

#define fIllegalType(type) (((type) < TYPE_SECTION) || ((type) > TYPE_BLANK))
#define fIsToken(type) \
	(((type) == TYPE_TOKEN_SCALAR) || ((type) == TYPE_TOKEN_VECTOR))

#if defined(__cplusplus)
extern "C" {
#endif

struct stLINE;

typedef struct stTOKEN {
	int              type;
	int              index;
	char            *name;
	char            *value;
	struct stTOKEN  *next;   /* point to the next token in this line  */
	struct stTOKEN  *snext;  /* point to the next token within a      */
	                         /* section. This pointer is valid only   */
	                         /* when type is TYPE_TOKEN_*.            */
	struct stLINE   *line;   /* point to the line which contains      */
	                         /* this token.                           */
} stToken;

typedef struct stLINE {
	struct stTOKEN  *token;  /* point to the first token in this line */
	struct stTOKEN  *etoken; /* point to the first TYPE_TOKEN_*       */
	                         /* token within a section. This pointer  */
	                         /* is valid only when this line is the   */
	                         /* definition of a section.              */
	struct stLINE   *prev;   /* previous line                         */
	struct stLINE   *next;   /* next line                             */
} stLine;

/*
 * If functions declared within this class return an pointer and no further
 * explanation, then the return value of NULL pointer indicates error,
 * others indicate success. If the return value has the type of integer and no
 * further explanation, zero indicates success while -1 indicates error.
 */

typedef struct stCCONFIG {
	char       filename[128];

	stLine    *lines;
	stLine   **sections;

	int       iTotalSections;

	/* If configuration was changed by function call like 'CfgSetValue'
	 * since the loading at very beginning, 'iDirtyFlag' will be set to 1.
	 * When the instance is deleted, changes will be saved to file.
	 * But if iWriteNow flag is set, your modification will be written
	 * to file immediately.
	 */
	int       iDirtyFlag;
	int       iWriteNow;

	int       iTokensPerLine;

	int       iCaseSensitive;

	/* This flag indicates whether the instance is properly initialized.
	 */
	int       iIAmReady;
} stCConfig;

DLL_APP unsigned long CfgInitialize(const char *fname);
DLL_APP unsigned long CfgInitializeString(const char *cfgstr);
DLL_APP int CfgInvalidate(unsigned long h);

/*
 * If 'flag' is not zero, then 'path' contains the configurations. It
 * doesn't indicate the name of the configuration file any more. So,
 * within the function, we should fetch the configurations directly from
 * the 'path'.
 */
int  CfgReload(unsigned long h, const char *path, int flag);
int  CfgLoadConfiguration(unsigned long h, const char *buf, int flag);

int  CfgReady(unsigned long h);
int  CfgSetWriteNow(unsigned long h, int writenow);
int  CfgSetCaseSensitive(unsigned long h, int yes);
int  CfgSetColumn(unsigned long h, int column);
int  CfgDump(unsigned long h, const char *path);
int  CfgSaveMeToFile(unsigned long h);
void CfgReleaseMemory(unsigned long h);

/*
 * This member function will get the value according to the id. The
 * value will be copied to memory pointed by parameter 'value'. So
 * be careful about the 'value' parameter.
 *
 * The parameter 'id' is also used to be a flag to distinguish the
 * type of configuration item you request. If it is NULL, I consider
 * that what you want is TYPE_TOKEN_VECTOR. This kind of token is
 * defined like the example below.
 *	[colors]
 *	red orange yellow green cyan blue purple
 * While non-NULL pointer indicates TYPE_TOKEN_SCALAR, for an example,
 *	[server configuration]
 * 	ip = 127.0.0.1
 *	port = 6001
 *
 *	[clients]
 *	client = 192.168.1.11
 *	client = 132.108.58.51
 * Under this kind of circumstance, 'id' may be a pointer to a
 * character string "ip", "port" or "client".
 *
 * So if you want to retrieve the ip address from the above example,
 * you may issue a function call like this,
 *	char  caIpAddr[16];
 *	CfgGetValue(h, "server configuration", "ip", caIpAddr, 1, 1);
 * and if you want to retrieve the second client address, you may issue
 * the function call like this,
 *	char  caIpAddr[16];
 *	CfgGetValue(h, "clients", "client", caIpAddr, 2, 1);
 *
 * If you want to retrieve the yellow color from colors section in the
 * above example, be careful, it's type is different from "ip" or
 * "client", you may issue a function call like this,
 *	char   caColor[16];
 *	CfgGetValue(h, "colors", NULL, caColor, 3, 1);
 *
 * New!
 * If you have two section with same section name, and you want to
 * retrieve something from the second, just call this function with
 * parameter 'section_index' being 2.
 */
DLL_APP int CfgGetValue(unsigned long h, const char *section, const char *id, char *value, int index, int section_index);
int CfgGetValueInt(unsigned long h, const char *section, const char *id, int index, int section_index);

/*
 * Compared with fGetValue, CfgSetValue sets the value of id. The grammar
 * is almost the same with fGetValue. Now let's reuse the example above
 * to show this. Function call
 *	CfgSetValue(h, "server configuration", "ip", "192.168.1.11", 1, 1);
 * sets the ip address to 192.168.1.11 in server configuration section,
 * while function call
 *	CfgSetValue(h, "colors", NULL, "black", 3, 1);
 * sets the third item in colors section, that is yellow, to black
 * color.
 * And function call
 *	CfgSetValue(h, "clients", "client", "203.93.112.1", 2, 1);
 * sets the second client's address from 132.108.58.51 to 203.93.112.1.
 */
int CfgSetValue(unsigned long h, const char *section, const char *id, const char *value, int index, int section_index);

/*
 * This function returns the number of 'id's defined in 'section'
 * when 'id' is not NULL.
 *
 * If 'id' equals NULL the specified section should be a section
 * which defined some tokens with type TYPE_TOKEN_VECTOR. And the
 * return value indicates the number of tokens within this section.
 *
 * If 'section_index' equals zero, we'll return the count of sections
 * with section name being the same with parameter 'section'.
 */
DLL_APP int CfgGetCount(unsigned long h, const char *section, const char *id, int section_index);

/*
 * Get the sequence number of a 'section', or an 'id'.
 *
 * This function returns index of the first appearance of 'id' in
 * 'section'. But if 'id' is NULL, the return value indicates which
 * section the 'section' is.
 *
 * We may have several 'id's in 'section' and they may be discrete.
 * With the help of parameter 'start', we can easily find out the index
 * of each of the 'id's.
 *
 * For example, we call this function with 'start' being zero, find out
 * the first index of the first 'id', then let 'start' being the newly
 * returned index, then call this function again, then what will be
 * returned? The INDEX of the next 'id'! See?
 */
int CfgGetIndex(unsigned long h, const char *section, const char *id, int start, int section_index);

/*
 * How many sections are in the configuration file? Go to find
 * it out, using CfgGetSectionNum().
 */
int CfgGetSectionNum(unsigned long h);
int CfgGetUniqueSectionNum(unsigned long h);

/*
 * What's the name of the third section?
 * The answer is CfgGetSectionByNum!
 */
int CfgGetSectionByNum(unsigned long h, int seq, char *value);
int CfgGetUniqueSectionByNum(unsigned long h, int seq, char *value);

/*
 * Function CfgAddToken adds a section or a token exactly after the item
 * which has the corresponding type and the index number equals to
 * 'where'. The newly added item will be the first one by default.
 *
 * When using this function to add a section, only parameter 'section'
 * and 'where' are used. 'where' means that we should add the section
 * exactly before the 'where'th section.
 *
 * If 'where' equals -1, then the newly added item will be the last.
 */
int CfgAddToken(unsigned long h, const char *section, int where, const char *id, const char *value, int section_index);

/*
 * Function CfgAddComment adds comment line to configuration file. If
 * both section and id are all NULL pointer, comment will be added
 * after the 'where' line. If both section and id are all non-NULL
 * pointer, comment will be added before the 'where'th definition
 * of 'id' in 'section'. If section isn't NULL and id is NULL, comment
 * will be added before the definition of 'section'. All other
 * combinations of parameters will cause failure.
 * By the way, a blank line other than comment line will be added
 * if parameter comment is NULL pointer.
 */
int CfgAddComment(unsigned long h, const char *comment, int where, const char *section, const char *id, int section_index);

/*
 * If id is a NULL pointer, CfgDelToken deletes a TYPE_TOKEN_VECTOR token
 * with index from the specified section. If id isn't NULL, CfgDelToken
 * deletes the 'id = value' pattern found in the specified section.
 */
int CfgDelToken(unsigned long h, const char *section, const char *id, int index, int section_index);

/*
 * To prevent you from deleting a whole section accidently, it's
 * defined as a separate function.
 */
int CfgDelSection(unsigned long h, const char *section, int section_index);

DLL_APP int CfgExistSection(unsigned long h, const char *section, int section_index);

/*
 * Do you really need comments about this function here?
 */
int CfgTestMe(unsigned long h);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_CCONFIG_30473C2D_744CDECE_426C0B80_H */
