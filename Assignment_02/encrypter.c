#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100

void encrypt(char *curr, int k)
{
    int tmp = *curr;
    int flag = 0;

    if (tmp >= 'a')
    {
        tmp -= 'a';
        flag = 1;
    }
    else
        tmp -= 'A';

    tmp = (tmp + k) % 26;

    if (flag)
        tmp += 'a';
    else
        tmp += 'A';

    *curr = tmp;
}

int main(){
    char string[] = "Hello, I am Harshith";
    encrypt(string+1, 1);
    
    printf("%s\n", string);

    return (0);
}