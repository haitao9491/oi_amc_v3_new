/*
 *
 * cconfig.c - Configuration file manipulation tools.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#else
#pragma warning(disable : 4996 ; disable : 4311  ; disable : 4312  ; disable : 4267  ; disable : 4244 )
#endif
#include <string.h>
#include "os.h"
#include "cconfig.h"

unsigned long
CfgInitialize(const char *fname)
{
	stCConfig *cfg = NULL;
	char       caBakFilename[148];

	if((fname == NULL) || (strlen(fname) >= sizeof(cfg->filename))) {
		return 0ul;
	}

	cfg = malloc(sizeof(stCConfig));
	if(cfg == NULL)
		return 0ul;

	cfg->iIAmReady      = 0;
	cfg->lines          = NULL;
	cfg->sections       = NULL;
	cfg->iDirtyFlag     = 0;
	cfg->iWriteNow      = 1;
	cfg->iCaseSensitive = 1;
	cfg->iTotalSections = 0;
	cfg->iTokensPerLine = 0;
	strcpy(cfg->filename, fname);

	if(CfgLoadConfiguration((unsigned long)cfg, cfg->filename, 0) == -1) {
		CfgInvalidate((unsigned long)cfg);
		return 0ul;
	}

	cfg->iIAmReady = 1;

	/* Make a backup copy of the configuration file. */
	memset(caBakFilename, 0, sizeof(caBakFilename));
	sprintf(caBakFilename, "%s.bak", cfg->filename);
	CfgDump((unsigned long)cfg, caBakFilename);

	return (unsigned long)cfg;
}

unsigned long CfgInitializeString(const char *cfgstr)
{
	stCConfig *cfg = NULL;

	if (cfgstr == NULL)
		return 0ul;

	cfg = malloc(sizeof(stCConfig));
	if(cfg == NULL)
		return 0ul;

	memset(cfg, 0, sizeof(stCConfig));
	cfg->iWriteNow      = 1;
	cfg->iCaseSensitive = 1;

	if (CfgLoadConfiguration((unsigned long)cfg, cfgstr, 1) == -1) {
		CfgInvalidate((unsigned long)cfg);
		return 0ul;
	}

	cfg->iIAmReady = 1;

	return (unsigned long)cfg;
}

int
CfgInvalidate(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;

	if(cfg->iIAmReady && cfg->iDirtyFlag && (CfgSaveMeToFile(h) == -1)) {
		/* Dump to file failed. */
	}

	CfgReleaseMemory(h);

	free(cfg);

	return 0;
}

int
CfgReady(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;

	return(cfg->iIAmReady);
}

int
CfgSetWriteNow(unsigned long h, int writenow)
{
	stCConfig *cfg = (stCConfig *)h;
	int        oldflag = cfg->iWriteNow;

	cfg->iWriteNow = (writenow ? 1 : 0);

	return(oldflag);
}

int
CfgSetCaseSensitive(unsigned long h, int yes)
{
	stCConfig *cfg = (stCConfig *)h;
	int        oldflag = cfg->iCaseSensitive;

	cfg->iCaseSensitive = (yes ? 1 : 0);

	return(oldflag);
}

int
CfgSetColumn(unsigned long h, int column)
{
	stCConfig *cfg = (stCConfig *)h;
	int        oldcolumn = cfg->iTokensPerLine;

	cfg->iTokensPerLine = (column > 0 ? column : 0);

	return(oldcolumn);
}

int
CfgReload(unsigned long h, const char *path, int flag)
{
	stCConfig *cfg = (stCConfig *)h;

	CfgReleaseMemory(h);

	cfg->iIAmReady      = 0;
	cfg->lines          = NULL;
	cfg->sections       = NULL;
	cfg->iDirtyFlag     = 0;
	cfg->iWriteNow      = 1;
	cfg->iCaseSensitive = 0;
	cfg->iTotalSections = 0;
	cfg->iTokensPerLine = 0;

	if(flag && (path == NULL)) {
		return(-1);
	}

	if(!flag && (path != NULL)) {
		if(strlen(path) >= sizeof(cfg->filename)) {
			return(-1);
		}
		strcpy(cfg->filename, path);
	}

	if(CfgLoadConfiguration(h, flag ? path : cfg->filename, flag) == -1) {
		return(-1);
	}

	cfg->iIAmReady  = 1;
	return(0);
}

int
CfgSaveMeToFile(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;
	char       caTempFilename[148];

	if(!cfg->iIAmReady)
		return(-1);

	if(cfg->filename[0] == '\0')
		return(-1);

	memset(caTempFilename, 0, sizeof(caTempFilename));
	sprintf(caTempFilename, "%s.tmp", cfg->filename);
	if(CfgDump(h, caTempFilename) == -1) {
		return(-1);
	}

	if(unlink(cfg->filename) == -1) {
		return(-1);
	}

	if(rename(caTempFilename, cfg->filename) == -1) {
		return(-1);
	}

	return(0);
}

#define fIsDelimiter(c)	(((c) == ' ') || ((c) == '\t'))

#define STH_WRONG_EXIT(msg) { \
	if(!cflag) \
		fclose(fp); \
	return(-1); \
}

#define ADD_LINE { \
	if((line = (stLine *)malloc(sizeof(stLine))) == NULL) \
		STH_WRONG_EXIT("memory allocation fail"); \
	line->next   = NULL; \
	line->prev   = NULL; \
	line->token  = NULL; \
	line->etoken = NULL; \
	if(curline == NULL) \
		cfg->lines = curline = line; \
	else { \
		curline->next = line; \
		line->prev    = curline; \
		curline       = line; \
	} \
	line     = NULL; \
	curtoken = NULL; \
}

