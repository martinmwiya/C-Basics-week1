#include<stdio.h>
int main()
{
int num1;
int num2;
printf("Enter Num1:\n");
scanf("%d", &num1);
printf("Enter Num2:\n");
scanf("%d", &num2);
int sum = num1 + num2;
printf("Sum is:%d\n", sum);
int dif = num1 - num2;
printf("Dif is:%d\n", dif);
int pro = num1 * num2;
printf("Pro is:%d\n", pro);
if (num2  != 0) {
int qoutient = num1 / num2;
printf("Qoutient is:%d\n", qoutient);
} else {
printf("Can not divide by Zero.\n");
}
return 0;
}
