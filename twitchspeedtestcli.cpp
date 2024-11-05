#include "twitchspeedtestcli.h"
#include <nlohmann/json.hpp>
#include <librtmp/rtmp_sys.h>
#include <librtmp/rtmp.h>
#include <regex>
#include <iostream>

#ifdef WIN32
    #include <Wininet.h>

    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "wininet.lib")
    #pragma comment(lib, "winmm.lib")
#else
    #include <curl/curl.h>
#endif

#define SAVC(x) static const AVal av_##x = AVC((char*)#x)

//static const AVal av_OBSVersion = AVC("TwitchTest/1.4-qt");
static const AVal av_OBSVersion = AVC((char*)"TwitchTest/1.4-qt");
static const AVal av_setDataFrame = AVC((char*)"@setDataFrame");

SAVC(onMetaData);
SAVC(duration);
SAVC(width);
SAVC(height);
SAVC(videocodecid);
SAVC(videodatarate);
SAVC(framerate);
SAVC(audiocodecid);
SAVC(audiodatarate);
SAVC(audiosamplerate);
SAVC(audiosamplesize);
SAVC(audiochannels);
SAVC(stereo);
SAVC(encoder);
SAVC(fileSize);

SAVC(onStatus);
SAVC(status);
SAVC(details);
SAVC(clientid);

SAVC(avc1);
SAVC(mp4a);

#include <list>
#include <util/platform.h>
#include <fstream>

static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char *) contents, size * nmemb);
    return size * nmemb;
};

std::string downloadIngests()
{
    std::string response;

#ifdef WIN32
    auto hInternet = InternetOpenW(L"TwitchTest/1.52", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    auto hConnect = InternetOpenUrlW(hInternet, L"https://ingest.twitch.tv/api/v2/ingests", L"", -1L, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);

    DWORD ret;
    char buff[65536];
    int read = 0;
    for (;;)
    {
        if (InternetReadFile(hConnect, buff + read, sizeof(buff) - read, &ret))
        {
            if (ret == 0)
                break;

            read += ret;
        }
        else
        {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "";
        }
    }

    buff[read] = 0;

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    response = buff;
#else
    auto curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://ingest.twitch.tv/api/v2/ingests");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    auto res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
#endif

    if (response.length() == 0)
        return "";

    return response;
}

void twitchspeedtestcli::loadIngests()
{
    auto data = downloadIngests();

    auto json = nlohmann::json::parse(data);
    if (!json.contains("ingests"))
        return;

    for (auto &obj: json["ingests"])
        ingests.push_back(std::make_shared<ingest>(obj["name"], obj["url_template"]));
}

RTMP* twitchspeedtestcli::prepareRtmp(const std::string &url, const std::string &key)
{
    RTMP *rtmp = RTMP_Alloc();
    if (!rtmp)
        return nullptr;

    RTMP_Init(rtmp);

    rtmp->m_inChunkSize = RTMP_DEFAULT_CHUNKSIZE;
    rtmp->m_outChunkSize = RTMP_DEFAULT_CHUNKSIZE;
    rtmp->m_bSendChunkSizeInfo = 1;
    rtmp->m_nBufferMS = 30000;
    rtmp->m_nClientBW = 2500000;
    rtmp->m_nClientBW2 = 2;
    rtmp->m_nServerBW = 2500000;
    rtmp->m_fAudioCodecs = 3191.0;
    rtmp->m_fVideoCodecs = 252.0;
    rtmp->Link.sendTimeout = 10;
    rtmp->Link.swfAge = 30;

    rtmp->Link.flashVer.av_val = (char *) "FMLE/3.0 (compatible; FMSc/1.0)";
    rtmp->Link.flashVer.av_len = (int) strlen(rtmp->Link.flashVer.av_val);

    rtmp->m_outChunkSize = 4096;
    rtmp->m_bSendChunkSizeInfo = TRUE;

    auto nkey = key + "?bandwidthtest";
    nkey = std::regex_replace(nkey, std::regex("\\s+"), "");
    auto nurl = std::regex_replace(url, std::regex("\\{stream_key\\}"), nkey);

    auto res = RTMP_SetupURL(rtmp, strdup(nurl.c_str()));
    if (!res)
    {
        RTMP_Free(rtmp);
        return nullptr;
    }

    RTMP_EnableWrite(rtmp);

    RTMP_AddStream(rtmp, nkey.c_str());

    rtmp->m_bUseNagle = TRUE;

    return rtmp;
}

bool twitchspeedtestcli::sendRTMPMetadata(RTMP *rtmp)
{
    char metadata[2048] = {0};
    char *enc = metadata + RTMP_MAX_HEADER_SIZE, *pend = metadata + sizeof(metadata) - RTMP_MAX_HEADER_SIZE;

    enc = AMF_EncodeString(enc, pend, &av_setDataFrame);
    enc = AMF_EncodeString(enc, pend, &av_onMetaData);

    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_width, 16);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_height, 16);
    enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//
    enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, 10000);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, 30);
    enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//audioCodecID);//
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, 128); //ex. 128kb\s
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, 44100);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, 2);
    enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, 1);

    enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &av_OBSVersion);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    RTMPPacket packet = { 0 };

    packet.m_nChannel = 0x03;        // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INFO;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = rtmp->m_stream_id;
    packet.m_hasAbsTimestamp = TRUE;
    packet.m_body = metadata + RTMP_MAX_HEADER_SIZE;
    packet.m_nBodySize = enc - metadata + RTMP_MAX_HEADER_SIZE;

    return RTMP_SendPacket(rtmp, &packet, FALSE);
}

