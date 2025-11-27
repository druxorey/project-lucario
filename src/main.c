#include "../inc/main.h"

int main(int argc, char *argv[]) {
	#ifdef DEBUG
	printf("Debug Mode\n");
	#endif

	i64 defaultValue = 42;
	printf("Hello World %ld\n", defaultValue);

	return 0;
}
