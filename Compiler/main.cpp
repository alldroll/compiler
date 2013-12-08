//#include "codeGen.h"
#include <string.h>
#include <stdlib.h>
#include "codeGen.h"

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

typedef enum
{
	SCAN,
	PARSE,
	GEN,
}KeyT;

KeyT getKey(char* k)
{
	if (strlen(k) < 2)
	{
		cout << "Please enter command type" << endl;
		exit(EXIT_FAILURE);
	}
	switch(k[1])
	{
		case 's' : return PARSE;
		case 'l' : return SCAN;
		case 'g' : return GEN;
	}
	cout << "There is not such command" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc ,char* argv[])
{	
	FILE *file;
	char* filename;
	string outStream;
	if (argc < 2)
	{
		cout << "C compiler by Alexander D. Petrov  C8303a"<< endl;
		cout << "Usage : " << "compiler" <<" <filename>" << endl;
	}
	if (argc == 3)
	{
		filename = argv[2];
		file = fopen(filename, "r");
		if (file == NULL)
		{
			fprintf(stderr, "Cannot open file");
			return EXIT_FAILURE;
		}  
		try
		{
			Scanner scanner(file);
			Parser parser(scanner);

			switch(getKey(argv[1]))
			{
				case SCAN : 
					while (scanner.currToken != EOF_TOKEN)
						cout << scanner.next() << endl;
					break;
				case PARSE :
				{
					parser.parse();
					parser.printTree();
				}break;

				default :
				{
					int i = strlen(filename);
					outStream = filename;
					while (i >= 0 && outStream[i--] != '.' && outStream[i] != '\\');
	
					CodeGen generator(parser, outStream = outStream.substr(0, (i == 0 ? strlen(filename) : i + 1)) + ".asm");
					generator.generate();
				}
			}
			fclose(file);
		}
		catch (CompilerException &error)
		{
			freopen(outStream.c_str(), "w", stdout);
			cout << error.text() <<endl;
		}
	}
	return EXIT_SUCCESS;
}