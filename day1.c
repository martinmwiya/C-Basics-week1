#include<stdio.h>
int main()
{
printf("My name is Martin Mwiya,i am 17 years old and my favourite hobby is playing football\n");
char name[50];
int age;
printf("What is your name?\n");
scanf("%s", name);
printf("How old are you?");
scanf("%d", &age);
printf("Hello %s, how are you?\nYou are %d years old\n", name, age);
return 0;
}
