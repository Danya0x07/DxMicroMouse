#include "shell.h"
#include "uart.h"
#include "module.h"

#include <stdio.h>
#include <string.h>

#define MAX_WORDS_NUM   7
#define MAX_WORD_LEN    12

static void SendTelemetry(void)
{
    struct Module *m;

    FOR_EACH_MODULE(addr) {
        m = *addr;
        if (m->telemetry)
            Telemetry_Send(m->telemetry);
    }
}

static void PrintModules(void)
{
    struct Module *m;

    UART_SendString("Modules:\n");
    FOR_EACH_MODULE(addr) {
        m = *addr;
        UART_SendString(m->name);
        if (m->telemetry)
            UART_SendChar('*');
        UART_SendChar(' ');
    }
    UART_SendChar('\n');
}

static struct Module *FindModuleByName(const char *name)
{
    struct Module *m;

    FOR_EACH_MODULE(addr) {
        m = *addr;
        if (!strcmp(name, m->name))
            return m;
    }
    return NULL;
}

static int ParseWords(char *input, char (*words)[MAX_WORD_LEN], int maxNum)
{
    int wordCount = 0;

    char *strPart = strtok(input, " ");
    while (strPart && wordCount < maxNum) {
        strncpy(&words[wordCount][0], strPart, MAX_WORD_LEN);
        words[wordCount][MAX_WORD_LEN - 1] = '\0';
        wordCount++;
        strPart = strtok(NULL, " ");
    }
    return wordCount;
}

static void HandleInput(void)
{
    if (!UART_LineReceived())
        return;

    char inputLine[128];
    char words[MAX_WORDS_NUM][MAX_WORD_LEN];

    UART_ReadLine(inputLine, sizeof(inputLine));
    UART_Flush();

    int wordCount = ParseWords(inputLine, words, MAX_WORDS_NUM);

    if (wordCount < 1) {
        printf("No words query.\n");
        return;
    }

    // Prepare input for either shell command or module command.
    int argc = wordCount - 1;
    char *argv[MAX_WORDS_NUM - 1] = {NULL};
    for (int i = 0; i < argc; i++) {
        argv[i] = &words[i + 1][0];
    }

    struct Module *module = FindModuleByName(words[0]);

    if (!module) {
        if (!strcmp(words[0], "?"))
            PrintModules();
        else
            printf("No such module %s\n", words[0]);
        return;
    }

    if (!strcmp(argv[0], "telemetry")) {
        if (argc == 2) {
            if (!module->telemetry) {
                printf("%s has no telemetry()\n", module->name);
                return;
            }

            if (!strcmp(argv[1], "on")) {
                module->telemetry->enabled = true;
            }
            else { // off
                module->telemetry->enabled = false;
            }
        }
        else {
            printf("Usage: MODULE telemetry on|off\n");
            return;
        }
    }
    else {
        if (!module->execute) {
            printf("%s has no execute()\n", module->name);
            return;
        }

        int retcode = module->execute(argc, argv);

        if (retcode != 0) {
            printf("RetCode: %d\n", retcode);
        }
    }
}

void Shell_Spin(void)
{
    HandleInput();
    SendTelemetry();
}