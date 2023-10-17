//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include <algorithm>
#include <cstdio>
#include "ac/string.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/dynobj/dynobj_manager.h"
#include "font/fonts.h"
#include "debug/debug_log.h"
#include "script/runtimescriptvalue.h"
#include "util/string_compat.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;
extern int longestline;

const char *CreateNewScriptString(const char *text)
{
    return (const char*)ScriptString::Create(text).Obj;
}

char *CreateNewScriptString(size_t buf_length)
{
    return (char*)ScriptString::Create(buf_length).Obj;
}


int String_IsNullOrEmpty(const char *thisString) 
{
    if ((thisString == nullptr) || (thisString[0] == 0))
        return 1;

    return 0;
}

const char* String_Copy(const char *srcString) {
    return CreateNewScriptString(srcString);
}

const char* String_Append(const char *thisString, const char *extrabit) {
    size_t new_len = strlen(thisString) + strlen(extrabit);
    char *buffer = CreateNewScriptString(new_len);
    strcpy(buffer, thisString);
    strcat(buffer, extrabit);
    return buffer;
}

const char* String_AppendChar(const char *thisString, int extraOne) {
    char chr[5]{};
    size_t chw = usetc(chr, extraOne);
    size_t new_len = strlen(thisString) + chw;
    char *buffer = CreateNewScriptString(new_len);
    sprintf(buffer, "%s%s", thisString, chr);
    return buffer;
}

const char* String_ReplaceCharAt(const char *thisString, int index, int newChar) {
    size_t len = ustrlen(thisString);
    if ((index < 0) || ((size_t)index >= len))
        quit("!String.ReplaceCharAt: index outside range of string");

    size_t off = uoffset(thisString, index);
    int uchar = ugetc(thisString + off);
    size_t remain_sz = strlen(thisString + off);
    size_t old_sz = ucwidth(uchar);
    char new_chr[5]{};
    size_t new_chw = usetc(new_chr, newChar);
    size_t new_len = off + remain_sz + new_chw - old_sz;
    char *buffer = CreateNewScriptString(new_len);
    memcpy(buffer, thisString, off);
    memcpy(buffer + off, new_chr, new_chw);
    memcpy(buffer + off + new_chw, thisString + off + old_sz, remain_sz - old_sz + 1);
    return buffer;
}

const char* String_Truncate(const char *thisString, int length) {
    if (length < 0)
        quit("!String.Truncate: invalid length");
    size_t strlen = ustrlen(thisString);
    if ((size_t)length >= strlen)
        return thisString;

    size_t sz = uoffset(thisString, length);
    char *buffer = CreateNewScriptString(sz);
    memcpy(buffer, thisString, sz);
    buffer[sz] = 0;
    return buffer;
}

const char* String_Substring(const char *thisString, int index, int length) {
    if (length < 0)
        quit("!String.Substring: invalid length");
    size_t strlen = ustrlen(thisString);
    if ((index < 0) || ((size_t)index > strlen))
        quit("!String.Substring: invalid index");
    size_t sublen = std::min((size_t)length, strlen - index);
    size_t start = uoffset(thisString, index);
    size_t end = uoffset(thisString + start, sublen) + start;
    size_t copysz = end - start;

    char *buffer = CreateNewScriptString(copysz);
    memcpy(buffer, thisString + start, copysz);
    buffer[copysz] = 0;
    return buffer;
}

int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive) {

    if (caseSensitive) {
        return strcmp(thisString, otherString);
    }
    else {
        return ustricmp(thisString, otherString);
    }
}

int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    if (caseSensitive) {
        return (strncmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
    else {
        return (ustrnicmp(thisString, checkForString, ustrlen(checkForString)) == 0) ? 1 : 0;
    }
}

int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive) {
    // NOTE: we need size in bytes here
    size_t thislen = strlen(thisString), checklen = strlen(checkForString);
    if (checklen > thislen)
        return 0;

    if (caseSensitive) 
    {
        return (strcmp(thisString + (thislen - checklen), checkForString) == 0) ? 1 : 0;
    }
    else 
    {
        return (ustricmp(thisString + (thislen - checklen), checkForString) == 0) ? 1 : 0;
    }
}