static __inline int ADD_TOKEN_FUNC(stLine *curline, stToken **curtoken,
		int Type, int Index,
		char *Name, int Namelen, char *Value, int Valuelen)
{
	stToken *token;

	if((token = (stToken *)malloc(sizeof(stToken))) == NULL)
		return -1;

	token->type  = Type;
	token->index = Index;

	if(!Name)
		token->name = NULL;
	else {
		if(Namelen <= 0)
			return -1;
		if((token->name  = (char *)malloc(Namelen+1)) == NULL)
			return -1;
		memcpy(token->name, Name, Namelen);
		token->name[Namelen] = '\0';
	}

	if(!Value)
		token->value = NULL;
	else {
		if(Valuelen <= 0)
			return -1;
		if((token->value  = (char *)malloc(Valuelen+1)) == NULL)
			return -1;
		memcpy(token->value, Value, Valuelen);
		token->value[Valuelen] = '\0';
	}

	token->next  = NULL;
	token->snext = NULL;
	token->line  = NULL;

	if(*curtoken  == NULL)
		curline->token = *curtoken = token;
	else {
		(*curtoken)->next = token;
		*curtoken       = token;
	}

	return 0;
}

#define ADD_TOKEN(Type, Index, Name, Namelen, Value, Valuelen) \
	if(ADD_TOKEN_FUNC(curline, &curtoken, \
			Type, Index, Name, Namelen, Value, Valuelen) < 0) \
		STH_WRONG_EXIT("Error add token")

