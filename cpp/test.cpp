#include <string>
#include <vector>
#include <iostream>

int main()
{
    std::vector<double> a;
    std::vector<double> b;

    b = {1.0, 2.0, 3.0};
    a = b;
    b[1]++;
    a[0]++;
    std::cout << a[0] << " " << a[1] << " " << a[2] << "\n";
    std::cout << b[0] << " " << b[1] << " " << b[2] << "\n";
}
