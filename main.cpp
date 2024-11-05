#include <iostream>
#include <regex>
#include <list>
#include <filesystem>
#include <functional>
#include "twitchspeedtestcli.h"

class BaseParam
{
public:
    std::string name;

    virtual void init(std::string const& value) = 0;

    virtual ~BaseParam() = default;
};

class StringParam : public BaseParam
{
public:
    StringParam(std::string const& name, std::string& dest)
    {
        this->name = name;
        this->dest = &dest;
    }

    void init(std::string const& value) override
    {
        *dest = value;
    }

    std::string* dest;
};

class IntParam: public BaseParam
{
public:
    IntParam(std::string const& name, int& dest)
    {
        this->name = name;
        this->dest = &dest;
    }

    void init(std::string const& value) override
    {
        *dest = std::stoi(value);
    }

    int* dest;
};

class OptionParam: public BaseParam
{
public:
    OptionParam(std::string const& name, bool& dest)
    {
        this->name = name;
        this->dest = &dest;
    }

    void init(std::string const& value) override
    {
        *dest = true;
    }

    bool* dest;
};

class CallbackParam: public BaseParam
{
public:
    CallbackParam(std::string const& name, std::function<void()> callback)
    {
        this->name = name;
        this->callback = callback;
    }

    void init(std::string const& value) override
    {
        callback();
    }

    std::function<void()> callback;
};

Args args;
std::list<std::unique_ptr<BaseParam>> argInitializers;

void initArgs()
{
    argInitializers.push_back(std::make_unique<CallbackParam>("help", []{
        std::cout << "Usage: twitchspeedtest-cli [OPTIONS]..." << std::endl;
        std::cout << "Tests upload speed to all known Twitch ingest endpoints and suggests fastest" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --key=KEY               Twitch stream key" << std::endl;
        std::cout << "  --out=PATH              Path for results to be written in json format" << std::endl;
        std::cout << "  --match=TEXT            Ingest endpoint name must contain TEXT" << std::endl;
        std::cout << "  --regex=EXPR            Ingest endpoint name must match EXPR" << std::endl;
        std::cout << "  --test-time=VALUE       Test time (default 10)" << std::endl;
        std::cout << "  --min-test-time=VALUE   Minimum test time (default 5)" << std::endl;
        std::cout << "  --ingests               Print available ingest endpoints" << std::endl;

        exit(0);
    }));
    argInitializers.push_back(std::make_unique<StringParam>("key", args.key));
    argInitializers.push_back(std::make_unique<StringParam>("out", args.dumpPath));
    argInitializers.push_back(std::make_unique<StringParam>("match", args.stringSearch));
    argInitializers.push_back(std::make_unique<StringParam>("regex", args.regexSearch));
    argInitializers.push_back(std::make_unique<IntParam>("test-time", args.testTime));
    argInitializers.push_back(std::make_unique<IntParam>("min-test-time", args.minTestTime));
    argInitializers.push_back(std::make_unique<OptionParam>("ingests", args.printIngests));
}

void parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        for (auto& item: argInitializers)
        {
            std::smatch match;
            if (std::regex_match(arg, match, std::regex("--" + item->name + "=?(.+)?")))
                item->init(match[1].str());
        }
    }
}

int main(int argc, char* argv[])
{
    initArgs();
    parseArgs(argc, argv);

    if (args.key.length() == 0 && !args.printIngests)
    {
        std::cerr << "Twitch key not passed. Expected --key" << std::endl;
        return 1;
    }

    if (args.dumpPath.length())
    {
        std::filesystem::path dumpPath = std::filesystem::absolute(args.dumpPath);
        if (!std::filesystem::is_directory(dumpPath.parent_path()))
        {
            std::cerr << dumpPath.parent_path() << " is not a directory" << std::endl;
            return 1;
        }
    }

    auto test = new twitchspeedtestcli();
    if (args.printIngests) {
        auto ingests = test->getIngests();
        for (auto& ingest : ingests)
        {
            std::cout << ingest->name << " | " << ingest->url << std::endl;
        }
        exit(0);
    }

    test->startTest(args);
    if (args.dumpPath.length()) {
        test->dumpResults(args.dumpPath);
    }

    return 0;
}
