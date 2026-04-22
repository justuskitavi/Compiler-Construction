scanner: main.o scanner.o
	g++ $^ -o $@

%.o: %.cpp
	g++ -c $< -o $@

%.cpp: %.l
	flex -i -o $@ $<

clean:
	rm -f *.o scanner scanner.cpp recursive ll1

recursive:
	g++ recursive_parser.cpp -o recursive

ll1:
	g++ ll1_parser.cpp parse_table.cpp terminals.cpp production_rules.cpp -o ll1

parse: scanner ll1
	./scanner input.txt ; ./ll1 tokens.out

all: clean scanner recursive ll1