int
CfgLoadConfiguration(unsigned long h, const char *path, int cflag)
{
	stCConfig *cfg = (stCConfig *)h;
	char       caLine[1024], caTempLine[1024], caTempLine2[1024];
	char      *pcEqualSign;
	int        iEqualSignPos, iSectionDefined;
	int        iTokenScalarDefined, iTokenVectorDefined;
	int        i, j, k, l, m, flag, iStart, iEnd, iTokenEnd, iIndex, rptr;
	stToken   *token, *curtoken, *ptoken = NULL, *ntoken;
	stLine    *line, *curline;
	FILE      *fp = NULL;

	if(!cflag && ((fp = fopen(path, "r"))) == NULL) {
		if((fp = fopen(path, "w+")) == NULL) {
			return(-1);
		}
		fclose(fp);
		if((fp = fopen(path, "r")) == NULL) {
			return(-1);
		}
	}

	curline  = cfg->lines;
	iSectionDefined = 0;
	iTokenScalarDefined = iTokenVectorDefined = 1;
	iIndex = 1;
	rptr = 0;
	while((!cflag && !feof(fp)) ||
	      ( cflag && (*(path + rptr)))) {
		memset(caLine, 0, sizeof(caLine));

		if(!cflag) {
			if(fgets(caLine, sizeof(caLine), fp) == NULL)
				continue;
		}
		else {
			int xx = rptr, cc = 0;

			while((*(path + xx)) && ((*(path + xx)) != '\n')) {
				xx++;
				cc++;
			}
			if ((*(path + xx)) == '\n') {
				xx++;
				cc++;
			}
			if (cc) {
				xx = (cc < (int)sizeof(caLine)) ? cc : sizeof(caLine) - 1;
				memcpy(caLine, path + rptr, xx);
				rptr += xx;
			}
			else
				continue;
		}

		iStart = 0;
		if((iEnd = (strlen(caLine) - 1)) < 0)
			STH_WRONG_EXIT("get line fail");
		while ((iEnd >= 0) && ((caLine[iEnd]==0x0d) || (caLine[iEnd]==0x0a))) {
			caLine[iEnd--] = '\0';
		}
		while((iEnd >= iStart) && fIsDelimiter(caLine[iEnd]))
			iEnd--;

		ADD_LINE;

		/* Is it a blank line? */
		if(iStart > iEnd) {
			ADD_TOKEN(TYPE_BLANK, 0, NULL, 0, NULL, 0);
			continue;
		}

		/* Check whether the line is comment. */
		if(caLine[0] == '#') {
			ADD_TOKEN(TYPE_COMMENT, 0, caLine, iEnd+1, NULL, 0);
			continue;
		}

		while((iStart <= iEnd) && fIsDelimiter(caLine[iStart]))
			iStart++;

		/* Check whether the line is the definition of section. */
		if((caLine[iStart] == '[') && (caLine[iEnd] == ']')) {
			i = iStart + 1;
			j = iEnd - 1;
			while((i <= iEnd) && fIsDelimiter(caLine[i]))
				i++;
			while((j >= iStart) && fIsDelimiter(caLine[j]))
				j--;
			for(flag = 0, l = 0, k = i; k <= j; k++) {
				if(!fIsDelimiter(caLine[k])) {
					flag = 0;
					caTempLine[l++] = caLine[k];
				}
				else {
					if(flag == 0) {
						caTempLine[l++] = ' ';
						flag = 1;
					}
				}
			}
			ADD_TOKEN(TYPE_SECTION, 1, caTempLine, l, NULL, 0);
			if(!iTokenScalarDefined && !iTokenVectorDefined) {
				/* The previous section is empty */
			}
			iIndex = iSectionDefined = 1;
			iTokenScalarDefined = iTokenVectorDefined = 0;
			continue;
		}

		if(iSectionDefined != 1)
			STH_WRONG_EXIT("define section first");

		pcEqualSign = strchr(caLine, '=');
		if(pcEqualSign == NULL) {
			/* no equal sign found. So this line should likes this:
			 *	[idenfifier[ \t]*]*
			 */
			if(iTokenScalarDefined == 1)
				STH_WRONG_EXIT("Cannot mix type scalar and vector together");

			while(iStart <= iEnd) {
				iTokenEnd = iStart;
				while((iTokenEnd <= iEnd) && !fIsDelimiter(caLine[iTokenEnd]))
					iTokenEnd++;
				ADD_TOKEN(TYPE_TOKEN_VECTOR, iIndex, NULL, 0, &caLine[iStart], iTokenEnd-iStart);
				iIndex++;
				while((iTokenEnd <= iEnd) && fIsDelimiter(caLine[iTokenEnd]))
					iTokenEnd++;
				iStart = iTokenEnd;
			}

			/* If everything is ok, continue to process the next
			 * line.
			 */
			iTokenVectorDefined = 1;
			continue;
		}

		if(iTokenVectorDefined == 1)
			STH_WRONG_EXIT("Cannot mix type scalar and vector together");

		iEqualSignPos = pcEqualSign - caLine;
		pcEqualSign   = strrchr(caLine, '=');
		if(pcEqualSign == NULL)
			STH_WRONG_EXIT("I don't know why this situation occur");
		if((pcEqualSign - caLine) != iEqualSignPos)
			STH_WRONG_EXIT("Two equal sign in one line");
		i = iEqualSignPos - 1;
		j = iEqualSignPos + 1;
		while((i >= iStart) && fIsDelimiter(caLine[i]))
			i--;
		while((j <= iEnd) && fIsDelimiter(caLine[j]))
			j++;
		for(flag = 0, l = 0, k = iStart; k <= i; k++) {
			if(!fIsDelimiter(caLine[k])) {
				flag = 0;
				caTempLine[l++] = caLine[k];
			}
			else {
				if(flag == 0) {
					caTempLine[l++] = ' ';
					flag = 1;
				}
			}
		}
		for(flag = 0, m = 0, k = j; k <= iEnd; k++) {
			if(!fIsDelimiter(caLine[k])) {
				flag = 0;
				caTempLine2[m++] = caLine[k];
			}
			else {
				if(flag == 0) {
					caTempLine2[m++] = ' ';
					flag = 1;
				}
			}
		}
		ADD_TOKEN(TYPE_TOKEN_SCALAR, 1, caTempLine, l, caTempLine2, m);
		iTokenScalarDefined = 1;
	}

	if(!cflag)
		fclose(fp);

	/* Let each token within a line has a pointer to the line. */
	for(curline = cfg->lines; curline != NULL; curline = curline->next) {
		curtoken = curline->token;
		while(curtoken != NULL) {
			curtoken->line = curline;
			curtoken = curtoken->next;
		}
	}

	/* count the number of sections, and save the respective line pointer
	 * in variable 'sections'.
	 */
	for(i = 0, curline = cfg->lines; curline != NULL; curline = curline->next) {
		token = curline->token;
		if((token != NULL) && (token->type == TYPE_SECTION))
			i++;
	}
	cfg->iTotalSections = i;
	cfg->sections = (stLine **)malloc(sizeof(stLine *) * (cfg->iTotalSections + 1));
	if(cfg->sections == NULL) {
		return(-1);
	}
	for(j = 0, curline = cfg->lines; curline != NULL; curline = curline->next) {
		token = curline->token;
		if((token != NULL) && (token->type == TYPE_SECTION)) {
			*(cfg->sections + j) = curline;
			j++;
		}
	}
	*(cfg->sections + j) = NULL;

	/* Now sections with same name are allowed, so we needn't this check
	 * any more. Instead, we'll calculate the index of each section.
	 */
	/*
	for(j = 1; j < cfg->iTotalSections; j++) {
		for(k = 0; k < j; k++) {
			if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections + k))->token->name, (*(cfg->sections + j))->token->name)) {
				return(-1);
				break;
			}
		}
	}
	*/
	for(j = 1; j < cfg->iTotalSections; j++) {
		for(k = 0; k < j; k++) {
			if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections + k))->token->name, (*(cfg->sections + j))->token->name)) {
				(*(cfg->sections + j))->token->index++;
			}
		}
	}

	/* now set each section's etoken pointer. */
	for(i = 0; i < cfg->iTotalSections; i++) {
		curline  = *(cfg->sections+i);
		while((curline != NULL) && (curline != *(cfg->sections+i+1))) {
			token = curline->token;
			if((token == NULL) || !fIsToken(token->type)) {
				curline = curline->next;
				continue;
			}
			(*(cfg->sections+i))->etoken = token;
			break;
		}
	}

	/* now link the tokens within a section together. */
	for(i = 0; i < cfg->iTotalSections; i++) {
		curline  = *(cfg->sections+i);
		j        = 1;
		while((curline != NULL) && (curline != *(cfg->sections+i+1))) {
			token = curline->token;
			if((token == NULL) || !fIsToken(token->type)) {
				curline = curline->next;
				continue;
			}
			if(j == 1) {
				ptoken = token;
				j      = 0;
			}
			else {
				ptoken->snext = token;
				ptoken        = token;
			}

			while(token->next != NULL) {
				token         = token->next;
				ptoken->snext = token;
				ptoken        = token;
			}
			curline = curline->next;
		}
	}

	/* now compute the index parameter of each TYPE_TOKEN_SCALAR token. */
	for(i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections + (++i))) {
		token = (*(cfg->sections+i))->etoken;
		if((token == NULL) || (token->type != TYPE_TOKEN_SCALAR))
			continue;
		while(token != NULL) {
			ntoken = token->snext;
			while(ntoken != NULL) {
				if(!STRCASECMP(cfg->iCaseSensitive, token->name, ntoken->name))
					ntoken->index++;
				ntoken = ntoken->snext;
			}
			token = token->snext;
		}
	}

	return(0);
}

