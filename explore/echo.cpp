#include <cstdio>
#include <iostream>

int main(int argc, char **argv) {
    std::string s;
    if (argc == 1) {
        std::istreambuf_iterator<char> eos;
        s = std::string(std::istreambuf_iterator<char>(std::cin), eos);
    } else {
        for (int i = 1; i < argc; ++i)
            s += std::string(" ") + argv[i];
    }
    std::cout << "Echoed: " << s << std::endl;
    return 0;
}
