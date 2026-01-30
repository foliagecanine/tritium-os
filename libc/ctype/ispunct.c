#include <ctype.h>

int ispunct(int c) {
	unsigned char uc = (unsigned char)c;
	return uc == '!' || uc == '"' || uc == '#' || uc == '$' || uc == '%' || uc == '&'
		|| uc == '\'' || uc == '(' || uc == ')' || uc == '*' || uc == '+' || uc == ','
		|| uc == '-' || uc == '.' || uc == '/' || uc == ':' || uc == ';' || uc == '<'
		|| uc == '=' || uc == '>' || uc == '?' || uc == '@' || uc == '[' || uc == '\\'
		|| uc == ']' || uc == '^' || uc == '_' || uc == '`' || uc == '{' || uc == '|'
		|| uc == '}' || uc == '~';
}