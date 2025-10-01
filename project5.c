#include<stdio.h>
int main()
{
	float x;
        float y;
	char ch;
	printf("Enter first number\n");
	scanf("%f",&x);
	printf("Enter second number\n");
	scanf("%f",&y);	
	printf("Enter\n+ for add\n- for sub\n* for multiply\n/ for div\n");
	scanf("\n%c",&ch);
	switch(ch)
	{		
		case '+':
			printf("Add=%f",(x+y));
			break;
		case '-':
			printf("Sub=%f",(x-y));
			break;
		case '*':
			printf("Multiply=%f",(x*y));
			break;
		case '/':
			printf("Div=%f",(x/y));
			break;
		default:
			printf("Invalid input!!");
	}
}
