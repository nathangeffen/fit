#include <boost/process.hpp>

#include <string>
#include <iostream>

using namespace boost::process;

int main()
{
    ipstream pipe_stream;
    child c("gcc --version", std_out > pipe_stream);

    std::string line;

    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        std::cerr << line << std::endl;

    c.wait();
    ipstream is;
    std::cout << "A\n";
    system("ls -al", std_out > is);
    std::string s;
    double d;
    //is >> s;
    is >> d;
    std::cout << "s is: " << s << " d " << d << "\n";
    std::cout << "B\n";
}
