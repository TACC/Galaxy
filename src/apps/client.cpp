#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "ClientServer.h"

int
main(int argc, char **argv)
{
	ClientServer cs;

	if (argc == 1)
		cs.setup_client("localhost");
	else
		cs.setup_client(argv[1]);
		
	std::cerr << "setup OK\n";

	char c;
	do
	{
		c = getchar();
		std::cerr << "got " << c << "\n";
		if (c == 's')
			write(cs.get_socket(), &c, 1);
	}  while (c != 'q');
}