twitchspeedtestcli::twitchspeedtestcli()
{
    loadIngests();
}

void twitchspeedtestcli::startTest(Args const& args)
{
    results.clear();
    if (ingests.size() == 0)
    {
        std::cerr << "Failed to load twitch ingests" << std::endl;
        return;
    }

    std::list<std::shared_ptr<ingest>> filteredIngests;
    for (auto const &ingest: ingests)
    {
        if (args.regexSearch.length() > 0 && !std::regex_search(ingest->name, std::regex(args.regexSearch)))
            continue;

        if (args.stringSearch.length() > 0 && ingest->name.find(args.stringSearch) == std::string::npos)
            continue;

        filteredIngests.push_back(ingest);
    }

    std::cout << "Ingests to be tested: " << filteredIngests.size() << std::endl;

    int i = 0;
    for (auto const &ingest: filteredIngests)
    {
        std::cout << "Testing [" << ++i << "/" << filteredIngests.size() << "] " << ingest->name << " | " << ingest->url
                  << std::endl;
        RTMP *rtmp = prepareRtmp(ingest->url, args.key);
        if (!rtmp)
        {
            std::cerr << "Failed to initialize RTMP" << std::endl;
            continue;
        }

        float speed;
        if (testRtmp(rtmp, args, speed))
            results.emplace_back(ingest, true, speed);
        else
            results.emplace_back(ingest, false, 0);

        std::cout << std::endl;
    }

    if (auto bestResult = getBestResult())
    {
        std::cout << "Best result - " << bestResult->i->name << " | " << "Speed: " << bestResult->speed << " kbps"
                  << std::endl;
    } else
    {
        std::cout << "Best result: none" << std::endl;
    }
}

