#include "../inc/definitions.h"
#include "../inc/memory.h"

int main(int argc, char *argv[]) {
    memoryInit();
	#ifdef DEBUG
	printf("Debug Mode\n");
	#endif

	int a = 5;
	printf("Value of a: %d\n", a);

	return 0;
}
