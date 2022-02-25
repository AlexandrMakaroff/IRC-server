#include "Server.hpp"

int main(int ac, char **av)
{
    try
    {
        Server server(ac, av);
        server.start();
        server.exec();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return (0);
}