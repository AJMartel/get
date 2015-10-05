#ifndef _PROGRESS_BAR_H_
#define _PROGRESS_BAR_H_

#include <cstddef>
#include <string>
#include <utility>
#include <chrono>

#include "utils.h"

/**
 * Progress bar looks like this:
 *
 *  [***>    ] 18.1MiB / 791.9 MiB @ 5.7 MiB/s
 */
class ProgressBar
{
private:
    unsigned m_cols;
    unsigned m_width;
    const std::size_t m_bytes;
    std::size_t m_bytes_received;
    unsigned m_old_position;
    std::chrono::high_resolution_clock::time_point m_old_time;
    std::size_t m_old_byte_cnt;

    std::pair<double, std::string> unit(std::size_t file_size, unsigned precision = 1) const;
    std::string build_size() const;
    std::string rate(std::size_t bytes_received);

public:
    ProgressBar(std::size_t bytes) :
        m_cols{Utils::terminal_width()},
        m_bytes{bytes}, m_bytes_received{0}, m_old_position{0},
        m_old_time{std::chrono::high_resolution_clock::now()},
        m_old_byte_cnt{0}
    {
        // taking like 50 %
        m_width = m_cols * 50 / 100;
    }

    void update(std::size_t new_bytes = 1);
};

#endif /* _PROGRESS_BAR_H_ */