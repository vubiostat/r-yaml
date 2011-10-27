#include "yaml.h"

yaml_char_t *
find_implicit_tag(str, len)
  const yaml_char_t *str;
  size_t len;
{
  /* This bit was taken from implicit.re, which is in the Syck library.
   *
   * Copyright (C) 2003 why the lucky stiff */

  const yaml_char_t *cursor, *limit, *marker;
  cursor = str;
  limit = str + len;

/*!re2c

re2c:define:YYCTYPE  = "yaml_char_t";
re2c:define:YYCURSOR = cursor;
re2c:define:YYMARKER = marker;
re2c:define:YYLIMIT  = limit;
re2c:yyfill:enable   = 0;

NULL = [\000] ;
ANY = [\001-\377] ;
DIGIT = [0-9] ;
DIGITSC = [0-9,] ;
DIGITSP = [0-9.] ;
YEAR = DIGIT DIGIT DIGIT DIGIT ;
MON = DIGIT DIGIT ;
SIGN = [-+] ;
HEX = [0-9a-fA-F,] ;
OCT = [0-7,] ;
INTHEX = SIGN? "0x" HEX+ ; 
INTOCT = SIGN? "0" OCT+ ;
INTSIXTY = SIGN? DIGIT DIGITSC* ( ":" [0-5]? DIGIT )+ ;
INTCANON = SIGN? ( "0" | [1-9] DIGITSC* ) ;
FLOATFIX = SIGN? DIGIT DIGITSC* "." DIGITSC* ;
FLOATEXP = SIGN? DIGIT DIGITSC* "." DIGITSP* [eE] SIGN DIGIT+ ;
FLOATSIXTY = SIGN? DIGIT DIGITSC* ( ":" [0-5]? DIGIT )+ "." DIGITSC* ;
INF = ( "inf" | "Inf" | "INF" ) ;
FLOATINF = [+]? "." INF ;
FLOATNEGINF = [-] "." INF ;
FLOATNAN = "." ( "nan" | "NaN" | "NAN" ) ;
NULLTYPE = ( "~" | "null" | "Null" | "NULL" )? ;
BOOLYES = ( "yes" | "Yes" | "YES" | "true" | "True" | "TRUE" | "on" | "On" | "ON" ) ;
BOOLNO = ( "no" | "No" | "NO" | "false" | "False" | "FALSE" | "off" | "Off" | "OFF" ) ;
TIMEZ = ( "Z" | [-+] DIGIT DIGIT ( ":" DIGIT DIGIT )? ) ;
TIMEYMD = YEAR "-" MON "-" MON ;
TIMEISO = YEAR "-" MON "-" MON [Tt] MON ":" MON ":" MON ( "." DIGIT* )? TIMEZ ;
TIMESPACED = YEAR "-" MON "-" MON [ \t]+ MON ":" MON ":" MON ( "." DIGIT* )? [ \t]+ TIMEZ ;
TIMECANON = YEAR "-" MON "-" MON "T" MON ":" MON ":" MON ( "." DIGIT* [1-9]+ )? "Z" ;
MERGE = "<<" ;
DEFAULTKEY = "=" ;

NULLTYPE NULL       {   return "null"; }

BOOLYES NULL        {   return "bool#yes"; }

BOOLNO NULL         {   return "bool#no"; }

INTHEX NULL         {   return "int#hex"; }

INTOCT NULL         {   return "int#oct"; }

INTSIXTY NULL       {   return "int#base60"; }

INTCANON NULL       {   return "int"; }

FLOATFIX NULL       {   return "float#fix"; }

FLOATEXP NULL       {   return "float#exp"; }

FLOATSIXTY NULL     {   return "float#base60"; }

FLOATINF NULL       {   return "float#inf"; }

FLOATNEGINF NULL    {   return "float#neginf"; }

FLOATNAN NULL       {   return "float#nan"; }

TIMEYMD NULL        {   return "timestamp#ymd"; }

TIMEISO NULL        {   return "timestamp#iso8601"; }

TIMESPACED NULL     {   return "timestamp#spaced"; }

TIMECANON NULL      {   return "timestamp"; }

DEFAULTKEY NULL     {   return "default"; }

MERGE NULL          {   return "merge"; }

ANY                 {   return "str"; }

*/

}
