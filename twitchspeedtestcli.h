#ifndef TWITCHTEST_CLI_TWITCHSPEEDTESTCLI_H
#define TWITCHTEST_CLI_TWITCHSPEEDTESTCLI_H

#define NOMINMAX
#include <string>
#include <cstdint>
#include <list>
#include <memory>
#include <librtmp/amf.h>

class RTMP;

struct ingest
{
    std::string name;
    std::string url;
};

struct result
{
    std::shared_ptr<ingest> i;
    bool ok;
    float speed;
};

struct Args
{
    std::string key;
    std::string stringSearch;
    std::string regexSearch;
    std::string dumpPath;
    int minTestTime = 5;
    int testTime = 10;
    bool printIngests = false;
};

class twitchspeedtestcli
{
private:
    static const int32_t startMeasureWait = 2500;

    RTMP *prepareRtmp(std::string const &url, std::string const &key);

    bool sendRTMPMetadata(RTMP *rtmp);

    bool testRtmp(RTMP *rtmp, Args const& args, float& speed);

    void loadIngests();

    std::list<std::shared_ptr<ingest>> ingests;
    std::list<result> results;

public:
    twitchspeedtestcli();

    void startTest(Args const& args);

    void dumpResults(std::string const& path);

    result* getBestResult();
    std::list<std::shared_ptr<ingest>> const &getIngests() const;
};


#endif //TWITCHTEST_