int
CfgDump(unsigned long h, const char *path)
{
	stCConfig *cfg = (stCConfig *)h;
	FILE     *fpDump;
	stLine   *l;
	stToken  *t;
	int       iMaxTokenLen, iTokens, len, i;

	if(!cfg->iIAmReady)
		return(-1);

	if((fpDump = fopen(path, "w")) == NULL) {
		return(-1);
	}

	for(l = cfg->lines; l != NULL; l = l->next) {
		if((t = l->token) == NULL)
			continue;
		if(t->type == TYPE_TOKEN_VECTOR) {
			for(iMaxTokenLen = 0; t != NULL; t = t->next) {
				len = strlen(t->value);
				if(iMaxTokenLen < len)
					iMaxTokenLen = len;
			}
			iMaxTokenLen++;
			if(cfg->iTokensPerLine > 0)
				iTokens = cfg->iTokensPerLine;
			else {
				iTokens = 70 / iMaxTokenLen;
				if(iTokens > 16)
					iTokens = 16;
				if(iTokens < 1)
					iTokens = 1;
			}
			for(t = l->token, i = 0; t != NULL; t = t->next) {
				fprintf(fpDump, "%-*s", iMaxTokenLen, t->value);
				if(!(++i % iTokens)) {
					fprintf(fpDump, "\n");
					i = 0;
				}
			}
			if(i)
				fprintf(fpDump, "\n");
			continue;
		}
		else if(t->type == TYPE_COMMENT)
			fprintf(fpDump, "%s\n", t->name);
		else if(t->type == TYPE_BLANK)
			fprintf(fpDump, "\n");
		else if(t->type == TYPE_SECTION)
			fprintf(fpDump, "[%s]\n", t->name);
		else if(t->type == TYPE_TOKEN_SCALAR)
			fprintf(fpDump, "%-15s = %s\n", t->name, t->value);
		else {
		}
	}

	fclose(fpDump);
	cfg->iDirtyFlag = 0;

	return(0);
}

void
CfgReleaseMemory(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *curline, *nextline;
	stToken   *curtoken, *nexttoken;

	curline  = cfg->lines;
	while(curline != NULL) {
		nextline = curline->next;
		curtoken = curline->token;
		while(curtoken != NULL) {
			nexttoken = curtoken->next;
			free(curtoken->name);
			free(curtoken->value);
			free(curtoken);
			curtoken  = nexttoken;
		}
		free(curline);
		curline  = nextline;
	}
	cfg->lines = NULL;

	if(cfg->sections != NULL) {
		free(cfg->sections);
	}
	cfg->sections = NULL;
}

int
CfgGetValue(unsigned long h, const char *section, const char *id, char *value, int index, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *line;
	stToken   *token;
	int        i;

	if(!cfg->iIAmReady)
		return(-1);
	if(section == NULL)
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(-1);

	/* Check whether the specified section exists. */
	for(i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections+(++i))) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, line->token->name) && (line->token->index == section_index))
			break;
	}
	if(line == NULL) {
		/* specified section not found. */
		return(-1);
	}

	if((token = line->etoken) == NULL) {
		/* empty section. */
		return(-1);
	}

	while(token != NULL) {
		if(id == NULL) {
			if((token->type == TYPE_TOKEN_VECTOR) &&
			   (token->index == index)) {
				strcpy(value, token->value);
				return(0);
			}
		}
		else {
			if((token->type == TYPE_TOKEN_SCALAR) &&
			   (token->index == index) &&
			   !STRCASECMP(cfg->iCaseSensitive, token->name, id)) {
				strcpy(value, token->value);
				return(0);
			}
		}
		token = token->snext;
	}

	return(-1);
}

int
CfgGetValueInt(unsigned long h, const char *section, const char *id, int index, int section_index)
{
	char  value[256];

	if(CfgGetValue(h, section, id, value, index, section_index) == -1)
		return(-1);

	return(atoi(value));
}

int
CfgSetValue(unsigned long h, const char *section, const char *id, const char *value, int index, int section_index)
{
	stCConfig    *cfg = (stCConfig *)h;
	stLine       *line;
	stToken      *token;
	int           i;
	unsigned int  len;

	if(!cfg->iIAmReady)
		return(-1);
	if((section == NULL) || (value == NULL))
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(-1);

	/* Check whether the specified section exists. */
	for(i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections+(++i))) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, line->token->name) && (line->token->index == section_index))
			break;
	}
	if(line == NULL) {
		/* specified section not found. */
		return(-1);
	}

	if((token = line->etoken) == NULL) {
		/* empty section. */
		return(-1);
	}

	while(token != NULL) {
		if(((id == NULL) && (token->type == TYPE_TOKEN_VECTOR) &&
		    (token->index == index))
		   ||
		   ((id != NULL) && (token->type == TYPE_TOKEN_SCALAR) &&
		    (token->index == index) && !STRCASECMP(cfg->iCaseSensitive, token->name, id))) {
			if(!STRCASECMP(cfg->iCaseSensitive, token->value, value))
				return(0);
			len = strlen(value);
			if(len <= strlen(token->value))
				strcpy(token->value, value);
			else {
				free(token->value);
				token->value = (char *)malloc(len + 1);
				if(token->value == NULL) {
					return(-1);
				}
				memcpy(token->value, value, len);
				token->value[len] = '\0';
			}
			if(cfg->iWriteNow)
				CfgSaveMeToFile(h);
			else
				cfg->iDirtyFlag = 1;
			return(0);
		}
		token = token->snext;
	}

	return(-1);
}

