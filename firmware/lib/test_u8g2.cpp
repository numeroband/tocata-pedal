#include <display.h>

int main(int argc, const char* argv[])
{
    static tocata::Display display{8, 9};
    display.init();
    display.run();
    
    return 0;
}