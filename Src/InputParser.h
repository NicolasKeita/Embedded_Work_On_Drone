import std;

class InputParser
{
public:
    explicit InputParser(int argc, char* argv[]);

    void ParseArguments();

    bool HasOption(std::string_view option) const;

    std::string GetOptionValue(std::string_view option) const;

    std::vector<std::string> GetPositionalArguments() const;

private:
    std::vector<std::string> m_arguments;
    std::vector<std::string> m_positionalArguments;
};