int
CfgGetCount(unsigned long h, const char *section, const char *id, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *line;
	stToken   *token;
	int        index, i;

	/* in case of non-vital error condition, we should return 0. */

	if(!cfg->iIAmReady)
		return(-1);
	if(section == NULL)
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(0);

	if(section_index == 0) {
		/* count sections. */
		for(index = 0, i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections+(++i))) {
			if(!STRCASECMP(cfg->iCaseSensitive, section, line->token->name))
				index = line->token->index;
		}
		return(index);
	}

	/* Check whether the specified section exists. */
	for(i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections+(++i))) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, line->token->name) && (line->token->index == section_index))
			break;
	}
	if(line == NULL) {
		/* specified section not found. */
		return(0);
	}

	if((token = line->etoken) == NULL) {
		/* empty section. */
		return(0);
	}

	if(id == NULL) {
		if(token->type != TYPE_TOKEN_VECTOR)	/* token type confused. */
			return(-1);
		while(token->snext != NULL)
			token = token->snext;
		return(token->index);
	}
	else {
		if(token->type == TYPE_TOKEN_SCALAR) {
			for(index = 0; token != NULL; token = token->snext) {
				if(!STRCASECMP(cfg->iCaseSensitive, token->name, id))
					index = token->index;
			}
			return(index);
		}
		if(token->type == TYPE_TOKEN_VECTOR) {
			for(index = 0; token != NULL; token = token->snext) {
				if(!STRCASECMP(cfg->iCaseSensitive, token->value, id))
					index++;
			}
			return(index);
		}
	}

	return(-1);
}

int
CfgGetIndex(unsigned long h, const char *section, const char *id, int start, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *line;
	stToken   *token;
	int        index, i;

	if(!cfg->iIAmReady)
		return(-1);
	if(section == NULL)
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(-1);
	for(i = 0, line = *cfg->sections; line != NULL; line = *(cfg->sections+(++i))) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, line->token->name) && (line->token->index == section_index))
			break;
	}
	if(line == NULL)
		return(-1);
	if(id == NULL)
		return(i+1);
	if((token = line->etoken) == NULL)
		return(-1);

	if(token->type == TYPE_TOKEN_SCALAR) {
		for(index = 1; token != NULL; token = token->snext, index++) {
			if(!STRCASECMP(cfg->iCaseSensitive, token->name, id) && (index > start))
				return(index);
		}
		return(-1);
	}
	if(token->type == TYPE_TOKEN_VECTOR) {
		for(index = 1; token != NULL; token = token->snext, index++) {
			if(!STRCASECMP(cfg->iCaseSensitive, token->value, id) && (index > start))
				return(index);
		}
		return(-1);
	}

	return(-1);
}

int
CfgGetSectionNum(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;
	return(cfg->iTotalSections);
}

int
CfgGetUniqueSectionNum(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;
	int  i, j;

	if(!cfg->iIAmReady)
		return(-1);

	for(i = 0, j = 0; i < cfg->iTotalSections; i++)
		if((*(cfg->sections + i))->token->index == 1)
			j++;

	return(j);
}

int
CfgGetSectionByNum(unsigned long h, int seq, char *value)
{
	stCConfig *cfg = (stCConfig *)h;
	if(!cfg->iIAmReady)
		return(-1);
	if(value == NULL)
		return(-1);

	if((seq < 1) || (seq > cfg->iTotalSections))
		return(-1);

	strcpy(value, (*(cfg->sections + seq - 1))->token->name);

	return(0);
}

int
CfgGetUniqueSectionByNum(unsigned long h, int seq, char *value)
{
	stCConfig *cfg = (stCConfig *)h;
	if(!cfg->iIAmReady)
		return(-1);
	if(value == NULL)
		return(-1);

	if((seq < 1) || (seq > cfg->iTotalSections))
		return(-1);

	if((*(cfg->sections + seq - 1))->token->index == 1) {
		strcpy(value, (*(cfg->sections + seq - 1))->token->name);
		return(0);
	}

	return(-1);
}

int
CfgAddToken(unsigned long h, const char *section, int where, const char *id, const char *value, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *line, *curline, **tmpsections;
	stToken   *token, *curtoken;
	int       i = -1, iWhat, iWhere;
	int       j = 0;

	if(!cfg->iIAmReady)
		return(-1);
	if(section == NULL)
		return(-1);

	if(value == NULL) {
		iWhat = TYPE_SECTION;
		i     = cfg->iTotalSections;
	}
	else {
		if(id == NULL) {
			iWhat = TYPE_TOKEN_VECTOR;
			i     = CfgGetCount(h, section, NULL, section_index);
		}
		else {
			iWhat = TYPE_TOKEN_SCALAR;
			i     = CfgGetCount(h, section, id, section_index);
		}
	}

	if(i == -1)
		iWhere = 0;
	else {
		if((where < 0) || (where > i))
			iWhere = i;
		else
			iWhere = where;
	}

	token = NULL;
	line  = NULL;
	if((token = (stToken *)malloc(sizeof(stToken))) == NULL) {
		return(-1);
	}
	token->type    = iWhat;
	token->index   = (iWhat == TYPE_SECTION) ? 1 : iWhere + 1;
	if(iWhat == TYPE_TOKEN_VECTOR)
		token->name = NULL;
	else {
		if(iWhat == TYPE_SECTION)
			i = strlen(section);
		else
			i = strlen(id);
		token->name = (char *)malloc(i + 1);
		if(token->name == NULL) {
			free(token);
			return(-1);
		}
		memcpy(token->name, (iWhat == TYPE_SECTION) ? section : id, i);
		token->name[i] = '\0';
	}
	if(iWhat == TYPE_SECTION)
		token->value  = NULL;
	else {
		i = strlen(value);
		token->value = (char *)malloc(i + 1);
		if(token->value == NULL) {
			free(token->name);	free(token);
			return(-1);
		}
		memcpy(token->value, value, i);
		token->value[i] = '\0';
	}
	token->next = token->snext = NULL;

	if((line = (stLine *)malloc(sizeof(stLine))) == NULL) {
		free(token->name);	free(token->value);
		free(token);
		return(-1);
	}
	line->next   = NULL;
	line->prev   = NULL;
	line->token  = token;
	line->etoken = NULL;

	token->line  = line;

#define SUCCESS_EXIT { \
	if(cfg->iWriteNow) \
		CfgSaveMeToFile(h); \
	else \
		cfg->iDirtyFlag = 1; \
	return(0); \
}