const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
{
    char resultBuffer[STD_BUFFER_SIZE] = "";
    size_t outputSize = 0; // length in bytes
    if (caseSensitive)
    {
        size_t lookForLen = strlen(lookForText);
        size_t replaceLen = strlen(replaceWithText);
        for (const char *ptr = thisString; *ptr; ++ptr)
        {
            if (strncmp(ptr, lookForText, lookForLen) == 0)
            {
                memcpy(&resultBuffer[outputSize], replaceWithText, replaceLen);
                outputSize += replaceLen;
                ptr += lookForLen - 1;
            }
            else
            {
                resultBuffer[outputSize] = *ptr;
                outputSize++;
            }
        }
    }
    else
    {
        size_t lookForLen = ustrlen(lookForText);
        size_t lookForSz = strlen(lookForText); // length in bytes
        size_t replaceSz = strlen(replaceWithText); // length in bytes
        const char *p_cur = thisString;
        for (int c = ugetxc(&thisString); *p_cur; p_cur = thisString, c = ugetxc(&thisString))
        {
            if (ustrnicmp(p_cur, lookForText, lookForLen) == 0)
            {
                memcpy(&resultBuffer[outputSize], replaceWithText, replaceSz);
                outputSize += replaceSz;
                thisString = p_cur + lookForSz;
            }
            else
            {
                usetc(&resultBuffer[outputSize], c);
                outputSize += ucwidth(c);
            }
        }
    }

    resultBuffer[outputSize] = 0; // terminate
    return CreateNewScriptString(resultBuffer);
}

const char* String_LowerCase(const char *thisString) {
    size_t len = strlen(thisString);
    char *buffer = CreateNewScriptString(len);
    memcpy(buffer, thisString, len);
    ustrlwr(buffer);
    return buffer;
}

const char* String_UpperCase(const char *thisString) {
    size_t len = strlen(thisString);
    char *buffer = CreateNewScriptString(len);
    memcpy(buffer, thisString, len);
    ustrupr(buffer);
    return buffer;
}

int String_GetChars(const char *texx, int index) {
    if ((index < 0) || (index >= ustrlen(texx)))
        return 0;
    return ugetat(texx, index);
}

int StringToInt(const char*stino) {
    return atoi(stino);
}

int StrContains (const char *s1, const char *s2) {
    VALIDATE_STRING (s1);
    VALIDATE_STRING (s2);
    char *tempbuf1 = ags_strdup(s1);
    char *tempbuf2 = ags_strdup(s2);
    ustrlwr(tempbuf1);
    ustrlwr(tempbuf2);

    char *offs = ustrstr(tempbuf1, tempbuf2);

    if (offs == nullptr)
    {
        free(tempbuf1);
        free(tempbuf2);
        return -1;
    }

    *offs = 0;
    int at = ustrlen(tempbuf1);
    free(tempbuf1);
    free(tempbuf2);
    return at;
}

//=============================================================================

size_t break_up_text_into_lines(const char *todis, bool apply_direction, SplitLines &lines, int wii, int fonnt, size_t max_lines) {
    if (fonnt == -1)
        fonnt = play.normal_font;

    //  char sofar[100];
    if (todis[0]=='&') {
        while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
        if (todis[0]==' ') todis++;
    }
    lines.Reset();
    longestline=0;

    // Don't attempt to display anything if the width is tiny
    if (wii < 3)
        return 0;

    int line_length;

    split_lines(todis, lines, wii, fonnt, max_lines);

    // Right-to-left just means reverse the text then
    // write it as normal
    if (apply_direction && (game.options[OPT_RIGHTLEFTWRITE] != 0))
        for (size_t rr = 0; rr < lines.Count(); rr++) {
            (get_uformat() == U_UTF8) ?
                lines[rr].ReverseUTF8() :
                lines[rr].Reverse();
            line_length = get_text_width_outlined(lines[rr].GetCStr(), fonnt);
            if (line_length > longestline)
                longestline = line_length;
        }
    else
        for (size_t rr = 0; rr < lines.Count(); rr++) {
            line_length = get_text_width_outlined(lines[rr].GetCStr(), fonnt);
            if (line_length > longestline)
                longestline = line_length;
        }
    return lines.Count();
}

// FIXME!!
// This is a ugly "safety fix" that tests whether the script tries
// to write inside the Character's struct (e.g. char.name?), and truncates
// the write limit... except it does not use full length anymore (40 now).
size_t MAXSTRLEN = MAX_MAXSTRLEN;
void check_strlen(char*ptt) {
    MAXSTRLEN = MAX_MAXSTRLEN;
    uintptr_t charstart = (uintptr_t)&game.chars[0];
    uintptr_t charend = charstart + sizeof(CharacterInfo)*game.numcharacters;
    if (((uintptr_t)&ptt[0] >= charstart) && ((uintptr_t)&ptt[0] <= charend))
        MAXSTRLEN=30;
}

/*void GetLanguageString(int indxx,char*buffr) {
VALIDATE_STRING(buffr);
char*bptr=get_language_text(indxx);
if (bptr==NULL) strcpy(buffr,"[language string error]");
else strncpy(buffr,bptr,199);
buffr[199]=0;
}*/

