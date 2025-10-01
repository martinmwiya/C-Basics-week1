#include <stdio.h>
int main ()
{
float celcius;
float fahrenheit;
printf("Enter temperature in celsius:\n");
scanf("%f", &celcius);
fahrenheit = (celcius * 9/5)*32;
printf("Fahrenheit: %.1f\n", fahrenheit);
printf("Enter temperature in fahrenheit:");
scanf("%f", &fahrenheit);
celcius = (fahrenheit -32)*5/9;
printf("Celcius: %.1f\n", celcius);
return 0;
}