#define HCLERROR_EXIT(msg) { \
	free(token->name); \
	free(token->value); \
	free(token); \
	free(line); \
	return(-1); \
}

	if(iWhat == TYPE_SECTION) {
		tmpsections = (stLine **)malloc(sizeof(stLine *) * (cfg->iTotalSections + 2));
		if(tmpsections == NULL) {
			HCLERROR_EXIT("memory allocation.");
		}

		/* Now we support same name sections! */
		/*
		for(i = 0; i < cfg->iTotalSections; i++) {
			if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections+i))->token->name, token->name)) {
				free(tmpsections);
				HCLERROR_EXIT("Section already exists.");
			}
		}
		*/

		if(cfg->iTotalSections == 0) {
			if(cfg->lines == NULL) {
				cfg->lines = line;
			}
			else {
				curline = cfg->lines;
				while(curline->next != NULL)
					curline = curline->next;
				curline->next = line;
				line->prev = curline;
			}
		}
		else {
			if(cfg->lines == *(cfg->sections+iWhere)) {
				cfg->lines = line;
				line->next = *(cfg->sections+iWhere);
				(*(cfg->sections+iWhere))->prev = line;
			}
			else {
				curline = cfg->lines;
				while(curline->next != *(cfg->sections+iWhere))
					curline = curline->next;
				while((curline != NULL) &&
				      ((curline->token->type == TYPE_COMMENT) ||
				       (curline->token->type == TYPE_BLANK)))
					curline = curline->prev;
				if(curline == NULL) {
					cfg->lines->prev = line;
					line->next = cfg->lines;
					cfg->lines = line;
				}
				else {
					line->prev = curline;
					line->next = curline->next;
					if(curline->next != NULL)
						curline->next->prev = line;
					curline->next = line;
				}
			}
		}
		cfg->iTotalSections++;
		for(i = 0; i < iWhere; i++)
			*(tmpsections+i) = *(cfg->sections+i);
		*(tmpsections+iWhere) = line;
		for(i = iWhere+1; i < cfg->iTotalSections; i++)
			*(tmpsections+i) = *(cfg->sections+i-1);
		*(tmpsections+cfg->iTotalSections) = NULL;
		free(cfg->sections);
		cfg->sections = tmpsections;

		/* Now recalculate the sections' index. */
		for(i = 0; i < iWhere; i++) {
			if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name))
				j = (*(cfg->sections+i))->token->index;
		}
		(*(cfg->sections+i))->token->index += j;
		for(i = iWhere + 1; i < cfg->iTotalSections; i++) {
			if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name))
				(*(cfg->sections+i))->token->index++;
		}

		SUCCESS_EXIT;
	}

	if((cfg->sections == NULL) || (cfg->lines == NULL))
		HCLERROR_EXIT("unknown error.");
	for(i = 0; i < cfg->iTotalSections; i++) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name) && ((*(cfg->sections+i))->token->index == section_index))
			break;
	}
	if(i == cfg->iTotalSections)
		HCLERROR_EXIT("cannot find the section.");

	curline = *(cfg->sections + i);
	curtoken = curline->etoken;
	if((curtoken != NULL) && (curtoken->type != iWhat))
		HCLERROR_EXIT("cannot mix type scalar/vector in a section.");
	if((curtoken == NULL) && (iWhere != 0))
		HCLERROR_EXIT("No tokens in this section, where should be 0.");

	if(iWhat == TYPE_TOKEN_VECTOR) {
		if(curtoken == NULL) {
			line->next      = curline->next;
			line->prev      = curline;
			if(curline->next != NULL)
				curline->next->prev = line;
			curline->next   = line;
			curline->etoken = token;
			SUCCESS_EXIT;
		}
		if(iWhere == 0) {
			token->snext = curtoken;
			token->next  = curtoken;
			token->line  = curtoken->line;
			curtoken->line->token = token;
			curline->etoken = token;
			curtoken     = token;
			while((token = token->snext) != NULL)
				token->index++;
			free(line);
			SUCCESS_EXIT;
		}
		while((curtoken != NULL) && (curtoken->index != iWhere))
			curtoken = curtoken->snext;
		if(curtoken == NULL)
			HCLERROR_EXIT("Specified position not found.");
		token->snext = curtoken->snext;
		token->next  = curtoken->next;
		token->line  = curtoken->line;
		curtoken->snext = token;
		curtoken->next  = token;
		while((token = token->snext) != NULL)
			token->index++;
		free(line);
		SUCCESS_EXIT;
	}

	if(iWhat == TYPE_TOKEN_SCALAR) {
		if(curtoken == NULL) {
			line->next      = curline->next;
			line->prev      = curline;
			if(curline->next != NULL)
				curline->next->prev = line;
			curline->next   = line;
			curline->etoken = token;
			SUCCESS_EXIT;
		}
		if(iWhere == 0) {
			line->next      = curline->next;
			line->prev      = curline;
			if(curline->next != NULL)
				curline->next->prev = line;
			curline->next   = line;
			token->snext    = curline->etoken;
			curline->etoken = token;
			while((token = token->snext) != NULL) {
				if(!STRCASECMP(cfg->iCaseSensitive, id, token->name))
					token->index++;
			}
			SUCCESS_EXIT;
		}
		while((curline != NULL) && (curline != *(cfg->sections+i+1))) {
			if(curline->token->type == TYPE_TOKEN_SCALAR) {
				curtoken = curline->token;
				if(!STRCASECMP(cfg->iCaseSensitive, curtoken->name, id) && (curtoken->index == iWhere))
					break;
			}
			curline = curline->next;
		}
		if((curline == NULL) || (curline == *(cfg->sections+i+1)))
			HCLERROR_EXIT("Specified position not found.");

		line->next      = curline->next;
		line->prev      = curline;
		if(curline->next != NULL)
			curline->next->prev = line;
		curline->next   = line;
		token->snext    = curtoken->snext;
		curtoken->snext = token;
		while((token = token->snext) != NULL) {
			if(!STRCASECMP(cfg->iCaseSensitive, id, token->name))
				token->index++;
		}
		SUCCESS_EXIT;
	}

	HCLERROR_EXIT("Can program run to here?");
}