bool twitchspeedtestcli::testRtmp(RTMP *rtmp, Args const& args, float& speed)
{
    if (!RTMP_Connect(rtmp, NULL))
    {
        RTMP_Free(rtmp);
        std::cerr << "Failed to connect RTMP" << std::endl;
        return false;
    }

    int duration = args.testTime;
    if (duration <= 0)
        duration = 10;

    duration *= 1000;

    int uncappedDuration = std::min(args.minTestTime > 0 ? args.minTestTime * 1000 : 5000, duration);

    RTMPPacket packet = { 0 };
    struct sockaddr_storage clientSockName;
    socklen_t nameLen = sizeof(struct sockaddr_storage);
    getsockname(rtmp->m_sb.sb_socket, (struct sockaddr *) &clientSockName, &nameLen);

    if (!RTMP_ConnectStream(rtmp, 0))
    {
        RTMP_Free(rtmp);
        std::cerr << "Failed to connect RTMP stream" << std::endl;
        return false;
    }

    if (!sendRTMPMetadata(rtmp)) {
        std::cerr << "Send RTMP metadata failed" << std::endl;
        return false;
    }

    unsigned char junk[4096] = { 0xde };

    packet.m_nChannel = 0x05; // source channel
    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet.m_body = (char *) junk + RTMP_MAX_HEADER_SIZE;
    packet.m_nBodySize = sizeof(junk) - RTMP_MAX_HEADER_SIZE;

    uint64_t realStart = os_gettime_ns();
    uint64_t startTime = 0, lastUpdateTime = 0;
    uint64_t bytesSent = 0, startBytes = 0;
    uint64_t sendTime = 0, sendCount = 0;

    bool uncapped = false, failed = false;

    for (;;)
    {
        uint64_t nowTime = os_gettime_ns();

        if (!RTMP_SendPacket(rtmp, &packet, FALSE))
        {
            failed = true;
            std::cerr << "Failed to connect send RTMP packet" << std::endl;
            break;
        }

        uint64_t diff = os_gettime_ns() - nowTime;

        sendTime += diff;
        sendCount++;

        bytesSent += packet.m_nBodySize;

        if (nowTime - realStart > startMeasureWait && !startTime)
        {
            startTime = nowTime;
            startBytes = bytesSent;
            lastUpdateTime = nowTime;
        }

        if (startTime && (bytesSent - startBytes > 0 && nowTime - startTime > 250))
        {
            speed = ((bytesSent - startBytes)) / ((nowTime - startTime) / 1000.0f);
            speed = speed * 8 / 1000;

            uncapped = speed > 10000;
        }

        if (startTime && (nowTime - lastUpdateTime > 500))
        {
            lastUpdateTime = nowTime;

            std::cout << "\r" << "Speed: " << (std::to_string((uint64_t)speed) + " kbps") << ", Passed: " << std::fixed << std::setprecision(2) << (nowTime - startTime) / 1000.0 << "s" << std::flush;

            if (nowTime - startTime > (uncapped ? uncappedDuration : duration))
            {
                std::cout << std::endl;
                break;
            }
        }
    }

    RTMP_DeleteStream(rtmp, 0);
    RTMP_Close(rtmp);
    closesocket(rtmp->m_sb.sb_socket);
    RTMP_Free(rtmp);

    return !failed;
}

result* twitchspeedtestcli::getBestResult()
{
    result *bestRes = nullptr;
    for (auto &res: results)
    {
        if (res.ok)
        {
            if (!bestRes || res.speed > bestRes->speed)
                bestRes = &res;
        }
    }

    return bestRes;
}

void twitchspeedtestcli::dumpResults(std::string const& path)
{
    using json = nlohmann::json;

    auto bestRes = getBestResult();

    json obj;
    obj["best_name"] = bestRes ? bestRes->i->name : "";
    obj["best_speed"] = bestRes ? bestRes->speed : 0.0;
    obj["injests"] = json::array();

    for (auto &res: results)
    {
        obj["injests"].push_back(
                json::object({
                                     { "name",     res.i->name },
                                     { "endpoint", res.i->url },
                                     { "ok",       res.ok },
                                     { "speed",    res.speed },
                             }));
    }

    std::ofstream file(path);
    file << obj.dump(4);
    file.close();

    return;
}

std::list<std::shared_ptr<ingest>>const& twitchspeedtestcli::getIngests() const
{
    return ingests;
}
