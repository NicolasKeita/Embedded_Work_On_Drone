import std;
import InputParser;

int main(int argc, char* argv[])
{
    InputParser parser(argc, argv);
    parser.ParseArguments();

    std::cout << "Options détectées :" << std::endl;
    if (parser.HasOption("--help"))
    {
        std::cout << "  --help : affiche l'aide" << std::endl;
    }
    if (parser.HasOption("--version"))
    {
        std::cout << "  --version : affiche la version" << std::endl;
    }

    const std::string output = parser.GetOptionValue("--output");
    if (!output.empty())
    {
        std::cout << "  --output : " << output << std::endl;
    }

    const auto positional = parser.GetPositionalArguments();
    if (!positional.empty())
    {
        std::cout << "Arguments positionnels :" << std::endl;
        for (const auto& argument : positional)
        {
            std::cout << "  " << argument << std::endl;
        }
    }

    return 0;
}