void my_strncpy(char *dest, const char *src, int len) {
    // the normal strncpy pads out the string with zeros up to the
    // max length -- we don't want that
    if (strlen(src) >= (unsigned)len) {
        strncpy(dest, src, len);
        dest[len] = 0;
    }
    else
        strcpy(dest, src);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/math.h"

// int (const char *thisString)
RuntimeScriptValue Sc_String_IsNullOrEmpty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(String_IsNullOrEmpty, const char);
}

// const char* (const char *thisString, const char *extrabit)
RuntimeScriptValue Sc_String_Append(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(const char, const char, myScriptStringImpl, String_Append, const char);
}

// const char* (const char *thisString, char extraOne)
RuntimeScriptValue Sc_String_AppendChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_AppendChar);
}

// int (const char *thisString, const char *otherString, bool caseSensitive)
RuntimeScriptValue Sc_String_CompareTo(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_CompareTo, const char);
}

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_StrContains(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(const char, StrContains, const char);
}

// const char* (const char *srcString)
RuntimeScriptValue Sc_String_Copy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Copy);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_EndsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_EndsWith, const char);
}

// const char* (const char *texx, ...)
RuntimeScriptValue Sc_String_Format(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(String_Format, 1);
    return RuntimeScriptValue().SetScriptObject((void*)CreateNewScriptString(scsf_buffer), &myScriptStringImpl);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_LowerCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_LowerCase);
}

// const char* (const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
RuntimeScriptValue Sc_String_Replace(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ2_PBOOL(const char, const char, myScriptStringImpl, String_Replace, const char, const char);
}

// const char* (const char *thisString, int index, char newChar)
RuntimeScriptValue Sc_String_ReplaceCharAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_ReplaceCharAt);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_StartsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_StartsWith, const char);
}

// const char* (const char *thisString, int index, int length)
RuntimeScriptValue Sc_String_Substring(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_Substring);
}

// const char* (const char *thisString, int length)
RuntimeScriptValue Sc_String_Truncate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_Truncate);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_UpperCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_UpperCase);
}

// FLOAT_RETURN_TYPE (const char *theString);
RuntimeScriptValue Sc_StringToFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(const char, StringToFloat);
}

// int (char*stino)
RuntimeScriptValue Sc_StringToInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(const char, StringToInt);
}

// int (const char *texx, int index)
RuntimeScriptValue Sc_String_GetChars(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(const char, String_GetChars);
}

RuntimeScriptValue Sc_strlen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF(strlen);
    return RuntimeScriptValue().SetInt32(ustrlen((const char*)self));
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// const char* (const char *texx, ...)
const char *ScPl_String_Format(const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    return CreateNewScriptString(scsf_buffer);
}


void RegisterStringAPI()
{
    ScFnRegister string_api[] = {
        { "String::IsNullOrEmpty^1",  API_FN_PAIR(String_IsNullOrEmpty) },
        { "String::Format^101",       Sc_String_Format, ScPl_String_Format },

        { "String::Append^1",         API_FN_PAIR(String_Append) },
        { "String::AppendChar^1",     API_FN_PAIR(String_AppendChar) },
        { "String::CompareTo^2",      API_FN_PAIR(String_CompareTo) },
        { "String::Contains^1",       API_FN_PAIR(StrContains) },
        { "String::Copy^0",           API_FN_PAIR(String_Copy) },
        { "String::EndsWith^2",       API_FN_PAIR(String_EndsWith) },
        { "String::IndexOf^1",        API_FN_PAIR(StrContains) },
        { "String::LowerCase^0",      API_FN_PAIR(String_LowerCase) },
        { "String::Replace^3",        API_FN_PAIR(String_Replace) },
        { "String::ReplaceCharAt^2",  API_FN_PAIR(String_ReplaceCharAt) },
        { "String::StartsWith^2",     API_FN_PAIR(String_StartsWith) },
        { "String::Substring^2",      API_FN_PAIR(String_Substring) },
        { "String::Truncate^1",       API_FN_PAIR(String_Truncate) },
        { "String::UpperCase^0",      API_FN_PAIR(String_UpperCase) },
        { "String::get_AsFloat",      API_FN_PAIR(StringToFloat) },
        { "String::get_AsInt",        API_FN_PAIR(StringToInt) },
        { "String::geti_Chars",       API_FN_PAIR(String_GetChars) },
        { "String::get_Length",       API_FN_PAIR(strlen) },
    };

    ccAddExternalFunctions(string_api);
}
