%option reentrant
%option bison-bridge
%option noyywrap
%{
#include <stdio.h>
#include "parser.h"
#include "parser.yac.hh"

extern int yyparse(parser_t *parser);
%}
%%
[ \t]                  ;
\n                     { printf("\n"); }
\-?[0-9]*\.[0-9]+      { yylval->fval = atof(yytext); return FLOAT; }
\-?[0-9]+              { yylval->ival = atoi(yytext); return INT; }
\:                     { return COLON; }

(?i:abs)               { yylval->pval = pos_absolute; return POSTYPE; }
(?i:reltgt)            { yylval->pval = pos_relative_target; return POSTYPE; }
(?i:relact)            { yylval->pval = pos_relative_actual; return POSTYPE; }

(?i:frequencydivisor)  { return FREQUENCYDIVISOR; }
(?i:setbyteorder)      { return SETBYTEORDER; }
(?i:bigendian)         { return BIGENDIAN; }
(?i:littleendian)      { return LITTLEENDIAN; }
(?i:sendcurrentframe)  { return SENDCURRENTFRAME; }
(?i:streamframes)      { return STREAMFRAMES; }
(?i:stop)              { return STOP; }
(?i:profile)           { return PROFILE; }
(?i:set)               { return SET; }
(?i:execute)           { return EXECUTE; }
(?i:table)             { return TABLE; }
(?i:next)              { return NEXT; }
(?i:after)             { return AFTER; }
(?i:blend)             { return BLEND; }
(?i:before)            { return BEFORE; }
(?i:sinusoid)          { return SINUSOID; }
(?i:start)             { return START; }
(?i:lights)            { return LIGHTS; }
(?i:on)                { return ON; }
(?i:off)               { return OFF; }
(?i:bye)               { return BYE; }
(?i:home)              { return HOME; }
(?i:clearfault)        { return CLEARFAULT; }
(?i:setinternalstatus) { return SETINTERNALSTATUS; }
(?i:preoperational)    { return PREOPERATIONAL; }
(?i:operational)       { return OPERATIONAL; }
(?i:outputenabled)     { return OUTPUTENABLED; }
(?i:sendinternalstatus) { return SENDINTERNALSTATUS; }

[A-Za-z][A-Za-z0-9]*   { yylval->sval = strdup(yytext); return STRING;}

%%

void *parser_create()
{
  parser_t *parser = (parser_t *) malloc(sizeof(parser_t));

  if(!parser)
    return NULL;

  yylex_init(&(parser->scanner));
  return (void *) parser;
}

void parser_destroy(void **parser)
{
  if(!parser) return;
  if(!*parser) return;

  yylex_destroy(((parser_t *) *parser)->scanner);
  free(*parser);
  parser = NULL;
}

int parser_parse_string(void *p, const char *s, command_t *c)
{
  // Parser or command not set
  if(!p || !c)
    return -1;

  // Clear command
  //c->type = cmd_None;

  parser_t *parser = (parser_t *) p;
  parser->command = c;

  YY_BUFFER_STATE buffer = yy_scan_string(s, parser->scanner);
  int retval = yyparse(parser);
  yy_delete_buffer(buffer, parser->scanner);

  parser->command = NULL;

  if(retval == 0)
    return 0;

  return -1;
}

void yyerror(void *yyscanner, const char *s)
{
}

