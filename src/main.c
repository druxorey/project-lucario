#include "../inc/definitions.h"

int main(int argc, char *argv[]) {
	#ifdef DEBUG
	printf("Debug Mode\n");
	#endif

	int a = 5;
	printf("Value of a: %d\n", a);

	return 0;
}
