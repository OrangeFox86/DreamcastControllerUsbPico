#include "UsbCdcTtyParser.hpp"
#include "hal/System/LockGuard.hpp"

#include <limits>
#include <string.h>
#include <algorithm>

const char* UsbCdcTtyParser::WHITESPACE_CHARS = "\n\r\t ";

UsbCdcTtyParser::UsbCdcTtyParser(MutexInterface& m, char helpChar) :
    mParserRx(),
    mParserMutex(m),
    mCommandReady(false),
    mHelpChar(helpChar),
    mParsers(),
    mOverflowDetected(false)
{}

void UsbCdcTtyParser::addCommandParser(std::shared_ptr<CommandParser> parser)
{
    mParsers.push_back(parser);
}

void UsbCdcTtyParser::addChars(const char* chars, uint32_t len)
{
    // Entire function is locked
    LockGuard lockGuard(mParserMutex);

    // Reserve space for new characters
    uint32_t newCapacity = mParserRx.size() + len;
    if (newCapacity > MAX_QUEUE_SIZE)
    {
        newCapacity = MAX_QUEUE_SIZE;
    }
    mParserRx.reserve(newCapacity);

    for (uint32_t i = 0; i < len; ++i, ++chars)
    {
        // Flag overflow - next command will be ignored
        if (mParserRx.size() >= MAX_QUEUE_SIZE && *chars != 0x08)
        {
            mOverflowDetected = true;
        }

        if (mOverflowDetected)
        {
            if (*chars == '\n')
            {
                printf("Error: Command input overflow\n");
                // Remove only command that overflowed
                std::vector<char>::reverse_iterator lastEnd =
                    std::find(mParserRx.rbegin(), mParserRx.rend(), '\n');
                if (lastEnd == mParserRx.rend())
                {
                    mParserRx.clear();
                }
                else
                {
                    // We still captured a full command, so at least leave that for processing
                    mParserRx.erase((lastEnd + 1).base(), mParserRx.end());
                }
                mOverflowDetected = false;
            }
        }
        else if (*chars == 0x08)
        {
            if (!mParserRx.empty())
            {
                // Backspace
                mParserRx.pop_back();
            }
        }
        else
        {
            if (*chars == '\n')
            {
                mCommandReady = true;
            }
            mParserRx.push_back(*chars);
        }
    }
}

void UsbCdcTtyParser::process()
{
    // Only do something if a command is ready
    if (mCommandReady)
    {
        // Begin lock guard context
        LockGuard lockGuard(mParserMutex);

        mCommandReady = false;

        // End of command is at the new line character
        std::vector<char>::iterator eol =
            std::find(mParserRx.begin(), mParserRx.end(), '\n');

        if (eol != mParserRx.end())
        {
            // Just in case it gets parsed as a string, changed EOL to NULL
            *eol = '\0';
            // Move past whitespace characters
            const char* ptr = &mParserRx[0];
            uint32_t len = eol - mParserRx.begin();
            while (len > 0 && strchr(WHITESPACE_CHARS, *ptr) != NULL)
            {
                --len;
                ++ptr;
            }

            if (len > 0)
            {
                if (*ptr == mHelpChar)
                {
                    printf("HELP\n"
                           "Command structure: [whitespace]<command-char>[command]<\\n>\n"
                           "\n"
                           "COMMANDS:\n");
                    printf("%c: Prints this help\n", mHelpChar);
                    // Print help for all commands
                    for (std::vector<std::shared_ptr<CommandParser>>::iterator iter = mParsers.begin();
                        iter != mParsers.end();
                        ++iter)
                    {
                        (*iter)->printHelp();
                    }
                }
                else
                {
                    // Find command parser that can process this command
                    bool processed = false;
                    for (std::vector<std::shared_ptr<CommandParser>>::iterator iter = mParsers.begin();
                        iter != mParsers.end() && !processed;
                        ++iter)
                    {
                        if (strchr((*iter)->getCommandChars(), *ptr) != NULL)
                        {
                            (*iter)->submit(ptr, len);
                            processed = true;
                        }
                    }

                    if (!processed)
                    {
                        printf("Error: Invalid command\n");
                    }
                }
            }
            // Else: empty string - do nothing

            mParserRx.erase(mParserRx.begin(), eol + 1);
        }
    } // End lock guard context
}
