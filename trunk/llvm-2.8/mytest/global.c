#include <stdio.h>

int g_a, g_b, g_array1[15], g_array2[5];
int main()
{
	int i = 0;
	i = g_array1[0] + g_array2[0] + g_a + g_b;	
	printf("i=%d\n", i);
	return 0;
}
