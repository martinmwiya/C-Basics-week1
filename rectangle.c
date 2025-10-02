#include <stdio.h>
int main()  {

      float length;
      float width;


        printf("Enter Length:\n");
        scanf("%f", &length);
        printf("Enter Width:\n");
        scanf("%f", &width);

   float area = length * width;

 printf("Area of a Rectangle: %.2f\n", area);

return 0;

}
