#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char *test_name = "";

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"test", required_argument, 0, 't'}
        };
        int c = getopt_long(argc, argv, "t:h",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 't': 
            test_name = optarg;
            break;
        }
    }

    printf("running test: %s\n", test_name);
    return EXIT_SUCCESS;
}
