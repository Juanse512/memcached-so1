all: spell_check checker_test hashing_test helpers_test io_test rules_test
spell_check: main.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall main.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o spell_check -lm 
checker_test: ./tests/checker_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall ./tests/checker_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o checker_test -lm 
hashing_test: ./tests/hashing_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall ./tests/hashing_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o hashing_test -lm 
helpers_test: ./tests/helpers_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall ./tests/helpers_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o helpers_test -lm 
io_test: ./tests/io_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall ./tests/io_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o io_test -lm 
rules_test: ./tests/rules_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c
	- gcc -g -Wall ./tests/rules_test.c ./src/checker.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/rules.c -o rules_test -lm 
clean: spell_check
	- rm spell_check rules_test helpers_test io_test hashing_test checker_test