#include "shell.h"
#include "shell_port.h"

#include <string.h>

#define MAX_WORDS_NUM   7
#define MAX_WORD_LEN    12

#ifndef FOR_EACH_PP
#   define FOR_EACH_PP(parr)   for (void **pp = (void **)(parr); *pp; pp++)
#endif

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

static const struct ShellCommand *FindByName(const struct ShellCommand *const *commands, const char *name)
{
    const struct ShellCommand *command;

    FOR_EACH_PP(commands) {
        command = (*pp);
        if (!strcmp(name, command->name))
            return command;
    }
    return NULL;
}

static void PrintCommands(const struct ShellCommand *const *commands)
{
    const struct ShellCommand *command;

    PrintString("CMDLIST:\n");
    FOR_EACH_PP(commands) {
        command = (*pp);
        PrintString(command->name);
        PrintChar(' ');
    }
    PrintChar('\n');
}

void Shell_Spin(const struct ShellCommand *const *commands)
{
    if (!LineAvailable())
        return;

    char inputLine[128];
    char words[MAX_WORDS_NUM][MAX_WORD_LEN];

    ReadLine(inputLine, sizeof(inputLine));

    int wordCount = ParseWords(inputLine, words, MAX_WORDS_NUM);

    if (wordCount < 1) {
        PrintString("EOL\n");
        return;
    }

    // Prepare input for shell command.
    int argc = wordCount - 1;
    char *argv[MAX_WORDS_NUM - 1] = {NULL};
    for (int i = 0; i < argc; i++) {
        argv[i] = &words[i + 1][0];
    }

    const struct ShellCommand *command = FindByName(commands, words[0]);

    if (!command) {
        if (!strcmp(words[0], "?")) {
            PrintCommands(commands);
        }
        else {
            PrintString("NOCMD ");
            PrintString(words[0]);
            PrintChar('\n');
        }
        return;
    }

    int retcode = command->execute(argc, argv);

    if (retcode != 0) {
        PrintString("RET ");
        PrintInt(retcode);
        PrintChar('\n');
    }
}