
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tNUMBER = 258,
     UNARY_PLUS = 259,
     tSUBTRACT = 260,
     tADD = 261,
     tXNOR = 262,
     tXOR = 263,
     tOR = 264,
     tAND = 265,
     MULTIPLICATION = 266,
     tMOD = 267,
     tDIVIDE = 268,
     tMULTIPLY = 269,
     tNOT = 270,
     tROOT4 = 271,
     tROOT3 = 272,
     tROOT = 273,
     tFUNCTION = 274,
     tPI = 275,
     tVARIABLE = 276,
     tSUPNUM = 277,
     tSUBNUM = 278,
     BOOLEAN_OPERATOR = 279,
     PERCENTAGE = 280,
     UNARY_MINUS = 281,
     tINVERSE = 282
   };
#endif
/* Tokens.  */
#define tNUMBER 258
#define UNARY_PLUS 259
#define tSUBTRACT 260
#define tADD 261
#define tXNOR 262
#define tXOR 263
#define tOR 264
#define tAND 265
#define MULTIPLICATION 266
#define tMOD 267
#define tDIVIDE 268
#define tMULTIPLY 269
#define tNOT 270
#define tROOT4 271
#define tROOT3 272
#define tROOT 273
#define tFUNCTION 274
#define tPI 275
#define tVARIABLE 276
#define tSUPNUM 277
#define tSUBNUM 278
#define BOOLEAN_OPERATOR 279
#define PERCENTAGE 280
#define UNARY_MINUS 281
#define tINVERSE 282




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 101 "./mp-equation-parser.y"

  MPNumber int_t;
  int integer;
  char *name;



/* Line 1676 of yacc.c  */
#line 114 "mp-equation-parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



