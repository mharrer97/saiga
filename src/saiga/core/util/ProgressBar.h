/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"
#include "saiga/core/util/Thread/threadName.h"
#include "saiga/core/util/assert.h"

#include <iostream>
#include <mutex>
#include <string>
namespace Saiga
{
/**
 * A synchronized progress bar for console output.
 * You must not write to the given stream while the progress bar is active.
 *
 * Usage Parallel Image loading:
 *
 * SyncedConsoleProgressBar loadingBar(std::cout, "Loading " + to_string(N) + " images ", N);
 * #pragma omp parallel for
 * for (int i = 0; i < N; ++i)
 * {
 *     images[i].load("...");
 *     loadingBar.addProgress(1);
 * }
 *
 */
struct SyncedConsoleProgressBar
{
    SyncedConsoleProgressBar(std::ostream& strm, const std::string header, int end, int length = 30)
        : strm(strm), header(header), end(end), length(length)
    {
        print();
        run();
    }

    ~SyncedConsoleProgressBar()
    {
        running = false;
        st.join();
        print();
        strm << "Done." << std::endl;
    }
    void addProgress(int i) { current += i; }

   private:
    std::ostream& strm;
    ScopedThread st;
    std::string header;
    bool running = true;
    int end;
    int length;
    int current = 0;

    void run()
    {
        st = ScopedThread([this]() {
            while (running)
            {
                print();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    void print()
    {
        SAIGA_ASSERT(current <= end);
        strm << "\r" << header << " [";
        auto progress = double(current) / end;
        int barLength = progress * length;
        for (auto i = 0; i < barLength; ++i)
        {
            strm << "=";
        }
        for (auto i = barLength; i < length; ++i) strm << " ";

        strm << "] " << progress * 100 << "% " << std::flush;
    }
};

}  // namespace Saiga
