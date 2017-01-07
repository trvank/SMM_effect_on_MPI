// verify.c (for CS415/515 Spring '14)
//
// Verify that the integers in a data file are sorted in an ascending order.
//
// Usage: verify <datafile>
//
//
#include <stdio.h>

int main(int argc, char **argv) 
{
  FILE *fp;
  int count, val1, val2, i;
  int intSize = sizeof(int);

  if (argc != 2) {
    printf("Usage: verify <datafile>\n");
    return 0;
  }
  if (!(fp = fopen(argv[1], "r"))) {
    printf("Can't open datafile %s\n", argv[1]);
    return 0;
  }

  fread(&count, intSize, 1, fp);
  fread(&val1, intSize, 1, fp);
  for (i=1; i<count; i++) {
    fread(&val2, intSize, 1, fp);
    if (val1 > val2) {
      printf("Failed: items %u, %u: %u > %u\n",
	     i-1, i, val1, val2);
      return 0;
    }
    val1 = val2;
  }
  printf("Data in %s is sorted\n", argv[1]);
}

