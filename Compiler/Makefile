LIBS=
CC=g++
CFLAGS=-c -g
SOURCES=main.cpp buffer.cpp exceptions.cpp token.cpp scanner.cpp syntaxNode.cpp symTable.cpp node.cpp parser.cpp codeGen.cpp           
ODIR=Debug
OBJECTS=$(SOURCES:%.cpp=$(ODIR)/%.o)
EXECUTABLE=compiler

run: clean all

all: $(SOURCES) $(EXECUTABLE)

$(ODIR)/%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $(ODIR)/$(EXECUTABLE) $(LIBS)

clean:
	-rm -rf $(ODIR)/*.o $(EXECUTABLE)