int
CfgAddComment(unsigned long h, const char *comment, int where, const char *section, const char *id, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *line, *curline;
	stToken   *token, *curtoken;
	int        i;

	if(!cfg->iIAmReady)
		return(-1);
	if(where < 0)
		return(-1);

	if((section == NULL) && (id == NULL)) {
		/* add after the 'where' line. */
		if((cfg->lines == NULL) || (where == 0))
			curline = NULL;
		else {
			curline = cfg->lines;
			for(i = 1; i < where; i++) {
				if(curline->next == NULL)
					break;
				curline = curline->next;
			}
		}
	}
	else if((section != NULL) && (id == NULL)) {
		/* add before the definition of this section. */
		for(i = 0; i < cfg->iTotalSections; i++) {
			if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections+i))->token->name, section) && ((*(cfg->sections+i))->token->index == section_index))
				break;
		}
		if(i == cfg->iTotalSections)
			return(-1);
		curline = (*(cfg->sections+i))->prev;
	}
	else if((section != NULL) && (id != NULL)) {
		/* add before the 'where'th definition of 'id' in 'section'. */
		for(i = 0; i < cfg->iTotalSections; i++) {
			if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections+i))->token->name, section) && ((*(cfg->sections+i))->token->index == section_index))
				break;
		}
		if(i == cfg->iTotalSections)
			return(-1);
		curline = *(cfg->sections+i);
		curtoken = curline->etoken;
		if((curtoken == NULL) || (curtoken->type != TYPE_TOKEN_SCALAR))
			return(-1);
		while(curtoken != NULL) {
			if((curtoken->index == where) && !STRCASECMP(cfg->iCaseSensitive, curtoken->name, id))
				break;
			curtoken = curtoken->snext;
		}
		if(curtoken == NULL)
			return(-1);
		curline = curtoken->line->prev;
	}
	else
		return(-1);

	if((token = (stToken *)malloc(sizeof(stToken))) == NULL) {
		return(-1);
	}
	token->type  = TYPE_COMMENT;
	token->index = 0;
	token->value = NULL;
	if(comment != NULL) {
		i = strlen(comment);
		token->name = (char *)malloc(i + 2);
		if(token->name == NULL) {
			free(token);
			return(-1);
		}
		memcpy(&token->name[1], comment, i);
		token->name[0] = '#';
		token->name[i+1] = '\0';
	}
	else {
		token->type = TYPE_BLANK;
		token->name = NULL;
	}

	token->next = token->snext = NULL;

	if((line = (stLine *)malloc(sizeof(stLine))) == NULL) {
		free(token->name);	free(token);
		return(-1);
	}
	line->token = token;
	line->etoken = NULL;
	line->next = line->prev = NULL;
	token->line = line;

	if(curline == NULL) {
		if(cfg->lines == NULL)
			cfg->lines = line;
		else {
			line->next = cfg->lines;
			cfg->lines->prev = line;
			cfg->lines = line;
		}
	}
	else {
		line->next = curline->next;
		line->prev = curline;
		if(curline->next != NULL)
			curline->next->prev = line;
		curline->next = line;
	}
	if(cfg->iWriteNow)
		CfgSaveMeToFile(h);
	else
		cfg->iDirtyFlag = 1;

	return(0);
}

