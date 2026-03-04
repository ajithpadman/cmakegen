grammar ConditionExpr;

// Parser rules (Jira-style: var op value, AND, OR, NOT, default, parentheses)
expr        : orExpr ;
orExpr      : andExpr ( OR andExpr )* ;
andExpr     : notExpr ( AND notExpr )* ;
notExpr     : NOT notExpr | primary ;
primary     : DEFAULT | LPAREN expr RPAREN | simple ;
simple      : IDENT op value ;
op          : EQUALS | IN | notIn ;
notIn       : NOT IN ;
value       : IDENT | LPAREN idList RPAREN ;
idList      : IDENT ( COMMA IDENT )* ;

// Lexer rules (case-insensitive keywords via fragments)
OR          : [oO][rR] ;
AND         : [aA][nN][dD] ;
NOT         : [nN][oO][tT] ;
IN          : [iI][nN] ;
EQUALS      : '=' | [eE][qQ][uU][aA][lL][sS] ;
DEFAULT     : [dD][eE][fF][aA][uU][lL][tT] ;
IDENT       : [a-zA-Z_][a-zA-Z0-9_-]* ;
LPAREN      : '(' ;
RPAREN      : ')' ;
COMMA       : ',' ;
WS          : [ \t\r\n]+ -> skip ;
