#ifndef TWITCHTEST_CLI_PLATFORM_H
#define TWITCHTEST_CLI_PLATFORM_H

#define UNUSED_PARAMETER(param) (void)param

#ifdef __cplusplus
extern "C" {
#endif
  extern uint64_t os_gettime_ns();
#ifdef __cplusplus
}
#endif

#endif //TWITCHTEST_CLI_PLATFORM_H