int
CfgDelToken(unsigned long h, const char *section, const char *id, int index, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	int        i;
	stToken   *token, *ptoken, *pltoken;
	stLine    *line;

	if(!cfg->iIAmReady)
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(-1);
	if(section == NULL)
		return(-1);

	for(i = 0; i < cfg->iTotalSections; i++)
		if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name) && ((*(cfg->sections+i))->token->index == section_index))
			break;
	if(i == cfg->iTotalSections) {
		return(-1);
	}

	token = (*(cfg->sections+i))->etoken;
	if(token == NULL) {
		return(-1);
	}

	if(((id == NULL) && (token->type != TYPE_TOKEN_VECTOR)) ||
	   ((id != NULL) && (token->type != TYPE_TOKEN_SCALAR))) {
		return(-1);
	}

	ptoken = token;
	while(token != NULL) {
		if((id == NULL) && (token->index == index)) {
			/* we find it! */
			line = token->line;
			if(ptoken == token)
				(*(cfg->sections+i))->etoken = token->snext;
			else
				ptoken->snext = token->snext;
			pltoken = line->token;
			if(pltoken == token)
				line->token = token->next;
			else {
				while(pltoken != NULL) {
					if(pltoken->next == token)
						break;
					pltoken = pltoken->next;
				}
				if(pltoken == NULL)
					return(-1);
				pltoken->next = token->next;
			}
			free(token->name);
			free(token->value);
			free(token);
			if(line->token == NULL) {
				line->prev->next = line->next;
				if(line->next != NULL)
					line->next->prev = line->prev;
				free(line);
			}
			for(token = ptoken->snext; token != NULL; token = token->snext)
				token->index--;
			if(cfg->iWriteNow)
				CfgSaveMeToFile(h);
			else
				cfg->iDirtyFlag = 1;
			return(0);
		}
		if((id != NULL) && !STRCASECMP(cfg->iCaseSensitive, token->name, id) && (token->index == index)) {
			line = token->line;
			line->prev->next = line->next;
			if(line->next != NULL)
				line->next->prev = line->prev;
			if(ptoken == token)
				(*(cfg->sections+i))->etoken = token->snext;
			else
				ptoken->snext = token->snext;
			free(token->name);
			free(token->value);
			free(token);
			free(line);
			for(token=ptoken->snext;token!=NULL;token=token->snext)
				if(!STRCASECMP(cfg->iCaseSensitive, token->name, id))
					token->index--;
			if(cfg->iWriteNow)
				CfgSaveMeToFile(h);
			else
				cfg->iDirtyFlag = 1;
			return(0);
		}
		ptoken = token;
		token  = token->snext;
	}

	return(-1);
}

int
CfgDelSection(unsigned long h, const char *section, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine    *pline, *line, *nline, *stopline;
	stToken   *token, *ntoken;
	int        i, j;

	if(!cfg->iIAmReady)
		return(-1);
	if((cfg->sections == NULL) || (cfg->lines == NULL))
		return(-1);
	if(section == NULL)
		return(-1);
	if(cfg->iTotalSections < 1)
		return(-1);
	for(i = 0; i < cfg->iTotalSections; i++) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name) && ((*(cfg->sections+i))->token->index == section_index))
			break;
	}
	if(i == cfg->iTotalSections)
		return(-1);
	line  = *(cfg->sections + i);
	pline = line->prev;
	while((pline != NULL) &&
	      ((pline->token->type == TYPE_COMMENT) ||
	       (pline->token->type == TYPE_BLANK)))
		pline = pline->prev;

	stopline = line;
	while(stopline->next != *(cfg->sections+i+1))
		stopline = stopline->next;
	while((stopline != line) &&
	      ((stopline->token->type == TYPE_COMMENT) ||
	       (stopline->token->type == TYPE_BLANK)))
		stopline = stopline->prev;
	stopline = stopline->next;

	if(pline == NULL)
		line = cfg->lines;
	else
		line = pline->next;
	while(line != stopline) {
		nline = line->next;
		token = line->token;
		while(token != NULL) {
			ntoken = token->next;
			free(token->name);
			free(token->value);
			free(token);
			token  = ntoken;
		}
		free(line);
		line = nline;
	}
	if(pline != NULL) {
		pline->next = line;
		if(line != NULL)
			line->prev = pline;
	}
	else {
		cfg->lines = line;
		if(line != NULL)
			line->prev = NULL;
	}

	for(j = i; *(cfg->sections + j + 1) != NULL; j++)
		*(cfg->sections + j) = *(cfg->sections + j + 1);
	*(cfg->sections + j) = NULL;

	cfg->iTotalSections--;

	for(j = 0; j < cfg->iTotalSections; j++) {
		if(!STRCASECMP(cfg->iCaseSensitive, (*(cfg->sections + j))->token->name, section) && ((*(cfg->sections + j))->token->index > section_index)) {
			(*(cfg->sections + j))->token->index--;
		}
	}

	if(cfg->iWriteNow)
		CfgSaveMeToFile(h);
	else
		cfg->iDirtyFlag = 1;

	return(0);
}

int
CfgExistSection(unsigned long h, const char *section, int section_index)
{
	stCConfig *cfg = (stCConfig *)h;
	int        i;

	if(!cfg->iIAmReady)
		return(0);
	if(section == NULL)
		return(0);
	for(i = 0; i < cfg->iTotalSections; i++) {
		if(!STRCASECMP(cfg->iCaseSensitive, section, (*(cfg->sections+i))->token->name) && ((*(cfg->sections+i))->token->index == section_index))
			break;
	}
	if(i == cfg->iTotalSections)
		return(0);

	return(1);
}

int
CfgTestMe(unsigned long h)
{
	stCConfig *cfg = (stCConfig *)h;
	stLine     *line;
	stToken    *token;
	char        caType[6][32] = {"", "TYPE_SECTION", "TYPE_TOKEN_SCALAR",
	                             "TYPE_TOKEN_VECTOR", "TYPE_COMMENT",
	                             "TYPE_BLANK"};
	char        caBuffer[512];

	if(!cfg->iIAmReady)
		return(-1);

	for(line = cfg->lines; line != NULL; line = line->next) {
		printf("Line %p (token %p, etoken %p, prev %p, next %p).\n",
			line,line->token,line->etoken, line->prev, line->next);
		for(token=line->token; token!=NULL; token=token->next) {
			sprintf(caBuffer, "Token %p (next %p, snext %p, line %p).\n(type %s, index %d)\n name %p \"%s\"\nvalue %p \"%s\"",
				token, token->next, token->snext, token->line,
				caType[token->type], token->index,
				token->name, (token->name ? token->name : ""),
				token->value, (token->value?token->value:""));
			printf("%s\n", caBuffer);
		}
	}

	return(0);
}

