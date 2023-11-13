#include <stdio.h>
#include <sys/time.h>

int main() {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  printf("<h1>seconds : %ld</h1>\r\n<h2>micro seconds : %ld</h2>\r\n", current_time.tv_sec, current_time.tv_usec);

  return 0;
}
