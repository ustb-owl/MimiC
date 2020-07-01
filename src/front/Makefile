myparser:parser.y ../define/ast.cpp
	bison -d parser.y
	flex lexer.l
	g++ -std=c++17 parser.tab.cc lex.yy.c ../define/ast.cpp -o myparser -I ../ -I ../../3rdparty