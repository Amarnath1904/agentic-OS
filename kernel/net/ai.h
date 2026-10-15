#ifndef NET_AI_H
#define NET_AI_H
#include "../kernel.h"
#define AI_PROMPT_CAP 512u
#define AI_REPLY_CAP 8193u
/* NULL on success; bridge errors are reply text with remote_error set.
 * Caller provides AI_REPLY_CAP bytes. One outstanding request at a time. */
const char* ai_ask(const char* prompt, char* reply, bool* remote_error);
const char* ai_plan(const char* prompt, char* reply, bool* remote_error);
#endif
