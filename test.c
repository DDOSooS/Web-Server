#include <stdio.h>
#include <string.h>

int main() {
    char main_string[] = "This is a sample string.";
    char sub_string[] = "sample";
    char *ptr;

    ptr = strstr(main_string, sub_string);

    if (ptr != NULL) {
        // Calculate the index by subtracting the base address of the main string
        int index = ptr - main_string;
        printf("The first occurrence of \"%s\" in \"%s\" is at index %d.\n", sub_string, main_string, index);
        printf("Substring found: %s\n", ptr);
    } else {
        printf("Substring \"%s\" not found in \"%s\".\n", sub_string, main_string);
    }

    return 0;
}
