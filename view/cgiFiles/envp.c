#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    int i = -1;
    (void)argc;
    (void)argv;
    printf("<h1 style=\'color: crimson; font-family: arial;\'>OUR ENV VARIABLES</h1>");
    while (envp[++i])
    {
        printf("<h3style=\'color: black; font-family: arial;\'>%s</h3><br>", envp[i]);
    }
}