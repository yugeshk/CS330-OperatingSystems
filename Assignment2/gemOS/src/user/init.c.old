
#include<ulib.h>
// Dont try to add any other header files.
int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
   int a = 10, b = 20;
   printf("%d\n", a+b);
   
   int pid = getpid();
   printf("Pid %d\n", pid);
   
   pid = fork();
   if(pid == 0)
   {
      printf(" OS C330 Child1\n");
      exit(0);
   }
   return 0;
}