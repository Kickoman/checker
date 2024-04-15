#include <iostream>

int main() {
    constexpr char diff = 'a' - 'A';
    std::istreambuf_iterator<char> eos;
    std::string s(std::istreambuf_iterator<char>(std::cin), eos);
    for (const auto c : s) {
        if (!std::isalpha(c))
            continue;
        std::cout << static_cast<char>(c - diff);
    }
    std::cout << std::endl;
    return 0;
}
