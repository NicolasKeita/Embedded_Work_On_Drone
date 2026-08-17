import std;

import InputParser;

int main(int argc, char* argv[])
{
    InputParser parser(argc, argv);
    parser.ParseArguments();

    if (parser.HasOption("--help"))
    {
        std::cout << "Usage: HelloWorld [options] [arguments]" << std::endl;
        return 0;
    }

    std::cout << "Hello, World!" << std::endl;

    return 0;
}