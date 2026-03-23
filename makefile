scanner: main.o scanner.o
	g++ $^ -o $@

%.o: %.cpp
	g++ -c $< -o $@

%.cpp: %.l
	flex -i -o $@ $<

clean:
	rm -f *.o scanner scanner.cpp