#ifndef ERROR_H
#define ERROR_H

#define  MISSING_STRING_TERMINATOR    0
#define  MISSING_CHAR_TERMINATOR      1
#define  INVALID_CHARACTER            2


#define  UNKNOWN_TYPENAME             3
#define  EXPECTED_IDENTIFIER          4
#define  EXPECTED_SEMICOLON           5
#define  EXPECTED_CLOSE_BRACKET       6
#define  PREVIOUSLY_DECLARED          7
#define  EXPECTED_RIGHT_PAREN         8
#define  INVALID_EXPRESSION           9
#define  UNCLOSED_ARRAY              10
#define  MISSING_ARRAY_LBRACE        11
#define  MISSING_ARRAY_RBRACE        12
#define  TOO_MANY_INIT_ELEMENTS      13
#define  EXPECTED_STRING_LITERAL     14  
#define  EXPECTED_FUNC_LPAREN        15      
#define  EXPECTED_FUNC_RPAREN        16
#define  EXPECTED_FUNC_LBRACE        17
#define  EXPECTED_FUNC_RBRACE        18
#define  LOCAL_PREVIOUSLY_DECLARED   19
#define  UNKNOWN_VARIABLE            20
#define  NOT_AN_ARRAY                21  
#define  EXPECTED_STRUCT_NAME        22
#define  STRUCT_MEMBER_EXISTS        23
#define  EXPECTED_STRUCT_RBRACE      24
#define  EXPECTED_STRUCT_SEMICOLON   25
#define  EXPECTED_STRUCT_LBRACE      26
#define  STRUCT_IS_POINTER           27
#define  NOT_A_MEMBER                28
#define  STRUCT_NOT_POINTER          29
#define  MEMBER_NOT_STRUCT           30
#define  EXPECTED_EQUAL_SIGN         31
#define  WRONG_ARRAY_SIZE            32
#define  ASSIGNING_TO_STRUCT         33
#define  UNCLOSED_IF_STMT            34
#define  UNCLOSED_ELSE_STMT          35
#define  UNCLOSED_WHILE_STMT         36
#define  UNCLOSED_FOR_STMT           37
#define  EXPECTED_FOR_RPAREN         38
#define  EXPECTED_FOR_SEMICOLON      39
#define  EXPECTED_DO_STMT_LBRACE     40  
#define	 UNCLOSED_DO_STMT	         41
#define  EXPECTED_A_WHILE_STMT       42
#define  FLOATING_MODULO_ARITH       43
#define  FLOATING_BITWISE_ARITH      44
#define  EXPECTED_CALL_LPAREN        45
#define  EXPECTED_CALL_RPAREN        46
#define  UNKNOWN_INPUT_TYPE          47
#define  UNKNOWN_FUNC_CALL           48
#define  EXPECTED_A_COMMA            49
#define  NOT_IN_LOOP                 50
#define  NOT_A_POINTER               51
#define  EXCESS_POINTER_DEREFS       52
#define  EXPECTED_LPAREN             53
#define  EXPECTED_RPAREN             54
#define  ASSIGNING_TO_ARRAY          55
#define  UNKNOWN_RETTYPE             56
#define  TOKEN_TOO_LONG              57

void error(int error_code);

#endif