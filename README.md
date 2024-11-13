# twitchspeedtest-cli
A tool to test Twitch ingest endpoint connectivity speed.

A cross-platform command line version of [original tool](https://r1ch.net/projects/twitchtest) and it's [linux port](https://github.com/danieloneill/TwitchTest)

```console
Usage: twitchspeedtest-cli [OPTIONS]...
Tests upload speed to all known Twitch ingest endpoints and suggests fastest

Options:
  --key=KEY               Twitch stream key
  --out=PATH              Path for results to be written in json format
  --match=TEXT            Ingest endpoint name must contain TEXT
  --regex=EXPR            Ingest endpoint name must match EXPR
  --test-time=VALUE       Test time (default 10)
  --min-test-time=VALUE   Minimum test time (default 5)
  --ingests               Print available ingest endpoints
```

## Example:
```console
wtf@WTF-PC:~$ ./twitchspeedtest-cli --key=live_XXXXXXXXXXXXXX --match=Oceania --out=result.json
Ingests to be tested: 2
Testing [1/2] Oceania: Australia, Sydney (2) | rtmp://syd02.contribute.live-video.net/app/{stream_key}
Speed: 79214 kbps, Passed: 5.46s

Testing [2/2] Oceania: Australia, Sydney (3) | rtmp://syd03.contribute.live-video.net/app/{stream_key}
Speed: 76281 kbps, Passed: 5.10s

Best result - Oceania: Australia, Sydney (2) | Speed: 79214.23 kbps

wtf@WTF-PC:~$ cat result.json 
{
    "best_name": "Oceania: Australia, Sydney (2)",
    "best_speed": 79214.234375,
    "injests": [
        {
            "endpoint": "rtmp://syd02.contribute.live-video.net/app/{stream_key}",
            "name": "Oceania: Australia, Sydney (2)",
            "ok": true,
            "speed": 79214.234375
        },
        {
            "endpoint": "rtmp://syd03.contribute.live-video.net/app/{stream_key}",
            "name": "Oceania: Australia, Sydney (3)",
            "ok": true,
            "speed": 76281.640625
        }
    ]
}
```
